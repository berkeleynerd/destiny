/*
	*************************************************************************

	Ball.h

	Author:    Kjartan Pierre Emilsson
	Created:   Nov. 2001
	OS:        Win32
	Project:   Destiny

	Description:

		Destiny schmestiny sample object


	Dependencies:

		Blue

	(c) CCP 2000, 2001, 2002

	*************************************************************************
*/

#ifndef _BALL_H_
#define _BALL_H_
#include "Partitionable.h"

#include "include/IBall.h"
#include "include/IDstConstants.h"

#include "DstConstants.h"
#include "MiniBall.h"
#include "MiniCapsule.h"
#include "MiniBox.h"
#include "Quaternion.h"

struct Vector3d;
#include <trinity/Include/IEveReferencePoint.h>

#include <blue/include/blue.h>
#include <unordered_set>
#include <vector>
#include <bitset>

class Box;
class Ball;

#include "MapOfLongLongs.h"
#include "Hashers.h"

typedef std::unordered_set<Ball *, BallPtrHasher, BallPtrHasher> SetOfOrderedBalls;
typedef std::unordered_set<Ball *> SetOfBalls;

class ProximitySensor{
public:
	ProximitySensor(): range(0.0),period(2.0),elapsed(0.0),active(false), onlyInteractives(false) {};
	float range;
	double period;
	double elapsed;
	bool active;
	bool onlyInteractives;
	MapOfLongLongs balls;
};


/*------------------------------------------------------------------------
   A Ball is an object with 6 degrees of freedom (position and velocity)
   The movement of the ball is basically governed by a differential
   equation describing the movement of a solid through a viscous media.
   In the linear case, and given a constant thrust, this results in a
   maximum velocity, which can be specified.

   The ball senses other balls as solids which cannot be traversed, so
   any movement of the balls will avoid each other or be stuck.

   In special cases, the ball can be specified as fixed, in which case
   its position and velocity will not be updated. You can also specify
   wether the ball is massive, meaning that other objects can collide with
   it, and whether it is global, meaning that it will visible from all
   other balls.

   We then define 3 dynamical states, which alter the behaviour of the
   differential equation. These are the following:

   Goto Point: The ball will attempt to go to the point specified.

   Follow: The ball will attempt to follow another ball, while maintaining
           some minimum range to it.

   Stop: The ball will not try to go anywhere, eventually coming to a halt, but
         it can still be bumped by others.

  Apart from changes between these dynamical states, the behavior of a ball
  is fully deterministic.
------------------------------------------------------------------------*/
typedef std::unordered_set<Box *, BoxPtrHasher, BoxPtrHasher> SetOfOrderedBoxes;
typedef std::vector<Ball *> VectorOfBalls;
typedef int64_t ID;
typedef std::vector<ID> VectorOfIDs;
typedef std::unordered_set<ID> SetOfIDs;



class Ball :
	public INotify,
	public IBall,
	public Partitionable
{
friend class Ballpark;
friend class Partition;
public:
	Ball(IRoot* lockobj = NULL);
	~Ball();

	using IBall::Unlock;

	void RegisterBallNotInPark();
public:

	//--------------------------------------------------//
	// Distributed state of ball.  Try to group
	// so that it packs more tightly.
	//--------------------------------------------------//
    ID mFollowId; // the ball being followed for a DSTBALL_FOLLOW, DSTBALL_ORBIT and DSTBALL_MISSILE mode
    ID mOwnerId; // the ball that launched me for DSTBALL_MISSILE mode
    int64_t mHarmonic; // Harmonic value of the ball

    Ballpark *mPark; // pointer to ball owner
    Ball *mTrackingPtr; // Var for tracking a target and notification on range.
    Ball *mFollowPtr; // Pointer to ball being followed

	int mAllianceID;
    int mCorporationID;

    short mMod[3];

    char mFormationID;
    char isCloaked;  //  non-zero if ball is cloaked for others

    bool isFree; // true if ball can move
	bool isGlobal; // true if ball is visible by all
	bool isMassive; // true if ball is solid
	bool isSpaceJunk; // true if ball is just a myriad of your thoughs
    bool mWithinTrackingRange;
    bool mHasProximity;
	bool mWithinRange;
	bool mHandled;    // Flag used when processing balls.

	float mRadius; // radius of ball (meters)
	float mMaxVel; // the maximum velocity this ball can move (meters/second)
    float mAgility; // the maneuverability of the ball
	float mSpeedFraction; // Current cruise velocity as a fraction of 'mMaxVel'
    float mFollowRange; // the range at which to follow ball

    double mMass; // mass of ball (kg)
    double mTimeFactor; // Constant used integration
    double mLastCollision; // time of last collision during multiple collisions in same time step
    double mNotificationRange;
    double mNewYaw; // current yaw of ball
	double mOldYaw; // yaw of ball one tick interval ago
	double mNewPitch; // current pitch of ball
	double mOldPitch; // pitch of ball one tick interval ago
	double mNewRoll; // current roll of ball
	double mOldRoll; // roll of ball one tick interval ago
	double mNewRollSpeed; // current roll of ball
	double mOldRollSpeed; // roll of ball one tick interval ago
	double mYawDelta; // Correctly modulo adjusted yaw delta
    double mTrackingRange; // Var for tracking a target and notification on range.
	
	std::bitset<16> mFormationSlots;
	
	Vector3d mNewVel; // current velocity of ball
	Vector3d mOldVel; // velocity of ball one tick interval ago
    Vector3d mGoto; // current goto destination of ball for a DSTBALL_GOTO mode
    // work variables used in integration
	Vector3d mLastG;
	Vector3d mLastC;


	DSTBALLMODE mMode; // the current dynamical state of the ball	

	typedef RootParentLock<BlueList<MiniBall> > MiniBallList;
	typedef RootParentLock<BlueList<MiniBox> > MiniBoxList;
	typedef RootParentLock<BlueList<MiniCapsule> > MiniCapsuleList;
	MiniBallList mMiniBalls;
	MiniBoxList mMiniBoxes;
	MiniCapsuleList mMiniCapsules;
	//--------------------------------------------------//
	// Dynamic state of ball
	//--------------------------------------------------//

	Be::Time mNewTime; // timestamp of new destiny state
	Be::Time mOldTime; // timestamp of old destiny state

	SetOfIDs mFollowers; // IDs of balls following this ball
	SetOfOrderedBoxes mBoxes; // Boxes that this ball currently intersects, ordered by Box 'key'
	VectorOfIDs mCollisions; // Objects that have collided with me in the last time step
	
	bool mActiveBoxes[27];

	ProximitySensor mSensor; // List of proximity sensors associated with this ball

	//--------------------------------------------------//
	// Utility Functions
	//--------------------------------------------------//

	//////////////////////////////////////////////////////////////////////////////
	// sensor = AddProximitySensor(range)
	// pre: 'range' > 0.0
	// post: A proximity sensor with the given range and period is now active
	//////////////////////////////////////////////////////////////////////////////
	long AddProximitySensor(float range, double period, int shuffle, bool onlyInteractives=false);

	//////////////////////////////////////////////////////////////////////////////
	// RemoveProximitySensor(sensor)
	// pre: none
	// post: The current proximity sensor has been deactivated
	//////////////////////////////////////////////////////////////////////////////
	void RemoveProximitySensor();

	//////////////////////////////////////////////////////////////////////////////
	// CheckForProximity(ball)
	// pre: 'ball' is a valid ball pointer, not equal to this
	// post: If the ball given has left or entered the range of any sensor
	//       associated with this ball, an event has been dispatched
    //
    // This uses the function, subfunction pattern.
	//////////////////////////////////////////////////////////////////////////////
	void CheckForProximity();
    void CheckForProximity_NotificationRange();
    void CheckForProximity_TargetTracking();
    void CheckForProximity_Sensor();

	//////////////////////////////////////////////////////////////////////////////
	// DeleteBallFromBoxes()
	// pre: none
	// post: remove ball from any intersecting boxes in the current space partition
	//////////////////////////////////////////////////////////////////////////////
	void DeleteFromBoxes();

	//////////////////////////////////////////////////////////////////////////////
	// ClearBoxes()
	// pre: none
	// post: All boxes have been cleared away, and their state is false
	//////////////////////////////////////////////////////////////////////////////
	void ClearBoxes();

	//////////////////////////////////////////////////////////////////////////////
	// InsertInBox()
	// pre: none
	// post: The ball and box will have references to each other
	//////////////////////////////////////////////////////////////////////////////
	void InsertInBox(Box* box);

	//////////////////////////////////////////////////////////////////////////////
	// InsertInBox()
	// pre: none
	// post: The ball will be a member of the boxes that it intersects during 
	//       this and the previous timestep
	//////////////////////////////////////////////////////////////////////////////
	void InsertInBoxes(Box* box1, Box* top, long newBubbleId);

	//////////////////////////////////////////////////////////////////////////////
	// GetInflatedRadius()
	// Get the radius of the largest sphere the object can occupy during the time step
	//////////////////////////////////////////////////////////////////////////////
	double GetInflatedRadius(double dt);

	//////////////////////////////////////////////////////////////////////////////
	// GetInflatedRadius()
	// Get the radius of the ball
	//////////////////////////////////////////////////////////////////////////////
	float GetBoundingRadius();

	//////////////////////////////////////////////////////////////////////////////
	// SetBoxActive()
	// Update the activity status of the box
	//////////////////////////////////////////////////////////////////////////////
	void SetBoxActive(int boxId, bool isActive);

	//////////////////////////////////////////////////////////////////////////////
	// CalculateYawPitchRoll()
	// pre: none
	// post: Updates the current yaw, pitch and roll of ball
	//////////////////////////////////////////////////////////////////////////////
	virtual void CalculateYawPitchRoll(bool snap=false);

	//////////////////////////////////////////////////////////////////////////////
	// DispatchCollisions()
	// pre: none
	// post: Generates deco messages for collisions
	//////////////////////////////////////////////////////////////////////////////
	void DispatchCollisions();

	//////////////////////////////////////////////////////////////////////////////
	// UpdateProximityStatus()
	// pre: none
	// post: Udpates the proximity flags
	//////////////////////////////////////////////////////////////////////////////
	void UpdateProximityStatus();

	char ReserveFormationSlot();
	void FreeFormationSlot(size_t slot);

	void SetMode(DSTBALLMODE mode);

public:

	// Add and remove all miniballs in one swoop
	void AddMiniBalls();
	void RemoveMiniBalls();
	void AddActualMiniball(MiniBall* b);
	void GetRotatedVector(Vector3&);

	void AddMiniBall(
		double x,
		double y,
		double z,
		float r
		);

	void AddMiniBoxes();
	void RemoveMiniBoxes();
	void AddActualMiniBox(MiniBox* b);
	void AddMiniBox(
		Vector3d corner, 
		Vector3d localX, 
		Vector3d localY, 
		Vector3d localZ
		);

	void AddMiniCapsules();
	void RemoveMiniCapsules();
	void AddActualMinicapsule(MiniCapsule* c);

	void AddMiniCapsule(
		double ax,
		double ay,
		double az,
		double bx,
		double by,
		double bz,
		float r
		);

	////////////////////////////////
	// INotify
	bool OnModified(
		Be::Var* vBall
		);
	
	////////////////////////////////
	// IBall

	void GetDistances(
		double *surfaceDist, double *centerDist
		) { return; };

public:
	/////////////////////////////////////////
	// Blue class info

	EXPOSE_TO_BLUE();

	PyObject* Py__init__( PyObject* args );
	PyObject* PyAddMiniBall( PyObject* args );
	PyObject* PyAddMiniBox( PyObject* args );
	PyObject* PyGetRotatedVector( PyObject* args );
	PyObject* PyAddProximitySensor( PyObject* args );
	PyObject* PyReserveFormationSlot( PyObject* args );
	PyObject* PyFreeFormationSlot( PyObject* args );

};


class ClientBall :
	public ITriVectorFunction,
	public ITriQuaternionFunction,
	public IEveReferencePoint,
	public Ball
{
friend class Ballpark;
friend class Partition;
public:
	EXPOSE_TO_BLUE();

	ClientBall(IRoot* lockobj = NULL);
	~ClientBall();

	using IBall::Unlock;
	
	double mLastYaw;
	double mLastPitch;
	double mLastRoll;
	double mYawSpeed;
	double mPitchSpeed;

	double mCenterDist;
	double mSurfaceDist;

	Be::Time mPosUpdateTime; // last time pos was interpolated
	Be::Time mRotUpdateTime; // last time rot was interpolated

	// Used in interpolations
	Quaternion mLastRot;
	Vector3d mLastPos;
	Vector3d mLastVel;
	Vector3d mDeltaPos;
	Vector3d mIntAcc;
	float mInvInertiaTensor;

	float mMaxAngularVelocity;
	float mMaxAngle;
	Quaternion mImpactRotation;
	float mCurrentVelocityInfluence;
	float mImpactRotationDeterioration;
	bool mProcessingImpact;
	Quaternion mImpactVelocityAtImpact;
	Quaternion mImpactVelocity;
	Quaternion mImpactVelocityGoal;
	Quaternion mRotationAtVelocityPeak;
	Quaternion mVelocityAtVelocityPeak;
	Quaternion mVelocityGoalAtVelocityPeak;
	float mSpeedModifier;
	float mMinimumAngularVelocity;

	double mElapsed;
	
	PyObject* mBoxList; // Used in DispatchPartition
	long mLastTick;
	bool mShowBoxes;

	//////////////////////////////////////////////////////////////////////////////
	// CalculateYawPitchRoll()
	// pre: none
	// post: Updates the current yaw, pitch and roll of ball
	//////////////////////////////////////////////////////////////////////////////
	void CalculateYawPitchRoll(bool snap=false);

	//////////////////////////////////////////////////////////////////////////////
	// GetDelta(delta,time)
	// pre: 'delta' is valid.
	// post: delta points to a 3D vector that tells how much this ball has moved
	//       since the last call to GetDelta()
	//////////////////////////////////////////////////////////////////////////////
	void GetDelta(Vector3 *in, Be::Time time);

	//////////////////////////////////////////////////////////////////////////////
	// InforceContinuity()
	// pre: none
	// post: Interpolation points are continuous
	//////////////////////////////////////////////////////////////////////////////
	void InforceContinuity();

	void DispatchPartition();

	float GetImpactRotation( float dt );
	void ProcessImpact( float dt );
	void ApplyImpulsiveForceAtPosition( const Vector3 &impulsiveForce, const Vector3 &pos );
	void ApplyAngularVelocityToRotation( float dt );

	////////////////////////////////
	// IBall

	void GetDistances(
		double *surfaceDist, double *centerDist
		) { *surfaceDist = mSurfaceDist;
		    *centerDist = mCenterDist;
			return;
		};

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveReferencePoint
	/////////////////////////////////////////////////////////////////////////////////////
	virtual	Vector3d* GetReferencePoint(Vector3d* out, Be::Time time);;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriFunction
	/////////////////////////////////////////////////////////////////////////////////////
	// This is a stub, so that destiny will compile following CL 73220
	// It is functionality only required for curveset support, so I'm not sure what the correct functionality
	// for a ball would be.
	virtual void UpdateValue( double time ) {};

    Be::Time GetShiftedTime(Be::Time time);
	/////////////////////////////////////////////////////////////////////////////////////
	// ITriQuaternionFunction
	/////////////////////////////////////////////////////////////////////////////////////
	Quaternion* Update(
		Quaternion* in,
		Be::Time time
		);

	Quaternion* Update(
		Quaternion* in,
		double time
		);

	Quaternion* GetValueAt(
		Quaternion* in,
		Be::Time time
		);

	Quaternion* GetValueAt(
		Quaternion* in,
		double time
		);

	Quaternion* GetValueDotAt(
		Quaternion* in,
		Be::Time time
		);

	Quaternion* GetValueDotAt(
		Quaternion* in,
		double time
		);

	Quaternion* GetValueDoubleDotAt(
		Quaternion* in,
		Be::Time time
		){return in;};

	Quaternion* GetValueDoubleDotAt(
		Quaternion* in,
		double time
		){return in;};

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriVectorFunction
	/////////////////////////////////////////////////////////////////////////////////////
	Vector3* Update(
		Vector3* in,
		Be::Time time
		);

	Vector3* Update(
		Vector3* in,
		double time
		);

	Vector3* GetValueAt(
		Vector3* in,
		Be::Time time
		);

	Vector3* GetValueAt(
		Vector3* in,
		double time
		);

	Vector3* GetValueDotAt(
		Vector3* in,
		Be::Time time
		);

	Vector3* GetValueDotAt(
		Vector3* in,
		double time
		);

	Vector3* GetValueDoubleDotAt(
		Vector3* in,
		Be::Time time
		);

	Vector3* GetValueDoubleDotAt(
		Vector3* in,
		double time
		);
	Vector3d* InterpolatedPosition(Vector3d* out, Be::Time);

	PyObject* PyGetPartitionBoxes ( PyObject* args );
};
#endif
