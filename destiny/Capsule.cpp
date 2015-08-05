#include "StdAfx.h"
#include "Capsule.h"


void Capsule::Initialize(ID theID, double ax, double ay, double az, double bx, double by, double bz, float radius)
{
	mId = theID;
	mHemisphereA.x = ax;
	mHemisphereA.y = ay;
	mHemisphereA.z = az;
	mHemisphereB.x = bx;
	mHemisphereB.y = by;
	mHemisphereB.z = bz;
	mRadius = radius;
	mOldPos = mNewPos = GetCenter();
	mAB = mHemisphereB - mHemisphereA; // From A to B
}

float Capsule::GetBoundingRadius()
{
	return (float)mAB.Length() / 2.0f + mRadius;
}

Vector3d Capsule::GetCenter()
{
	return mHemisphereA + mAB / 2.0f;
}

void Capsule::CollideWithBall(Ball* ball)
{
	if(!ball->isFree)
	{
		return;
	}
	Vector3d p0, p1, vp1;
	double m1 = ball->mMass * ball->mAgility;

	p0 = p1 = ball->mNewPos;
	vp1 = ball->mNewVel;

	// Get the destination point for the ball
	mPark->Integrate(p1, vp1, ball->mLastG, m1, mPark->mFriction, ball->mTimeFactor, mPark->dt);

	// Calculate if and when collision occurs
	float rad = ball->mRadius + mRadius;
	Vector3d d = vp1;
	Vector3d AO = p0 - mHemisphereA; // From A to the center of the ball
	double m = mAB * d / (mAB * mAB);
	double n = mAB * AO / (mAB * mAB);
	Vector3d Q = d - (mAB * m);
	Vector3d R = AO - (mAB * n);

	double a = Q * Q;
	double b = 2.0 * (Q * R);
	double c = R * R - rad*rad;
	Vector3d normal;

	double radSq = rad * rad;

	double s1,s2,s,time_of_impact;
	s = -1.0;
	time_of_impact = -1.0;

	if( a == 0.0 )
	{
		// If a is zero, the ray is parallel to the cylinder axis (AB).
		// Check for collision with the A side cap.
		double s = CollideTwoSpheres(p0, p1, mHemisphereA, mHemisphereA, rad);
		if( s != -1.0 && (time_of_impact == -1.0 || s < time_of_impact) )
		{
			time_of_impact = s;
			normal = mHemisphereA - p0;
		}


		// Check for collision with the B side cap.
		s = CollideTwoSpheres(p0, p1, mHemisphereB, mHemisphereB, rad);
		if( s != -1.0 && (time_of_impact == -1.0 || s < time_of_impact) )
		{
			time_of_impact = s;
			normal = mHemisphereB - p0;
		}
		ReactToCollision(ball, p1, vp1, m1, normal, time_of_impact);
		return;
	}

	if( !Quadratic(s1,s2, a, b, c) )
	{
		return;
	}

	if( s1 >= 0.0 && s1 <= 1.0 )
	{
		s = s1;
	}
	if( s2 >= 0.0 && s2 <= 1.0 && s2 < s1 )
	{
		s = s2;
	}
	else if( s == -1.0 )
	{	
		// We don't collide with the cylinder
		double abSq = mAB.LengthSq();

		// Are we already inside  the cylinder ? Sphere checks already determine if we are already inside the spheres.
		const double t = (p0 - mHemisphereA) * mAB / abSq;
		if( t >= 0.0 && t <= 1.0)
		{
			const Vector3d proj = mHemisphereA + t * mAB;
			Vector3d dist = p0 - proj;
			if( dist.LengthSq() < radSq )
			{
				// p0 was inside the cylinder
				time_of_impact = 0.0;
				normal = -dist;
				ReactToCollision(ball,p1, vp1, m1, normal, time_of_impact);
				return;
			}
		}
	}

	double t_k1 = (s1 * m) + n;  // Where on the scale from 0 to 1 does the first collision occur on the cylinder from A to B
	double t_k2 = (s2 * m) + n;  // Same for the second collision

	if( t_k1 < 0.0 )
	{
		// Check for collision with the A side cap.
		double s = CollideTwoSpheres(p0, p1, mHemisphereA, mHemisphereA, rad);
		if( s != -1.0 && (time_of_impact == -1.0 || s < time_of_impact) )
		{
			time_of_impact = s;
			normal = mHemisphereA - p0;
		}
	}
	else if( t_k1 > 1.0 )
	{
		// Check for collision with the B side cap.
		double s = CollideTwoSpheres(p0, p1, mHemisphereB, mHemisphereB, rad);
		if( s != -1.0 && (time_of_impact == -1.0 || s < time_of_impact) )
		{
			time_of_impact = s;
			normal = mHemisphereB - p0;
		}
	}
	else
	{	
		if( s != -1.0 && (time_of_impact == -1.0 || s < time_of_impact) )
		{
			time_of_impact = s;
			Vector3d proj_ab_ao = (AO * mAB / (mAB.LengthSq())) * mAB;
			normal = AO - proj_ab_ao;

			if( normal.LengthSq() < radSq )
			{
				time_of_impact = 0.0;
			}
		}
	}

	if( t_k2 < 0.0 )
	{
		// Check for collision with the A side cap.
		double s = CollideTwoSpheres(p0, p1, mHemisphereA, mHemisphereA, rad);
		if( s != -1.0 && (time_of_impact == -1.0 || s < time_of_impact) )
		{
			time_of_impact = s;
			normal = mHemisphereA - p0;
		}

	}
	else if( t_k2 > 1.0 )
	{
		// Check for collision with the B side cap.
		double s = CollideTwoSpheres(p0, p1, mHemisphereB, mHemisphereB, rad);
		if( s != -1.0 && (time_of_impact == -1.0 || s < time_of_impact) )
		{
			time_of_impact = s;
			normal = mHemisphereB - p0;
		}
	}
	else
	{	
		if( s != -1.0 && (time_of_impact == -1.0 || s < time_of_impact) )
		{
			time_of_impact = s;
			Vector3d proj_ab_ao = (AO * mAB / (mAB.LengthSq())) * mAB;
			normal = AO - proj_ab_ao;
			if( normal.LengthSq() < radSq )
			{
				time_of_impact = 0.0;
			}
		}
	}
	
	
	// We have impact. Now react.
	ReactToCollision(ball, p1, vp1, m1, normal, time_of_impact);
}

void Capsule::ReactToCollision(Ball* ball, Vector3d& integratedBallPosition, Vector3d& ballVelocity, double m1, Vector3d& normal, double timeOfImpact)
{
	if( timeOfImpact == -1.0 )
	{
		return;
	}
	double centerDistance = normal.Length();
	normal.Normalize();
	Vector3d acceleration;

	if( timeOfImpact > 0.0 )
	{
		// This is the simple case of someone hitting a fixed object. Speed is thus inverted along the radial direction
		// Now given that. I would be situated at:
		double v1 = ballVelocity*normal;
		ballVelocity -= 2.0*v1*normal;
		mPark->Integrate(integratedBallPosition, ballVelocity, ball->mLastG, m1, mPark->mFriction, ball->mTimeFactor, (1.0-timeOfImpact)*mPark->dt);
		// In order to get there, my acceleration needs to be:
		double k = mPark->mFriction;
		acceleration = -(-m1*ball->mLastG + ball->mTimeFactor*m1*ball->mLastG - ball->mTimeFactor*ball->mNewVel*k + ballVelocity*k)/m1/(ball->mTimeFactor-1.0);
	}
	else
	{
		double normalComp = ball->mLastG*normal;
		// Calculate the acceleration
		double tmp = 1.0/(m1+mPark->dt*mPark->mFriction);
		double dist = mRadius + ball->mRadius + 1.0 - centerDistance; // The distance that we need to move the ball to get it out.
		if( dist < 1.0 )
		{
			dist = 1.0;
		}
		acceleration = ( (-dist/(mPark->dt*tmp*m1) )*normal - ball->mNewVel)/mPark->dt -normalComp*normal;
	}
	Vector3d lastC = 0.85*acceleration;
	if(timeOfImpact==ball->mLastCollision)
	{
		// Use the stronger collision of the two
		if(lastC.LengthSq() > ball->mLastC.LengthSq())
		{
			ball->mLastC = lastC;
		}
	}
	else
	{
		ball->mLastC = lastC;
		ball->mLastCollision = timeOfImpact;
	}

	ball->mCollisions.push_back(mId);
}

void Capsule::InsertInBoxes(Box* box1, Box* top, long newBubbleId)
{
	Box *box2;

	if(mMyBox && box1->mKey == mMyBox->mKey)
	{
		// We have the same base box as before.
		return;
	}

	// Now we know all the boxes we might have to add
	// We assume that when both modifiers are non-null, then the diagonal
	// box should be added as well. This is justified by the fact that it usually
	// very close anyway.

	// Let's run over all the boxes the ball currently thinks it is in and remove it from there
	// Note that this might invalidate the top box and its bubble, but we will restore it
	DeleteFromBoxes();

	// The box we had might well have been deleted. Create it again.
	box1 = mPark->mPartition->GetBox(mRadius,mNewPos);
	Box* boxA = mPark->mPartition->GetBox(mRadius, mHemisphereA);

	int64_t ix,iy,iz;

	
	long level = boxA->mLevel;

	if(mPark->isMaster)
	{
		// Restore the topmost bubbleID in case it got lost
		top = mPark->mPartition->GetTopmost(box1);
		top->mBubble = newBubbleId;
	}

	mMyBox = box1;

	// Here we determine which neighbouring boxes the ball might overlap, and insert accordingly
	int q;
	short mod[3];
	Vector3d interpolatedPos = mHemisphereA;
	Vector3d direction = mHemisphereB - mHemisphereA;
	float length = (float)direction.Length();
	direction = direction.Normalize() * mRadius * 2.0f;
	while(length >= 0.0f)
	{
		boxA = mPark->mPartition->GetBox(mRadius, interpolatedPos);
		ix = boxA->mKey.ix;
		iy = boxA->mKey.iy;
		iz = boxA->mKey.iz;

		for(q=0;q<3;q++)
		{
			if( boxA->lo[q] + mRadius - interpolatedPos[q] > 0.0f )
			{
				// Add box to the left...
				mod[q] = -1;
			}
			else if ( boxA->hi[q] - mRadius - interpolatedPos[q] < 0.0f )
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
		
		box2 = mPark->mPartition->GetBox( ix, iy , iz, level);
		InsertInBox(box2);

		// Add box in the x dimension
		if(mod[0])
		{
			box2 = mPark->mPartition->GetBox( ix+mod[0] , iy , iz , level);
			InsertInBox(box2);
		}

		// Add box in the y dimension
		if(mod[1])
		{
			box2 = mPark->mPartition->GetBox( ix , iy+mod[1] , iz , level );
			InsertInBox(box2);
		}

		// Add box in the z dimension
		if(mod[2])
		{
			box2 = mPark->mPartition->GetBox( ix , iy , iz+mod[2] , level);
			InsertInBox(box2);
		}

		// Add the corner boxes
		// xy corner
		if(mod[0] && mod[1])
		{
			box2 = mPark->mPartition->GetBox( ix+mod[0] , iy+mod[1] , iz , level);
			InsertInBox(box2);
		}

		// xz corner
		if(mod[0] && mod[2])
		{
			box2 = mPark->mPartition->GetBox(ix+mod[0] , iy , iz+mod[2], level);
			InsertInBox(box2);
		}

		// yz corner
		if(mod[1] && mod[2])
		{
			box2 = mPark->mPartition->GetBox(ix , iy+mod[1] , iz+mod[2], level);
			InsertInBox(box2);
		}

		// xyz corner
		if(mod[0] && mod[1] && mod[2])
		{
			box2 = mPark->mPartition->GetBox(ix+mod[0] , iy+mod[1] , iz+mod[2], level);
			InsertInBox(box2);
		}
		
		interpolatedPos = interpolatedPos + direction;
		length -= mRadius * 2.0f;
	}
}