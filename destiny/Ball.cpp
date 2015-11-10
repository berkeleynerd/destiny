#include "StdAfx.h"
#include "Ball.h"
#include "Ballpark.h"
#include "Partition.h"
#include "Box.h"
#include "Vector3.h"
#include <Trinity/include/TriMath.h>

#define ABS(X) ((X)<0.0?-(X):(X))
#define MIN(X,Y) ((X)<(Y)?(X):(Y))
#define SIGNUM(X) ((X)>=0.0?1.0:-1.0)

#define _HALFPI_  1.57079632679489655
#define _PI_  3.1415926535897931
#define _2PI_ 6.2831853071795862

void Modulo2pi(double& a,double& b);
double floatrand(double a,double b){
    //note, this is very poor!
    return a+rand()/(double)RAND_MAX*(b-a);
}

BLUE_DEFINE_INTERFACE( IEveReferencePoint );

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "Ball" );

static int count = 0;
static int ballCount = 0;

#define DSTCSTACKTRACE()
//__try{RaiseException(0, 0, 0, NULL);}__except (StackTrace(GetExceptionInformation())){}
ClientBall::ClientBall(IRoot *lockobj) :
    mLastYaw(0.0),
    mLastPitch(0.0),
    mLastRoll(0.0),
    mYawSpeed(0.0),
    mPitchSpeed(0.0),
    mPosUpdateTime(0),
    mRotUpdateTime(0),
    mElapsed(0.0),
    mCenterDist(0.0),
    mSurfaceDist(0.0),
	mMaxAngle(0.25f),
	mMaxAngularVelocity(0.5f * float( _PI_ )),
	mInvInertiaTensor(-1.0),
	mCurrentVelocityInfluence(1.0f),
    mLastTick(-1),
	mShowBoxes(false),
	mProcessingImpact(false),
	mMinimumAngularVelocity(0.01f),
	mSpeedModifier(10.0f)
{
    //CCP_LOG_CH( s_ch,"New Ball:%d. Total is: %d\n",int(this),++ballCount);
    mBoxList = PyList_New(27);
    mLastRot = Quaternion(0.0f,0.0f,0.0f,-1.0f);
	mImpactRotation = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
	mImpactVelocity = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
	mImpactVelocityGoal = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
};

Ball::Ball(IRoot* lockobj) :
    mPark(0),
    mRadius(0.0),
    mMass(0.0),
    isFree(false),
    isMassive(false),
    isGlobal(false),
    isSpaceJunk(false),
    isCloaked(0),
    mHarmonic(-1),
    mAllianceID(-1),
    mCorporationID(-1),
    mFormationID(-1),
    mMaxVel(0.0),
    mSpeedFraction(1.0),
    mAgility(1.0),
    mNewYaw(0.0),
    mOldYaw(0.0),
    mNewPitch(0.0),
    mOldPitch(0.0),
    mNewRoll(0.0),
    mOldRoll(0.0),
    mNewRollSpeed(0.0),
    mOldRollSpeed(0.0),
    mYawDelta(0.0),
    mNewTime(0),
    mOldTime(0),
    mMode(DSTBALL_STOP),
    mFollowRange(10.0),
    mFollowId(0),
    mOwnerId(0),
    mFollowPtr(0),
    mTimeFactor(1.0),
    mLastCollision(-1.0),
    mWithinRange(false),
    mHandled(false),
    mHasProximity(false),
    mNotificationRange(0.0),
    mTrackingPtr(0),
    mTrackingRange(0.0),
    mWithinTrackingRange(false),
    PARENTLOCK(mMiniBalls),
	PARENTLOCK(mMiniBoxes),
	PARENTLOCK(mMiniCapsules)
{
    mNewPos = Vector3d(1.e10,1.e10,1.e10);
    mCollisions.reserve(2);
    ClearBoxes();
    ++ballCount;
};

//--------------------------------------------------------------------
// Destructor
//--------------------------------------------------------------------

Ball::~Ball()
{
    --ballCount;
    if(ballCount <= 0)
        CCP_LOG_CH( s_ch,"Ball:%p destroyed. Total is: %d\n",this,ballCount);
}

//--------------------------------------------------------------------
// Destructor
//--------------------------------------------------------------------

ClientBall::~ClientBall()
{
    Py_DECREF(mBoxList);
    //CCP_LOG_CH( s_ch,"Ball:%d destroyed. Total is: %d\n",int(this),--ballCount);
}

//---------------------------------------------------------------------------------------
// RegisterBallNotInPark attempts to call a python callback to deal with BallNotInPark errors
//---------------------------------------------------------------------------------------
void Ball::RegisterBallNotInPark()
{
    if( Ballpark::s_ballNotInParkCallback && Ballpark::s_ballNotInParkCallback != Py_None )
    {
        PyObject* pyThis = PyOS->WrapBlueObject((IBall *)this);
        PyObject* ret = PyObject_CallFunction( Ballpark::s_ballNotInParkCallback, (char*)"O", pyThis );
        Py_XDECREF(pyThis);
        Py_XDECREF(ret);
    }
}

//---------------------------------------------------------------------------------------
// ClearBoxes Deletes links to all boxes
//---------------------------------------------------------------------------------------

void Ball::ClearBoxes()
{
    mBoxes.clear();
    mMyBox = 0;
    int i;
    for(i=0;i<27;i++)
        mActiveBoxes[i] = false;

    for(i=0;i<3;i++)
        mMod[i] = -2;
}

//---------------------------------------------------------------------------------------
// AddProximitySensor adds a proximity sensor with the given range to the ball
//---------------------------------------------------------------------------------------

long Ball::AddProximitySensor(float range, double period, int shuffle, bool onlyInteractives)
{
    if( mRadius + range < 0.0)
        return -1;

    mSensor.balls.clear();
    mSensor.range = range;
    mSensor.period = period;
    mSensor.active = true;
    mSensor.onlyInteractives = onlyInteractives;
    if(shuffle)
    {
        mSensor.elapsed = floatrand(0.0, period);
    }

    UpdateProximityStatus();
    return 0;
}

//---------------------------------------------------------------------------------------
// UpdateProximityStatus removes a previously added proximity sensor
//---------------------------------------------------------------------------------------
void Ball::UpdateProximityStatus()
{
    if(mTrackingRange != 0.0 || mNotificationRange != 0.0 || mSensor.active)
    {
        mHasProximity = true;
        mPark->mProximityBalls.insert(DictEntry(mId, this));
    }
    else
    {
        mHasProximity = false;
        mPark->mProximityBalls.erase(mId);
    }
}

//---------------------------------------------------------------------------------------
// RemoveProximitySensor removes a previously added proximity sensor
//---------------------------------------------------------------------------------------

void Ball::RemoveProximitySensor()
{
    mSensor.active = false;
    UpdateProximityStatus();
}

void Ball::CalculateYawPitchRoll(bool snap)
{
	if( !mPark )
    {
        // This ball is no longer in any ballpark
        mNewYaw = 0.0;
        mNewPitch = 0.0;
        mNewRoll = 0.0;
        //CCP_LOGERR_CH( s_ch,"Ball:%I64d no longer in ballpark, CalculateYawPitchRoll\n", mId);
        RegisterBallNotInPark();
        return;
    }

    // Transfer values to old
    mOldPitch     = mNewPitch;
    mOldYaw       = mNewYaw;
    mOldRoll      = mNewRoll;
    mOldRollSpeed = mNewRollSpeed;


    // Define some constants
    Vector3d offset;
    Vector3d direction;

    // massModifiers is used to make heavier ships slower in rotations
    double massModifier = mMass/mPark->mFriction;

    if(massModifier != 0.0)
    {
        massModifier = 100.0/massModifier;
    }
    else
    {
        // Use some default value if mass is zero
        massModifier  = 100.0;
    }

    double vel2 = mNewVel.LengthSq();
    // The direction of the velocity vector gives me yaw and pitch
    if(vel2 == 0.0)
    {
        // Indeterminate direction...use last goto
        direction = mGoto;
    }
    else
    {
        if( mMode == DSTBALL_WARP )
        {    // Really heavy ships have a tendancy to warp sideways.
            if( mEffectStamp == -1 ) //If they are aligning for warp, make them rotate towards their goto point, rather then their current velocity
            {
                //CCP_LOG_CH( s_ch,"Aligning %I64d: %d collisions.", mId, mCollisions.size());
                if( mCollisions.size() == 0) 
                    direction = mGoto - mNewPos; 
                else // The degenerate case where you're aligning and bumping an obstacle. You should rotate with your bumps.
                    direction = mNewVel;
            }
            else if( massModifier < 1.0 && mPark->mCurrentTime != mEffectStamp )
            { //If they are in warp, let them rotate faster as they gain speed into the warp
                //CCP_LOG_CH( s_ch,"Warping %I64d: Adjusting massModifier from %f.  EffectStamp id %d while ballpark stamp is %d", mId, massModifier, mEffectStamp, mPark->mCurrentTime);
                massModifier = MIN(1.0, massModifier * MIN(15.0, (mPark->mCurrentTime-mEffectStamp)*2));
                //CCP_LOG_CH( s_ch,"  massModifier on ball %I64d adjusted to %f.", mId, massModifier);
            }
        }
        else
        {
            direction = mNewVel;
        }

        if(mMode!=DSTBALL_MISSILE && !snap)
        {
            // for all balls that are not missile, then let the direction be
            // slightly in the destination point
            offset = mGoto - mNewPos;
            offset.Normalize();
            direction = direction + sqrt(vel2)*0.3*offset;
        }
    }

    // Yaw is directly calculated from the direction
    mNewYaw = atan2(direction.x, direction.z);

    // Same thing with pitch
    double r = direction.Length();
    if(r > 0.0)
        mNewPitch = -asin(direction.y/r);
    else
        mNewPitch = 0.0;

    // Clamp the values to the correct angle ranges
    Modulo2pi(mOldYaw, mNewYaw);
    Modulo2pi(mOldPitch, mNewPitch);

    if(snap)
    {
        mNewRoll = 0.0;
        mOldRoll = 0.0;
        mNewRollSpeed = 0.0;
        mOldRollSpeed = 0.0;
        mYawDelta = 0.0;
        return;
    }

    // Now actually make the angle change smooth, and dependent on mass.
    double tmp = 1.0/(1.0 + massModifier*0.5); 
    mNewYaw   = (mOldYaw   + massModifier*0.5*mNewYaw  )*tmp;
    mNewPitch = (mOldPitch + massModifier*0.5*mNewPitch)*tmp;

	// This is basically doing a modulus on the change in yaw, incase we spin very quickly within 1 tick
	mYawDelta = asin(sin(mNewYaw - mOldYaw));

	// Roll speed is increased by changes in yaw
	mNewRollSpeed += mYawDelta * mPark->mRollSpeedAcceleration;

	// Roll speed decays over time
	mNewRollSpeed += mOldRollSpeed * mPark->mRollSpeedDecay * mPark->dt;

	// Clamp roll speed so it doesn't get too crazy
	mNewRollSpeed = std::max(-_HALFPI_, std::min(_HALFPI_, mNewRollSpeed));

	// Roll is increased by the roll speed, scaled over time
	mNewRoll += mNewRollSpeed * mPark->mRollAcceleration * mPark->dt;

	// Roll also decays over time
	mNewRoll += mOldRoll * mPark->mRollDecay * mPark->dt;

	// Clamp the maximum roll so we don't flip upside down
	mNewRoll = std::max(-_HALFPI_, std::min(_HALFPI_, mNewRoll));
}

//---------------------------------------------------------------------------------------
// DispatchCollisions dispatches deco messages for collision events
//---------------------------------------------------------------------------------------

void Ball::DispatchCollisions()
{
    size_t size = mCollisions.size();

    if(size)
    {
        for(size_t i = 0; i < size ; i++)
        {
            if (!PyOS->SendEvent(
                (IBall*)this, "Destiny::Ball::DoCollision",
                "DoCollision", NULL,
                "Lddd", mCollisions[i],mLastC.x,mLastC.y,mLastC.z
                ))
            {
                PyOS->PyError();
            }

            //CCP_LOG_CH( s_ch,"Collision between: %I64d and %I64d\n",mId,mCollisions[i]->mId);
        }
    }
}


//---------------------------------------------------------------------------------------
// CheckForProximity checks whether balls traverse any of the current sensors
//---------------------------------------------------------------------------------------
void Ball::CheckForProximity()
{
    if(!mPark)
    {
        // This ball is no longer in any ballpark
        //CCP_LOGERR_CH( s_ch,"Ball:%I64d no longer in ballpark, CheckForProximity()\n", mId);
        RegisterBallNotInPark();
        return;
    }

    if(isMoribund)
	{
        return;
	}

    CheckForProximity_NotificationRange();
    CheckForProximity_TargetTracking();

    if(!mSensor.active)
	{
        return;
	}
    CheckForProximity_Sensor();

}

void Ball::CheckForProximity_NotificationRange()
{
    bool sendNotification = false;
    bool inside = false;

    if(mNotificationRange!=0.0)
    {
        if(mMode==DSTBALL_GOTO)
        {
            if((mNewPos-mGoto).LengthSq() < (mNotificationRange + mRadius)*(mNotificationRange + mRadius))
                inside = true;
            else
                inside = false;
        }
        else if(mMode==DSTBALL_FOLLOW || mMode==DSTBALL_MISSILE || mMode==DSTBALL_ORBIT)
        {
            double len2 = (mNewPos-mFollowPtr->mNewPos).LengthSq();

            double hiLim = (double)mFollowPtr->mRadius + mFollowRange + mNotificationRange + mRadius;
            double loLim = hiLim - 2.0*mNotificationRange;
            if(loLim < 0.0)
                loLim = 0.0;

            if(len2 < hiLim*hiLim && len2 > loLim*loLim)
                inside = true;
            else
                inside = false;

        }

        if(inside)
        {
            // ball is within notification range
            if(mWithinRange)
            {
                // No need to send notification
            }else
            {
                sendNotification = true;
                mWithinRange = true;
                // Should send notification
            }
        }
        else
        {
            // ball is outside notification range
            if(mWithinRange)
            {
                // Should send notification
                sendNotification = true;
                mWithinRange = false;
            }else
            {
                // No need to send notification
            }
        }
    }

    if(sendNotification)
    {
        if (!PyOS->SendEvent(
            (IBall*)this, "Destiny::Ball::DoRange",
            "DoRange", NULL,
            "(i)", mWithinRange
            ))
        {
            PyOS->PyError();
        }

        if(mWithinRange)
            CCP_LOG_CH( s_ch,"Ball %I64d entered range %f of desired position\n", mId, mNotificationRange);
        else
            CCP_LOG_CH( s_ch,"Ball %I64d left range %f of desired position\n", mId, mNotificationRange);
    }
}
void Ball::CheckForProximity_TargetTracking()
{
    bool sendNotification = false;
    if((mTrackingRange >= 0.00001) && (mTrackingPtr != NULL))
    {

        double distanceToTarget = (mNewPos - mTrackingPtr->mNewPos).LengthSq();
        if(distanceToTarget < ((mTrackingRange + mRadius) * (mTrackingRange + mRadius)))
        {
            // ball is within notification range
            if(!mWithinTrackingRange)
            {
                // Should send notification
                sendNotification = true;
                mWithinTrackingRange = true;
            }
        }
        else
        {
            // ball is outside notification range
            if(mWithinTrackingRange)
            {
                // Should send notification
                sendNotification = true;
                mWithinTrackingRange = false;
            }
        }
    }

    if(sendNotification)
    {
        if (!PyOS->SendEvent(
            (IBall*)this, "Destiny::Ball::DoTargetTracking",
            "DoTargetTracking", NULL,
            "(i)", mWithinTrackingRange
            ))
        {
            PyOS->PyError();
        }

        if(mWithinTrackingRange)
            CCP_LOG_CH( s_ch,"Ball %I64d entered tracking range %f of %I64d\n", mId, mTrackingRange, mTrackingPtr->mId);
        else
            CCP_LOG_CH( s_ch,"Ball %I64d left tracking range %f of %I64d\n", mId, mTrackingRange, mTrackingPtr->mId);
    }
}

void Ball::CheckForProximity_Sensor()
{
    mSensor.elapsed += mPark->dt;

    if(mSensor.elapsed < mSensor.period)
	{
        return;
	}

    mSensor.elapsed = 0.0;
    
    if(mPark->mProbe==mId)
        CCP_LOGWARN_CH( s_ch,"[%d] Checking proximity with range %f", mPark->mCurrentTime, mSensor.range);

    // Keep radius and velocity for restoring
    float radius = mRadius;
    Vector3d vel = mNewVel;

    mNewVel = Vector3d(0.0,0.0,0.0);
    mPark->SetBallRadius(this,mRadius + mSensor.range);

    VectorOfBalls uni;
	VectorOfStaticCollidables uniStat;
    mPark->mPartition->GetProximityCandidates(this, uni, uniStat, mPark->isMaster);

    // restore old ball values
    mNewVel = vel;
    mPark->SetBallRadius(this,radius);

    // Now cycle over balls that are in the sensor, and check whether they should
    // still be there
    if(mSensor.balls.size() > 0)
    {
        //create the set of nearby ballIDs on demand
        std::unordered_set<ID> nearby;
        for (VectorOfBalls::iterator i = uni.begin(); i!=uni.end(); ++i)
            nearby.insert((*i)->mId);

        for(MapOfLongLongs::iterator i = mSensor.balls.begin(); i!= mSensor.balls.end(); ) {
            MapOfLongLongs::iterator store = i++;
            ID anID = store->first;

            // If the sensor ball itself is in the list of neighbors, continue
            if(anID == mId)
                continue;

            // if anID is in the list of neigbors, continue
            if(nearby.find(anID) != nearby.end())
                continue;

            // This sensor ball is not listed in the nearby balls, so it must have disappeared
            mSensor.balls.erase(store);
            CCP_LOG_CH( s_ch,"Ball:%I64d vanished from proximity of ball:%I64d",anID, mId);

            if (!PyOS->SendEvent(
                (IBall*)this, "Destiny::Ball::DoProximity",
                "DoProximity", NULL,
                "Li", anID, 0
                ))
            {
                PyOS->PyError();
            }

            if(!mSensor.active)
                break; // Apparently I have been removed by a proximity event

        }
    }

    // Ok, now cycle over all nearby balls...
    VectorOfBalls::iterator jt;

    for(jt=uni.begin(); jt != uni.end(); ++jt)
    {
        Ball *b = *jt;
        // If I have died as the result of this loop, break away
        if(isMoribund)
            break;

        // If the ball is me, or if it is moribund, continue
        if(b->mId==mId ||  b->isMoribund)
            continue;

        // First find if we already have that ball in range
        bool ballInSensor = mSensor.balls.find(b->mId) != mSensor.balls.end();

        // Now check if it is now within range or not
        double range = (mNewPos - b->mNewPos).LengthSq();
        double ckRange = mSensor.range  + b->mRadius + mRadius;

        if(range > ckRange * ckRange ){
            // Out of range of sensor
            // If I was in the list, I should remove this ball
            // and generate a 'ball left' event
            if(ballInSensor){
                mSensor.balls.erase(b->mId);
                CCP_LOG_CH( s_ch,"Ball:%I64d left proximity of ball:%I64d",b->mId, mId);

                if (!PyOS->SendEvent(
                    (IBall*)this, "Destiny::Ball::DoProximity",
                    "DoProximity", NULL,
                    "Li", b->mId, 0
                    ))
                {
                    PyOS->PyError();
                }
            }

        }else{
            // In range of sensor
            // If I wasn't in the list, I should add this ball
            // and generate a 'ball entered' event
            //CCP_LOG_CH( s_ch,"Ball %I64d is in range of sensor %d with cloak state %d",b->mId, mId, b->isCloaked);
            if(!ballInSensor){
                mSensor.balls.insert(b->mId, 0);
                CCP_LOG_CH( s_ch,"Ball:%I64d entered proximity of ball:%I64d", b->mId, mId);

                if (!PyOS->SendEvent(
                    (IBall*)this, "Destiny::Ball::DoProximity",
                    "DoProximity", NULL,
                    "Li", b->mId, 1
                    ))
                {
                    PyOS->PyError();
                }
            }
        }

        if(!mSensor.active)
            break; // Apparently I have been removed by a proximity event

    } // End of cycling over balls

}

void Modulo2pi(double& a,double& b)
{
    b = fmod(b,_2PI_);
    a = fmod(a,_2PI_);

        if(ABS(b-a) < ABS(b+_2PI_-a) )
        {
            if(ABS(b-a) < ABS(b-_2PI_-a) )
            {
                return;// Unchanged values are fine
            }
            else
            {
                b = b - _2PI_;
                return;
            }
        }
        else
        {
            if(ABS(b+_2PI_-a) < ABS(b-_2PI_-a) )
            {
                b=  b + _2PI_;
                return;
            }
            else
            {
                b =  b - _2PI_;
                return;
            }
        }
}
//---------------------------------------------------------------------------------------
// DeleteBallFromBoxes removes the ball from all its boxes and clears the box list
//---------------------------------------------------------------------------------------

void Ball::DeleteFromBoxes()
{
    SetOfBoxes::iterator it;
    Box* box;

    // Go over all boxes the ball intersects
    for(it = mBoxes.begin();it != mBoxes.end(); ++it)
    {
        box = (*it);
        box->RemoveBall(this);
    }

    // Now clear the box list
    ClearBoxes();
}

void Ball::InsertInBox(Box* box)
{
	if( !box )
	{
		return;
	}
	box->AddBall(this);
	mBoxes.insert(box);
}

double Ball::GetInflatedRadius(double dt)
{
	return mRadius + dt * mNewVel.Length();
}

float Ball::GetBoundingRadius()
{
	return mRadius;
}

void Ball::SetBoxActive(int boxId, bool isActive)
{
	mActiveBoxes[boxId] = isActive;
}

void ClientBall::CalculateYawPitchRoll(bool snap)
{
    Ball::CalculateYawPitchRoll(snap);
    if(snap)
    {
        // Need to assign values for quaternion rotation
        mLastYaw = mNewYaw;
        mLastPitch = mNewPitch;
        D3DXQuaternionRotationYawPitchRoll(reinterpret_cast<D3DXQUATERNION*>( &mLastRot ), float(mLastYaw), float(mLastPitch), float(mLastRoll));
    }
}

void ClientBall::InforceContinuity()
{
    if(!mPark)
	{
        return;
	}

    // We should now calculate an interpolated value
    if(mLastTick != mPark->mCurrentTime)
    {
        // we have switched intervals
        if(mLastTick == -1)
        {
            // This is first
            mLastPos = mOldPos;
            mLastVel = mOldVel;
        }
        mIntAcc = mLastC+mLastG;
        mLastTick = mPark->mCurrentTime;
        mElapsed=0.0;        
    }
}

void Ball::SetMode(DSTBALLMODE mode)
{
    if(mode != mMode)
    {
        if(mode==DSTBALL_WARP)
        {
            // Going into warp

            //mIntAcc = Vector3d(0.0,0.0,0.0);
        }
        else if(mMode==DSTBALL_WARP)
        {
            if (mPark && !PyOS->PostEvent(
                (IEveBallpark*)mPark, "Destiny::OnExitWarp",
                "OnExitWarp",
                "Li", mId,0
                ))
            {
                PyOS->PyError();
            }
        }

        if (!PyOS->SendEvent(
            (IBall*)this, "Destiny::Ball::DoModeChange",
            "DoModeChange", NULL,
            "ii", mMode, mode
            ))
        {
            PyOS->PyError();
        }
    }

    mMode = mode;
}

//---------------------------------------------------------------------------------------
// ITriQuaternionFunction
//
// This function basically returns a quaternion for the ball that is
// based on the current velocity direction of the ball.
// The result quaternion is actually dampened such that discontinous changes
// are hidden.
//---------------------------------------------------------------------------------------

Quaternion* ClientBall::Update(
    Quaternion* in,
    Be::Time time
    )
{
    return GetValueAt(in,time);
}


Quaternion* ClientBall::Update(
    Quaternion* in,
    double time
    )
{
    BeOS->SetError(BEDEF, Clsid(),"Ball::Update() -> Use the Be::Time version instead.");
    *in = Quaternion(0.0f,0.0f,0.0f,-1.0f);
    return in;
}


Quaternion* ClientBall::GetValueAt(
    Quaternion* in,
    Be::Time time
    )
{
    if(!mPark)
    {
        // This ball is no longer in any ballpark
        // This should never happen really
        *in = Quaternion(0.0f,0.0f,0.0f,-1.0f);
        //CCP_LOGERR_CH( s_ch,"Ball:%I64d no longer in ballpark, GetValueAt(D3DXQUATERNION)\n", mId);
        RegisterBallNotInPark();
        return in;
    }

    // This is the time reference we use in interpolations
    Be::Time shiftedTime = GetShiftedTime(time);

    if(mRotUpdateTime == shiftedTime)
    {
        // I have already calculated this, use the cached value
        *in = mLastRot;
        return in;
    }
    if(shiftedTime < mRotUpdateTime)
    {
        // Time is running backwards it seems
        CCP_LOGWARN_CH( s_ch,"Time running backwards in Ball:%I64d",  mId);
        *in = mLastRot;
        return in;
    }
    else if(!isFree)
    {
        // This is a fixed ball. Don't interfere with its rotation
        *in = mLastRot = Quaternion(0.0f,0.0f,0.0f,-1.0f);
        mRotUpdateTime = shiftedTime;
    }
    else if(mNewTime-mOldTime==0)
    {
        // This is some degenerate case. Is called the first time
        CCP_LOGWARN_CH( s_ch,"Someone calling Ball:%I64d before an Evolve",  mId);
        // Just return the last known value
        *in = mLastRot;
        return in;
    }
    else
    {
        InforceContinuity();
        // This is the time elapsed since I was last interpolated
        if(mRotUpdateTime==0)
            mRotUpdateTime = shiftedTime;

        Be::Time deltaTime = shiftedTime-mRotUpdateTime;

        double fraction = double(shiftedTime-mOldTime)/double(mNewTime - mOldTime);

        double aYaw   = mOldYaw   + fraction*mYawDelta;
        double aPitch = mOldPitch + fraction*(mNewPitch - mOldPitch);
        double aRoll  = mOldRoll  + fraction*(mNewRoll  - mOldRoll);

        double friction;

        if(mMode==DSTBALL_MISSILE)
        {
            friction = 10.0;
        }
        else
        {
            friction = 0.9;
        }

        double dT = TimeAsDouble(deltaTime);

        Modulo2pi(mLastYaw,aYaw);
        Modulo2pi(mLastPitch,aPitch);

        mYawSpeed = mLastYaw;
        mPitchSpeed = mLastPitch;
				
        double tmp = 1.0/(1.0 + friction*dT);
        mLastYaw = (mLastYaw + friction*dT*aYaw)*tmp;
        mLastPitch = (mLastPitch + 1.3*friction*dT*aPitch)/(1.0 + 1.3*friction*dT);
        mLastRoll = (mLastRoll + friction*dT*aRoll)*tmp;

        tmp = 1.0/dT;
        mYawSpeed = (mLastYaw-mYawSpeed)*tmp;
        mPitchSpeed = (mLastPitch-mPitchSpeed)*tmp;

        D3DXQuaternionRotationYawPitchRoll(reinterpret_cast<D3DXQUATERNION*>( &mLastRot ), float(mLastYaw), float(mLastPitch), float(mLastRoll));

		if( mProcessingImpact )
		{
			ProcessImpact((float) dT);
		}

        *in = mLastRot;
        mRotUpdateTime = shiftedTime;
    }

    return in;
}


Quaternion* ClientBall::GetValueAt(
    Quaternion* in,
    double time
    )
{
    return in;
}

Quaternion* ClientBall::GetValueDotAt(
    Quaternion* in,
    Be::Time time
    )
{
    in->x = float(mYawSpeed);
    in->y = float(mPitchSpeed);
    return in;
}

Quaternion* ClientBall::GetValueDotAt(
    Quaternion* in,
    double time
    )
{
    return in;
}


void ClientBall::GetDelta(Vector3 *in, Be::Time time)
{
    GetValueAt(in,time);
    // Now mLastPos contains the true position

    *in = mDeltaPos.AsVector3();
}


//---------------------------------------------------------------------------------------
// InterpolatedPosition returns the absolute interpolated position
// It assumes that Evolve has been called at least once to define
// interpolation points
//---------------------------------------------------------------------------------------
 Vector3d* ClientBall::InterpolatedPosition(Vector3d* out, Be::Time time)
{
    if(!mPark)
    {
        // This ball is no longer in any ballpark
        // This should never happen really
        //CCP_LOGERR_CH( s_ch,"Ball:%I64d no longer in ballpark, InterpolatedPosition",  mId);
        RegisterBallNotInPark();
        *out = Vector3d(0.0, -std::numeric_limits<float>::max(), 0.0);
        return out;
    }
    
    // Shift time down one tick
    Be::Time shiftedTime  = GetShiftedTime(time);

    if(mPosUpdateTime==shiftedTime)
    {
        // This function has already been called for this time value
        // Return a cached value
        *out = mLastPos;
        return out;
    }

    if(shiftedTime < mPosUpdateTime)
    {
        // Removing this warning. After speaking to Einar we decided to temporarily remove this warning until
        // someone cleans up this part of Destiny: both ::GetValueAt() with time as a double are basically not
        // working, so they will always trigger this warning. Complete re-implementation of these function
        // is necessary! Steve, 4.11.2010
        //CCP_LOGWARN_CH( s_ch,"InterpolatedPosition: Time running backward in Ball:%I64d [%I64d %I64d %I64d]",  mId, mPosUpdateTime, shiftedTime, shiftedTime - mPosUpdateTime);
        *out = mLastPos;
        return out;
    }

    if(mOldTime==mNewTime)
    {
        // No interpolation possible. Just return the newest value
        CCP_LOG_CH( s_ch,"Time span zero in Ball:%I64d interpolation",  mId);
        mLastPos = mNewPos;
        mPosUpdateTime = shiftedTime;
        *out = mLastPos;
        return out;
    }
            
    InforceContinuity();

    bool shouldUpdateDist = false;
    if(mElapsed==0.0)
        shouldUpdateDist = true;

    // fraction tells us how far we are in the interpolation segment. Should usually be between 0 and 1.
    double fraction = TimeAsDouble(shiftedTime-mOldTime);
    
    if(mPosUpdateTime==0)
        mPosUpdateTime = shiftedTime;

    Be::Time deltaTime = shiftedTime-mPosUpdateTime;
    double dt = TimeAsDouble(deltaTime);

    // Remember the delta between current position and the new one
    mDeltaPos = mLastPos;

    mElapsed += dt;

    // Calculate the new position
    if(mMode != DSTBALL_WARP || (mMode == DSTBALL_WARP && mEffectStamp== -1))
    {
        mLastPos = mOldPos;
        mLastVel = mOldVel;
        mPark->Integrate(mLastPos, mLastVel, mIntAcc, mMass * mAgility, mPark->mFriction, mTimeFactor, fraction);
    }
    else
    {
        double t = ((mPark->mCurrentTime - mEffectStamp) - 1 + fraction)*mPark->dt;
        mPark->WarpDistance(
            this,
            mLastPos,
            mLastVel,
            t,
            true
        );
    }
    
    // Calculate the delta with last position and keep it
    mDeltaPos = mLastPos - mDeltaPos;

    // Mark this update
    mPosUpdateTime = shiftedTime;

    if(shouldUpdateDist && mId != mPark->mEgo)
    {
        Vector3d reference(0.0, 0.0, 0.0);
        mPark->GetReferencePoint(&reference, time);

        mCenterDist = (mLastPos - reference).Length() ;
        Ball *theEgo = mPark->GetEgo();
        if(theEgo)
            mSurfaceDist = mCenterDist - mRadius - theEgo->mRadius;
    }

    if(mShowBoxes)
        DispatchPartition();
    *out = mLastPos;
    return out;
}

//---------------------------------------------------------------------------------------
// This will standardize the getting of the Shifted time to be called when we set the 
// the mPosUpdateTime.
//---------------------------------------------------------------------------------------
Be::Time ClientBall::GetShiftedTime(Be::Time time)
{
    if (!mPark)
    {
        return time;
    }
    return time - 2*mPark->mTickInterval*10000;
}
//---------------------------------------------------------------------------------------
// ITriVectorFunction
//---------------------------------------------------------------------------------------

Vector3* ClientBall::Update(
    Vector3* in,
    Be::Time time
    )
{
    return GetValueAt(in,time);
}


Vector3* ClientBall::Update(
    Vector3* in,
    double time
    )
{
    BeOS->SetError(BEDEF, Clsid(),"Ball::Update() -> Use the Be::Time version instead.");
    *in = Vector3(0.0f,0.0f,0.0f);
    return in;
}


Vector3* ClientBall::GetValueAt(
    Vector3* in,
    Be::Time time
    )
{
    if(!mPark)
    {
        // This ball is no longer in any ballpark
        // This should never happen really
        *in = Vector3(0.0f, -std::numeric_limits<float>::max(), 0.0f);
        //CCP_LOGERR_CH( s_ch,"Ball:%I64d no longer in ballpark, GetValueAt(Vector3)",  mId);
        RegisterBallNotInPark();
        return in;
    }

    Ball *theEgo = mPark->GetEgo();
    if(this==theEgo)
    {
        *in = Vector3(0.0f, 0.0f, 0.0f);
        mCenterDist = mSurfaceDist = 0.0;
        return in;
    }
    if(!theEgo)
    {
        // The ballpark no longer has an ego ball
        // This should never happen really
        *in = Vector3(0.0f, -std::numeric_limits<float>::max(), 0.0f);
        CCP_LOGERR_CH( s_ch,"Ball: No valid ego, GetValueAt(Vector3)");
        return in;
    }

    Vector3d reference(0.0, 0.0, 0.0);
    mPark->GetReferencePoint(&reference, time);

    if(!isFree)
    {
        // A fixed ball doesn't need calculation. Just use its latest value
        *in = (mNewPos-reference).AsVector3();

        if(time - mPosUpdateTime > 10000000)
        {
            mCenterDist = D3DXVec3Length((D3DXVECTOR3*)in);
            mSurfaceDist = mCenterDist - mRadius - theEgo->mRadius;
            // Handsoft: 337897 -  the recent changes of scene->scene2 
            // seems to have changed the call order and this gets called on
            // fixed balls before the update does. this has the effect of making 
            // the update think time is running backwards. 
            // as this is short cricketing the update for a fixed ball
            // we want the timestamp to be the current shifted time.
            // this will cause InterpolatedPosition to early out.
            mPosUpdateTime = GetShiftedTime(time);
        }

        if(isGlobal)
        {
            in->x /= mPark->mSomeWeirdHackToFixSomething;
            in->y /= mPark->mSomeWeirdHackToFixSomething;
            in->z /= mPark->mSomeWeirdHackToFixSomething;
        }

        return in;
    }
    Vector3d ip(0.0,0.0,0.0);

    InterpolatedPosition(&ip, time);

    *in = (ip-reference).AsVector3();

    if(isGlobal)
    {
        in->x /= mPark->mSomeWeirdHackToFixSomething;
        in->y /= mPark->mSomeWeirdHackToFixSomething;
        in->z /= mPark->mSomeWeirdHackToFixSomething;
    }

    return in;
}


Vector3* ClientBall::GetValueAt(
    Vector3* in,
    double time
    )
{
    return GetValueAt(in,mPosUpdateTime);
}


Vector3* ClientBall::GetValueDotAt(
    Vector3* in,
    Be::Time time
    )
{
    if(!mPark)
    {
        // This ball is no longer in any ballpark
        // This should never happen really
        //CCP_LOGERR_CH( s_ch,"Ball:%I64d no longer in ballpark, GetValueDotAt(Vector3)",  mId);
        RegisterBallNotInPark();
        *in = Vector3(0.0f, 0.0f, 0.0f);
        return in;
    }
    Vector3d yada(0.0,0.0,0.0);
    InterpolatedPosition(&yada, time);

    *in = mLastVel.AsVector3();

    return in;
}

Vector3* ClientBall::GetValueDotAt(
    Vector3* in,
    double time
    )
{
    return in;
}


Vector3* ClientBall::GetValueDoubleDotAt(
    Vector3* in,
    Be::Time time
    )
{
    if(!mPark)
    {
        // This ball is no longer in any ballpark
        // This should never happen really
        //CCP_LOGERR_CH( s_ch,"Ball:%I64d no longer in ballpark, GetValueDoubleDotAt(Vector3)",  mId);
        RegisterBallNotInPark();
        *in = Vector3(0.0f, 0.0f, 0.0f);
        return in;
    }

    *in = ((mNewVel-mOldVel)/(mPark->dt)).AsVector3();

    return in;
}


Vector3* ClientBall::GetValueDoubleDotAt(
    Vector3* in,
    double time
    )
{
    return in;
}

//---------------------------------------------------------------------------------------
// INotify interface methods
//---------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------
// INotify::OnModified
//---------------------------------------------------------------------------------------

bool Ball::OnModified(
    Be::Var* value
    )
{
    if(!mPark)
        return true;

    if((Be::Var*)&mGoto.x == value)
    {
        mPark->GotoPoint(mId,mGoto.x,mGoto.y,mGoto.z);
    }
    else if((Be::Var*)&mGoto.y == value)
    {
        mPark->GotoPoint(mId,mGoto.x,mGoto.y,mGoto.z);
    }
    else if((Be::Var*)&mGoto.z == value)
    {
        mPark->GotoPoint(mId,mGoto.x,mGoto.y,mGoto.z);
    }
    else if((Be::Var*)&mNewPos.x == value)
    {
        mPark->SetBallPosition(mId,mNewPos.x,mNewPos.y,mNewPos.z);
    }
    else if((Be::Var*)&mNewPos.y == value)
    {
        mPark->SetBallPosition(mId,mNewPos.x,mNewPos.y,mNewPos.z);
    }
    else if((Be::Var*)&mNewPos.z == value)
    {
        mPark->SetBallPosition(mId,mNewPos.x,mNewPos.y,mNewPos.z);
    }
    else if((Be::Var*)&mNewVel.x == value)
    {
        mPark->SetBallVelocity(mId,mNewVel.x,mNewVel.y,mNewVel.z);
    }
    else if((Be::Var*)&mNewVel.y == value)
    {
        mPark->SetBallVelocity(mId,mNewVel.x,mNewVel.y,mNewVel.z);
    }
    else if((Be::Var*)&mNewVel.z == value)
    {
        mPark->SetBallVelocity(mId,mNewVel.x,mNewVel.y,mNewVel.z);
    }
    else if((Be::Var*)&mRadius == value)
    {
        mPark->SetBallRadius(mId,mRadius);
    }
    else if((Be::Var*)&mMaxVel == value)
    {
        mPark->SetMaxSpeed(mId,mMaxVel);
    }
    else if((Be::Var*)&mMass == value)
    {
        mPark->SetBallMass(mId,mMass);
    }
    else if((Be::Var*)&isFree == value)
    {
        mPark->SetBallFree(mId,isFree);
    }else if((Be::Var*)&isMassive == value)
    {
        mPark->SetBallMassive(mId,isMassive);
    }
    else if((Be::Var*)&isGlobal == value)
    {
        mPark->SetBallGlobal(mId,isGlobal);
    }
    else if((Be::Var*)&mAgility == value)
    {
        mPark->SetBallAgility(mId,mAgility);
    }
    else if((Be::Var*)&mSpeedFraction == value)
    {
        mPark->SetSpeedFraction(mId,mSpeedFraction);
    }
    else if((Be::Var*)&mHarmonic == value)
    {
        mPark->SetBallHarmonic(mId, mHarmonic, mCorporationID, mAllianceID, 0);
    }
    else if((Be::Var*)&mCorporationID == value)
    {
        mPark->SetBallHarmonic(mId, mHarmonic, mCorporationID, mAllianceID, 0);
    }
    else if((Be::Var*)&mAllianceID == value)
    {
        mPark->SetBallHarmonic(mId, mHarmonic, mCorporationID, mAllianceID, 0);
    }

    return true;
}


void Ball::AddMiniBalls()
{
    if(isFree)
	{
        return; // Don't want to add miniballs to a free ball
	}
    
    ssize_t miniballSize = mMiniBalls.GetSize();

    if(miniballSize > 0)
        CCP_LOG_CH( s_ch,"Adding %d miniballs", miniballSize);

    for(ssize_t i=0 ; i < miniballSize ; i++)
    {
        MiniBall *mb = (MiniBall *)(void *)(mMiniBalls.GetAt(i));
        AddActualMiniball(mb);
    }

}

void Ball::RemoveMiniBalls()
{
    ssize_t miniballSize = mMiniBalls.GetSize();
    if(miniballSize > 0)
        CCP_LOG_CH( s_ch,"Removing %d miniballs", miniballSize);

    for(ssize_t i=0 ; i < miniballSize ; i++)
    {
        MiniBall *mb = (MiniBall *)(void *)(mMiniBalls.GetAt(i));
        mPark->RemoveBall(mb->mId);
    }
}

void Ball::AddActualMiniball(MiniBall *b)
{
    if(isFree)
	{
        return;
	}
	
	if(!mPark)
	{
		RegisterBallNotInPark();
		return;
	}

    ID theId = -1;

    Ball *ball = mPark->AddBall(-1, mMass, b->mRadius, 0.0f,
                0, 0, 1, 0, 0,
                mNewPos.x + b->mPos.x, mNewPos.y + b->mPos.y, mNewPos.z + b->mPos.z,
                0.0, 0.0, 0.0,
                mAgility,
                mSpeedFraction
                );

    b->mId = ball->mId;
    ball->mOwnerId = mId;
    ball->SetMode(DSTBALL_MINIBALL);

}

void Ball::AddMiniBall(
    double x,
    double y,
    double z,
    float r
    )
{
    MiniBall* b = new OMiniBall;
    b->mPos.x = x;
    b->mPos.y = y;
    b->mPos.z = z;
    b->mRadius = r;
    mMiniBalls.Insert(-1, b);

    if(!isFree)
    {
        // Add the miniball as a bona fide ball
        AddActualMiniball(b);
    }

}

void Ball::AddMiniCapsules()
{
	if(isFree)
	{
		return; // Don't want to add minicapsules to a free ball
	}

	ssize_t minicapsuleSize = mMiniCapsules.GetSize();

	if(minicapsuleSize > 0)
		CCP_LOG_CH( s_ch,"Adding %d minicapsules", minicapsuleSize);

	for(ssize_t i=0 ; i < minicapsuleSize ; i++)
	{
		MiniCapsule *mc = (MiniCapsule *)(void *)(mMiniCapsules.GetAt(i));
		AddActualMinicapsule(mc);
	}
}

void Ball::RemoveMiniCapsules()
{
	ssize_t miniCapsuleSize = mMiniCapsules.GetSize();
	if(miniCapsuleSize > 0)
		CCP_LOG_CH( s_ch,"Removing %d minicapsules", miniCapsuleSize);

	for(ssize_t i=0 ; i < miniCapsuleSize ; i++)
	{
		MiniCapsule *mb = (MiniCapsule *)(void *)(mMiniCapsules.GetAt(i));
		mPark->RemoveCapsule(mb->mId);
	}
}

void Ball::AddActualMinicapsule(MiniCapsule* c)
{
	if(isFree)
	{
		return;
	}

	if(!mPark)
	{
		RegisterBallNotInPark();
		return;
	}

	ID theId = -1;

	Capsule *capsule = mPark->AddCapsule(
		-1, 
		mNewPos.x + c->mHemisphereA.x, mNewPos.y + c->mHemisphereA.y, mNewPos.z + c->mHemisphereA.z, 
		mNewPos.x + c->mHemisphereB.x, mNewPos.y + c->mHemisphereB.y, mNewPos.z + c->mHemisphereB.z, 
		c->mRadius);

	c->mId = capsule->mId;
	//capsule->mOwnerId = mId;
}

void Ball::AddMiniCapsule(
	double ax,
	double ay,
	double az,
	double bx,
	double by,
	double bz,
	float r
	)
{
	MiniCapsule* c = new OMiniCapsule;
	c->mHemisphereA.x = ax;
	c->mHemisphereA.y = ay;
	c->mHemisphereA.z = az;
	c->mHemisphereB.x = bx;
	c->mHemisphereB.y = by;
	c->mHemisphereB.z = bz;
	c->mRadius = r;
	mMiniCapsules.Insert(-1, c);

	if(!isFree)
	{
		AddActualMinicapsule(c);
	}
}


void Ball::AddMiniBox(
	Vector3d corner,
	Vector3d localX,
	Vector3d localY,
	Vector3d localZ
	)
{
	MiniBox* b = new OMiniBox;
	b->mCorner = corner;
	b->mLocalX = localX;
	b->mLocalY = localY;
	b->mLocalZ = localZ;

	mMiniBoxes.Insert(-1, b);

	if(!isFree)
	{
		// Add the collision object into the park
		AddActualMiniBox(b);
	}

}

void Ball::AddMiniBoxes()
{
	if(isFree)
	{
		return; // Don't want to add minis to a free ball
	}

	ssize_t miniboxSize = mMiniBoxes.GetSize();

	if(miniboxSize > 0)
		CCP_LOG_CH( s_ch,"Adding %d miniboxes", miniboxSize);

	for(ssize_t i=0 ; i < miniboxSize ; i++)
	{
		MiniBox *mb = (MiniBox*)(void *)(mMiniBoxes.GetAt(i));
		AddActualMiniBox(mb);
	}
}

void Ball::RemoveMiniBoxes()
{
	ssize_t miniBoxSize = mMiniBoxes.GetSize();
	if(miniBoxSize > 0)
		CCP_LOG_CH( s_ch,"Removing %d miniboxes", miniBoxSize);

	for(ssize_t i=0 ; i < miniBoxSize ; i++)
	{
		MiniBox *mb = (MiniBox *)(void *)(mMiniBoxes.GetAt(i));
		mPark->RemoveOrientedBox(mb->mId);
	}
}

void Ball::AddActualMiniBox(MiniBox* b)
{
	if(isFree)
	{
		return;
	}

	if(!mPark)
	{
		RegisterBallNotInPark();
		return;
	}

	ID theId = -1;

	OrientedBox *box = mPark->AddOrientedBox(
		-1, 
		b->mCorner.x + mNewPos.x, b->mCorner.y + mNewPos.y, b->mCorner.z + mNewPos.z,
		b->mLocalX.x, b->mLocalX.y, b->mLocalX.z,
		b->mLocalY.x, b->mLocalY.y, b->mLocalY.z,
		b->mLocalZ.x, b->mLocalZ.y, b->mLocalZ.z);

	b->mId = box->mId;
}


Vector3d* ClientBall::GetReferencePoint( Vector3d* out, Be::Time time )
{
    if( mPark )
    {
        mPark->GetReferencePoint( out, time );
    }
    return out;
}

//---------------------------------------------------------------------------------------
// Python thunkers
//---------------------------------------------------------------------------------------

PyObject* Ball::Py__init__(
    PyObject* args
    )
{
    if (!PyArg_ParseTuple(args, "|L", &mId))
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

void ClientBall::DispatchPartition()
{
    for(int i =0; i<27;i++)
    {
        PyList_SET_ITEM(mBoxList, i, PyInt_FromLong(mActiveBoxes[i]));
    }

    if (!PyOS->SendEvent(
        (IBall*)this, "Destiny::Ball::DoPartition",
        "DoPartition", NULL,
        "fffffffO",
        mMyBox->lo[0] - mLastPos.x,
        mMyBox->lo[1] - mLastPos.y,
        mMyBox->lo[2] - mLastPos.z,
        mMyBox->hi[0] - mLastPos.x,
        mMyBox->hi[1] - mLastPos.y,
        mMyBox->hi[2] - mLastPos.z,
        mRadius + mPark->dt*mNewVel.Length(),
        mBoxList
        ))
    {
        PyOS->PyError();
    }
}

//---------------------------------------------------------------------------------------
// GetPartitionBoxes
//---------------------------------------------------------------------------------------
PyObject* ClientBall::PyGetPartitionBoxes(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PyObject* boxes = PyList_New(27);

    for(int i =0; i<27;i++)
    {
        PyList_SET_ITEM(boxes, i, PyInt_FromLong(mActiveBoxes[i]));
    }

    // this is what we are returing (lox, loy, loz,hix,hiy,hiz, r, [boxinfo])
    PyObject *ret =  Py_BuildValue("fffffffO", mMyBox->lo[0]-mNewPos.x,mMyBox->lo[1]-mNewPos.y,mMyBox->lo[2]-mNewPos.z, mMyBox->hi[0]-mNewPos.x,mMyBox->hi[1]-mNewPos.y,mMyBox->hi[2]-mNewPos.z, mRadius + mPark->dt*mNewVel.Length(), boxes);
    Py_DECREF(boxes);
    return ret;
}

//---------------------------------------------------------------------------------------
// Physical impact methods
//---------------------------------------------------------------------------------------
void ClientBall::ApplyImpulsiveForceAtPosition( const Vector3 &impactForce, const Vector3 &pos )
{
	if( mInvInertiaTensor < 0.0f )
	{
		mInvInertiaTensor = 5.0f / ( 2.0f * (float)mMass * this->GetBoundingRadius() );
	}

	D3DXVECTOR3 axis;
	D3DXVECTOR3 posFromCenter = D3DXVECTOR3( pos.x, pos.y, pos.z );
	D3DXVECTOR3 force = D3DXVECTOR3( impactForce.x, impactForce.y, impactForce.z );
	D3DXVec3Cross( &axis, &posFromCenter, &force );
	D3DXVec3Scale( &axis, &axis, mInvInertiaTensor );
	
	D3DXQUATERNION newVelocityAroundAxis;
	// Apply the acceleration for 0.05 seconds to get the velocity
	float angularVelocity = D3DXVec3Length( &axis ) * 0.05f;
	
	// Early exit
	if( angularVelocity < mMinimumAngularVelocity )
	{
		return;
	}

	D3DXQuaternionRotationAxis( &newVelocityAroundAxis, &axis, std::min( angularVelocity, mMaxAngularVelocity) );
	
	// Add the new velocity to the velocity goal
	D3DXQuaternionMultiply( reinterpret_cast<D3DXQUATERNION*>( &mImpactVelocityGoal ), reinterpret_cast<D3DXQUATERNION*>( &mImpactVelocityGoal), &newVelocityAroundAxis );
	
	if( mImpactVelocityGoal.w != 0.0f )
	{
		// Cap the velocity to the max
		float angularVelocity;
		D3DXQuaternionToAxisAngle( reinterpret_cast<D3DXQUATERNION*>( &mImpactVelocityGoal ), &axis, &angularVelocity);
		D3DXQuaternionRotationAxis( reinterpret_cast<D3DXQUATERNION*>( &mImpactVelocityGoal ), &axis, std::min( angularVelocity, mMaxAngularVelocity) );
	} 

	mCurrentVelocityInfluence = 1.0f;
	mProcessingImpact = true;	
	mImpactVelocityAtImpact = mImpactVelocity;
}

void ClientBall::ProcessImpact(float dt)
{
	D3DXQUATERNION qi = D3DXQUATERNION( 0.0f, 0.0f, 0.0f, 1.0f );
	
	D3DXVECTOR3 rotationAxis;
	float rotationAngle;
	
	// Velocity going up
	if( mCurrentVelocityInfluence > 0.0f )
	{
		// Move the current impact velocity to the goal
		float deterioration = mSpeedModifier * mInvInertiaTensor * (float)mMass * (dt * dt) * mMaxVel;
		deterioration = std::min( 0.1f, std::max( deterioration, 0.002f ) ); // limit the deterioration
		mCurrentVelocityInfluence = std::max( 0.0f, mCurrentVelocityInfluence - deterioration );
		float influence = mCurrentVelocityInfluence * mCurrentVelocityInfluence;

		D3DXQuaternionSlerp( reinterpret_cast<D3DXQUATERNION*>( &mImpactVelocity ), 
			reinterpret_cast<D3DXQUATERNION*>( &mImpactVelocityGoal ), 
			reinterpret_cast<D3DXQUATERNION*>( &mImpactVelocityAtImpact ), 
			influence );

		ApplyAngularVelocityToRotation( dt );

		mVelocityAtVelocityPeak = mImpactVelocity;
		mVelocityGoalAtVelocityPeak = mImpactVelocityGoal;
		mRotationAtVelocityPeak = mImpactRotation;
		mImpactRotationDeterioration = 1.0f;
	}

	// Counter rotation
	float deterioration = mInvInertiaTensor * (float)mMass * (dt * dt) * mMaxVel;
	deterioration = std::min(dt, std::max(deterioration, 0.002f)); // limit the deterioration
	
	mImpactRotationDeterioration = std::max(0.0f, mImpactRotationDeterioration - deterioration);
	float influence = mImpactRotationDeterioration * mImpactRotationDeterioration;

	D3DXQuaternionSlerp( reinterpret_cast<D3DXQUATERNION*>( &mImpactVelocityGoal ), &qi, reinterpret_cast<D3DXQUATERNION*>( &mVelocityGoalAtVelocityPeak ), influence );	
	D3DXQuaternionSlerp( reinterpret_cast<D3DXQUATERNION*>( &mImpactVelocity ), &qi, reinterpret_cast<D3DXQUATERNION*>( &mVelocityAtVelocityPeak ), influence );
	D3DXQuaternionSlerp( reinterpret_cast<D3DXQUATERNION*>( &mImpactRotation ), &qi, reinterpret_cast<D3DXQUATERNION*>( &mRotationAtVelocityPeak ), influence );
	
	mProcessingImpact = mImpactRotationDeterioration != 0.0f;

	D3DXQuaternionNormalize( reinterpret_cast<D3DXQUATERNION*>( &mImpactRotation ), reinterpret_cast<D3DXQUATERNION*>( &mImpactRotation ) );
	D3DXQuaternionToAxisAngle( reinterpret_cast<D3DXQUATERNION*>( &mImpactRotation ), &rotationAxis, &rotationAngle );
	
	// Cap the angle to the maximum
	rotationAngle = rotationAngle*mMaxAngle / ( rotationAngle + mMaxAngle );
	
	// Apply the rotation around the axis axis so we get the new rotation
	D3DXQuaternionRotationAxis( reinterpret_cast<D3DXQUATERNION*>( &mImpactRotation ), &rotationAxis, rotationAngle );	
	D3DXQuaternionNormalize( reinterpret_cast<D3DXQUATERNION*>( &mImpactRotation ), reinterpret_cast<D3DXQUATERNION*>( &mImpactRotation ) );
	D3DXQuaternionMultiply( reinterpret_cast<D3DXQUATERNION*>( &mLastRot ), reinterpret_cast<D3DXQUATERNION*>( &mLastRot ), reinterpret_cast<D3DXQUATERNION*>( &mImpactRotation ) );
}

void ClientBall::ApplyAngularVelocityToRotation( float dt )
{
	D3DXQUATERNION deltaAngle = D3DXQUATERNION( 0.0f, 0.0f, 0.0f, 1.0f );
	D3DXQuaternionNormalize(reinterpret_cast<D3DXQUATERNION*>( &mImpactVelocity ), reinterpret_cast<D3DXQUATERNION*>( &mImpactVelocity ));
	D3DXVECTOR3 velocityAxis;
	float rotationVelocity;
	if( mImpactVelocity.w != 1.0f )
	{
		// Get the current velocity axis and velocity
		D3DXQuaternionToAxisAngle( reinterpret_cast<D3DXQUATERNION*>( &mImpactVelocity ), &velocityAxis, &rotationVelocity );

		// Apply the angular velocity around the velocity axis so we get the new angle
		D3DXQuaternionRotationAxis(&deltaAngle, &velocityAxis, rotationVelocity * dt );
		// Add the new angle to the current rotation
		D3DXQuaternionMultiply( reinterpret_cast<D3DXQUATERNION*>( &mImpactRotation ), reinterpret_cast<D3DXQUATERNION*>( &mImpactRotation ), &deltaAngle);			
		D3DXQuaternionNormalize(reinterpret_cast<D3DXQUATERNION*>( &mImpactRotation ), reinterpret_cast<D3DXQUATERNION*>( &mImpactRotation ));	
	}
}

//---------------------------------------------------------------------------------------
// TestingPythonMethod
//---------------------------------------------------------------------------------------
PyObject* Ball::PyAddMiniBall(PyObject* args)
{
    double x,y,z;
    float r;

    if (!PyArg_ParseTuple(args, "dddf", &x, &y, &z, &r))
        return NULL;

    AddMiniBall(x,y,z,r);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Ball::PyAddMiniBox(PyObject* args)
{
	double c0, c1, c2, x0, x1, x2, y0, y1, y2, z0, z1, z2;

	if (!PyArg_ParseTuple(args, "dddddddddddd", 
		&c0, &c1, &c2, 
		&x0, &x1, &x2, 
		&y0, &y1, &y2, 
		&z0, &z1, &z2))
	{
		return NULL;
	}

	AddMiniBox(Vector3d(c0, c1, c2),
		Vector3d(x0, x1, x2),
		Vector3d(y0, y1, y2),
		Vector3d(z0, z1, z2));

	Py_INCREF(Py_None);
	return Py_None;
}


void Ball::GetRotatedVector(Vector3& vec)
{
    D3DXMATRIX mat;

    D3DXMatrixRotationYawPitchRoll(&mat, float(mNewYaw), float(mNewPitch), float(mNewRoll));
    D3DXVec3TransformCoord((D3DXVECTOR3*)&vec, (D3DXVECTOR3*)&vec, &mat);
}

//---------------------------------------------------------------------------------------
// GetAtVector
//---------------------------------------------------------------------------------------
PyObject* Ball::PyGetRotatedVector(PyObject* args)
{
    PyObject *list;
    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &list))
        return NULL;

    Vector3 vec;

    int i;
    PyObject *val;
    for(i=0; i < 3; i++)
    {
        val = PyList_GetItem(list, i);
        if(!val)
        {
            CCP_LOGERR_CH( s_ch,"GetRotatedVector: Illegal parameters");
            return NULL;
        }

        vec[i] = float(PyFloat_AS_DOUBLE(val));
    }

    GetRotatedVector(vec);

    PyList_SET_ITEM(list, 0, PyFloat_FromDouble(vec[0]));
    PyList_SET_ITEM(list, 1, PyFloat_FromDouble(vec[1]));
    PyList_SET_ITEM(list, 2, PyFloat_FromDouble(vec[2]));

    Py_INCREF(list);
    return list;
}

//---------------------------------------------------------------------------------------
// Ball::PyAddProximitySensor
//---------------------------------------------------------------------------------------
PyObject* Ball::PyAddProximitySensor(
    PyObject* args
    )
{
    float range;
    double period = 2.0;
    int shuffle = 0;
    char onlyInteractives = 0;

    if (!PyArg_ParseTuple(args, "f|dib",
        &range,
        &period,
        &shuffle,
        &onlyInteractives
        ))
        return NULL;

    AddProximitySensor(range, period, shuffle, !!onlyInteractives);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* Ball::PyReserveFormationSlot(
    PyObject* args
    )
{
    char slot = ReserveFormationSlot();

    return PyInt_FromLong(slot);
}

char Ball::ReserveFormationSlot()
{
    if(mFormationID == -1)
        return -1; // No formation assigned

    if(!mPark)
        return -1; // Really an error...

    if(mFormationID > int(mPark->mFormations.size()) - 1)
        return -1; // Illegal formationID

    size_t numberOfSlots = mPark->mFormations[mFormationID].size();

    if(numberOfSlots > 16)
        return -1; // Illegal number of formations

    char slot = -1;
    for(size_t i = 0; i < numberOfSlots ; i++)
    {
        if(mFormationSlots[i])
            continue;

        // Found an empty slot. Reserve it.
        mFormationSlots.set(i);
        slot = char(i);
        break;
    }

    CCP_LOG_CH( s_ch, "[%d] Reserving formation slot %d in ball %I64d", mPark->mCurrentTime, slot, mId);

    return slot;
}

PyObject* Ball::PyFreeFormationSlot(
    PyObject* args
    )
{
    int slot;

    if (!PyArg_ParseTuple(args, "i",
        &slot
        ))
        return NULL;

    FreeFormationSlot(slot);

    Py_INCREF(Py_None);
    return Py_None;
}
void Ball::FreeFormationSlot(size_t slot)
{
    if(mFormationID == -1)
	{
        return; // No formation assigned
	}

    if(!mPark)
	{
        return; // Really an error...
	}

    if(mFormationID > int(mPark->mFormations.size()) - 1)
	{
        return; // Illegal formationID
	}

    size_t numberOfSlots = mPark->mFormations[mFormationID].size();

    if(slot > numberOfSlots - 1)
	{
        return;
	}

    CCP_LOG_CH( s_ch, "[%d] Freeing formation slot %d in ball %I64d", mPark->mCurrentTime, slot, mId);

    mFormationSlots.reset(slot);
}


size_t BallPtrHasher::operator ()(const Ball* b) const
{
    return std::hash<ID>()(b->mId);
}

bool BallPtrHasher::operator () (const Ball* r, const Ball* l) const
{
    return r->mId == l->mId;
}


void Ball::InsertInBoxes(Box* box1, Box* top, long newBubbleId)
{
	int q;
	short mod[3];

	double inflatedRadius = GetInflatedRadius(mPark->dt);
	Box *box2;

	for(q=0;q<3;q++)
	{

		if( box1->lo[q] + inflatedRadius - mNewPos[q] > 0.0 )
		{
			// Add box to the left...
			mod[q] = -1;
		}
		else if ( box1->hi[q] - inflatedRadius - mNewPos[q] < 0.0f )
		{
			// add box  to the right
			mod[q] = 1;
		}
		else
		{
			// don't need to include any adjacent box
			mod[q] = 0;
		}
	}

	if(mPark->mProbe==mId)
		CCP_LOGWARN_CH( s_ch,"[%d] Old box is: %d - New box is: %d", mPark->mCurrentTime, mMyBox, box1);
	if(mPark->mProbe==mId)
		CCP_LOGWARN_CH( s_ch,"[%d] Mod state: [(%d,%d),(%d,%d),(%d,%d)]", mPark->mCurrentTime, mod[0], mod[0],mod[1], mod[1],mod[2], mod[2]);

	if(mMyBox && box1->mKey == mMyBox->mKey)
	{
		// We have the same base box as before. Check if the mods are the same as well.
		if(mod[0]==mMod[0] && mod[1]==mMod[1] && mod[2]==mMod[2])
		{
			if(mPark->mProbe==mId)
				CCP_LOGWARN_CH( s_ch,"[%d] Skipping stuff as they are the same", mPark->mCurrentTime);
			// Same mods. This is the same config. Bail out.
			return;
		}
	}

	// Now we know all the boxes we might have to add
	// We assume that when both modifiers are non-null, then the diagonal
	// box should be added as well. This is justified by the fact that it usually
	// very close anyway.

	// Let's run over all the boxes the ball currently thinks it is in and remove it from there
	// Note that this might invalidate the top box and its bubble, but we will restore it
	DeleteFromBoxes();

	mMod[0] = mod[0];
	mMod[1] = mod[1];
	mMod[2] = mod[2];

	// The box we had might well have been deleted. Create it again.
	box1 = mPark->mPartition->GetBox(inflatedRadius,mNewPos);

	int64_t ix,iy,iz;

	ix = box1->mKey.ix;
	iy = box1->mKey.iy;
	iz = box1->mKey.iz;
	long level = box1->mLevel;

	// First the box I am in
	InsertInBox(box1);

	if(mPark->isMaster)
	{
		// Restore the topmost bubbleID in case it got lost
		top = mPark->mPartition->GetTopmost(box1);
		top->mBubble = newBubbleId;
	}

	SetBoxActive(13, true);
	mMyBox = box1;

	if (level==0 && mRadius * 2 > mPark->mPartition->mLevelWidth[0])
	{
		// If the ball diameter is larger than the top-level box width, then the ball probably overlaps
		// adjacent boxes in both positive and negative directions on each axis.
		// (One example of this is the sentry-gun sensors around stations, which have approx 300km diameter.
		// compared to the top-level grid size of 245km)
		// This assumption is always valid for stations and stargates for example, because they are always
		// placed at the centre of a box.
		// Rather than handling this in a complex way, simply insert the ball to ALL adjacent boxes

		int x, y, z;
		for (x=-1; x<2; x++)
		{
			for (y=-1; y<2; y++)
			{
				for (z=-1; z<2; z++)
				{
					if (x==0 && y==0 && z==0)
					{
						// Skip the centre box where the ball actually is, as it has been specially handled above
						continue;
					}

					box2 = mPark->mPartition->GetBox(ix+x , iy+y , iz+z, 0);
					InsertInBox(box2);
					SetBoxActive(13 + x + 3*y + 9*z, true);
				}
			}
		}
	}
	else
	{
		// Here we determine which neighbouring boxes the ball might overlap, and insert accordingly

		// Add box in the x dimension
		if(mod[0])
		{
			box2 = mPark->mPartition->GetBox( ix+mod[0] , iy , iz , level);
			InsertInBox(box2);
			SetBoxActive(13+mod[0], true);
		}

		// Add box in the y dimension
		if(mod[1])
		{
			box2 = mPark->mPartition->GetBox( ix , iy+mod[1] , iz , level );
			InsertInBox(box2);
			SetBoxActive(13+3*mod[1], true);
		}

		// Add box in the z dimension
		if(mod[2])
		{
			box2 = mPark->mPartition->GetBox( ix , iy , iz+mod[2] , level);
			InsertInBox(box2);
			SetBoxActive(13+9*mod[2], true);
		}

		// Add the corner boxes
		// xy corner
		if(mod[0] && mod[1])
		{
			box2 = mPark->mPartition->GetBox( ix+mod[0] , iy+mod[1] , iz , level);
			InsertInBox(box2);
			SetBoxActive(13+mod[0]+3*mod[1], true);
		}

		// xz corner
		if(mod[0] && mod[2])
		{
			box2 = mPark->mPartition->GetBox(ix+mod[0] , iy , iz+mod[2], level);
			InsertInBox(box2);
			SetBoxActive(13+mod[0]+9*mod[2], true);
		}

		// yz corner
		if(mod[1] && mod[2])
		{
			box2 = mPark->mPartition->GetBox(ix , iy+mod[1] , iz+mod[2], level);
			InsertInBox(box2);
			SetBoxActive(13+3*mod[1]+9*mod[2], true);
		}

		// xyz corner
		if(mod[0] && mod[1] && mod[2])
		{
			box2 = mPark->mPartition->GetBox(ix+mod[0] , iy+mod[1] , iz+mod[2], level);
			InsertInBox(box2);
			SetBoxActive(13+mod[0]+3*mod[1]+9*mod[2], true);
		}
	}
}