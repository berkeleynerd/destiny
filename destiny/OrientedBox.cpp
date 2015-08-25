#include "StdAfx.h"
#include "OrientedBox.h"

void OrientedBox::Initialize(
	ID theID, 
	double c0, double c1, double c2,
	double x0, double x1, double x2,
	double y0, double y1, double y2,
	double z0, double z1, double z2 )
{
	mId = theID;
	Vector3d corner(c0, c1, c2);
	Vector3d local_x(x0, x1, x2);
	Vector3d local_y(y0, y1, y2);
	Vector3d local_z(z0, z1, z2);

	m_boxShape = BoxShape(corner, local_x, local_y, local_z);
	mBoundingRadius = (float) m_boxShape.GetBoundingRadius();
	mOldPos = mNewPos = GetCenter();

	mAABB = AABB(m_boxShape.m_corners[CORNER_A], m_boxShape.m_corners[CORNER_A]);
	
	for(int i = CORNER_B; i < CORNER_COUNT; ++i)
	{
		mAABB.Update(m_boxShape.m_corners[i]);
	}
}

//StaticCollidable abstract functions
float OrientedBox::GetBoundingRadius()
{
	return (float) m_boxShape.GetBoundingRadius();
}

Vector3d OrientedBox::GetCenter()
{
	return m_boxShape.GetCenter();
}

void OrientedBox::CollideWithBall(Ball* ball)
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
	
	if( mAABB.CanExcludeCollision(p0, p1, ball->mRadius) )
	{
		return;
	}

	Vector3d collision_point;
	double collision_time;
	Vector3d normal;
	bool collision_exists = m_boxShape.CollideWithSphere(p0, ball->mRadius, p1 - p0, collision_point, collision_time, normal);
	if( collision_exists )
	{
		// We have impact. Now react.
		vp1 = ball->mNewVel;
		ReactToCollision(ball, p0, vp1, m1, normal, collision_time);
	}
}

void OrientedBox::ReactToCollision(Ball* ball, Vector3d& ballPosition, Vector3d& ballVelocity, double m1, Vector3d& normal, double timeOfImpact)
{
	if( timeOfImpact == -1.0 )
	{
		return;
	}

	double collisionDepth = normal.Length();
	normal.Normalize();
	Vector3d acceleration;

	if( timeOfImpact > 0.0 )
	{
		mPark->Integrate(ballPosition, ballVelocity, ball->mLastG, m1, mPark->mFriction, ball->mTimeFactor, timeOfImpact*mPark->dt);
		// This is the simple case of someone hitting a fixed object. Speed is thus inverted along the radial direction
		// Now given that. I would be situated at:
		double v1 = ballVelocity*normal;
		ballVelocity -= 2.0*v1*normal;
		
		mPark->Integrate(ballPosition, ballVelocity, ball->mLastG, m1, mPark->mFriction, ball->mTimeFactor, (1.0-timeOfImpact)*mPark->dt);
		// In order to get there, my acceleration needs to be:
		double k = mPark->mFriction;
		acceleration = -(-m1*ball->mLastG + ball->mTimeFactor*m1*ball->mLastG - ball->mTimeFactor*ball->mNewVel*k + ballVelocity*k)/m1/(ball->mTimeFactor-1.0);
	}
	else
	{
		// We are inside the box. Eject!
		double normalComp = ball->mLastG*normal;
		// Calculate the acceleration
		double tmp = 1.0/(m1+mPark->dt*mPark->mFriction);
		double dist = collisionDepth + 1.0;
		acceleration = ( (dist/(mPark->dt*tmp*m1) )*normal - ball->mNewVel)/mPark->dt -normalComp*normal;
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

void OrientedBox::InsertInBoxes(Box* box1, Box* top, long newBubbleId)
{
	int q;
	short mod[3];

	double insertionRadius = mAABB.GetLongestWidth() / 2.0;

	Box *box2;
	box1 = mPark->mPartition->GetBox(insertionRadius, mNewPos);
	for(q=0;q<3;q++)
	{

		if( box1->lo[q] - mAABB.mLowCorner[q] > 0.0 )
		{
			// Add box to the left...
			mod[q] = -1;
		}
		else if ( box1->hi[q] - mAABB.mHighCorner[q] < 0.0f )
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


	if(mMyBox && box1->mKey == mMyBox->mKey)
	{
		// We have the same base box as before. Check if the mods are the same as well.
		if(mod[0]==mMod[0] && mod[1]==mMod[1] && mod[2]==mMod[2])
		{
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
	box1 = mPark->mPartition->GetBox(insertionRadius,mNewPos);

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

	if (level==0 && mBoundingRadius * 2 > mPark->mPartition->mLevelWidth[0])
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