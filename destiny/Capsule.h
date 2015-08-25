#ifndef _CAPSULE_H_
#define _CAPSULE_H_

#include "Ball.h"
#include "Ballpark.h"
#include "StaticCollidable.h"

BLUE_CLASS( Capsule ) : public IRoot, public StaticCollidable
{
public:
	EXPOSE_TO_BLUE();
	void Initialize(ID theID, double ax, double ay, double az, double bx, double by, double bz, float radius);
	
	//StaticCollidable abstract functions
	float GetBoundingRadius();
	Vector3d GetCenter();
	void CollideWithBall(Ball* ball);
	void InsertInBoxes(Box* box1, Box* top, long newBubbleId);

private:
	void ReactToCollision(Ball* ball, Vector3d& ballPosition, Vector3d& ballVelocity, double m1, Vector3d& normal, double timeOfImpact);
	Vector3d mHemisphereA;
	Vector3d mHemisphereB;
	float mRadius;
	Vector3d mAB;
};
TYPEDEF_BLUECLASS( Capsule );
#endif