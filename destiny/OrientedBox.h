#ifndef _ORIENTED_BOX_H_
#define _ORIENTED_BOX_H_

#include "Ball.h"
#include "Ballpark.h"
#include "StaticCollidable.h"
#include "BoxShape.h"
#include "AABB.h"

BLUE_CLASS( OrientedBox ) : public IRoot, public StaticCollidable
{
public:
	EXPOSE_TO_BLUE();
	void Initialize(
	    ID theID,
	    ID parentObjectId,
		double c0, double c1, double c2,
		double x0, double x1, double x2,
		double y0, double y1, double y2,
		double z0, double z1, double z2 );

	//StaticCollidable abstract functions
	float GetBoundingRadius();
	Vector3d GetCenter();
	void CollideWithBall(Ball* ball);
	void InsertInBoxes(Box* box1, Box* top, long newBubbleId);

private:
	void ReactToCollision(Ball* ball, Vector3d& ballPosition, Vector3d& ballVelocity, double m1, Vector3d& normal, double timeOfImpact);
	BoxShape m_boxShape;
	short mMod[3];
	float mBoundingRadius;
	AABB mAABB;
};
TYPEDEF_BLUECLASS( OrientedBox );
#endif