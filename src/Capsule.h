#ifndef _CAPSULE_H_
#define _CAPSULE_H_

#include "Ball.h"
#include "Ballpark.h"
#include "Collision.h"
#include "StaticCollidable.h"

BLUE_CLASS( Capsule ) : public IRoot, public StaticCollidable
{
public:
	EXPOSE_TO_BLUE();
	void Initialize(ID theID, ID parentObjectId, double ax, double ay, double az, double bx, double by, double bz, float radius);
	
	//StaticCollidable abstract functions
	float GetBoundingRadius() override;
	Vector3d GetCenter() override;
	void CollideWithBall(Ball* ball) override;
	bool CheckCollision(const Vector3d& p0, const Vector3d& p1, float radius, Vector3d& normal, double& timeOfImpact) override;
	void InsertInBoxes(Box* box1, Box* top, long newBubbleId) override;

private:
	void ReactToCollision(Ball* ball, Vector3d& ballPosition, Vector3d& ballVelocity, double m1, Vector3d& normal, double timeOfImpact);
	void HandleCollisionNonIteratively(Ball* ball, Vector3d startPos, Vector3d endPos, double mass, Vector3d velocity);
	Vector3d mHemisphereA;
	Vector3d mHemisphereB;
	float mRadius;
	Vector3d mAB;
};
TYPEDEF_BLUECLASS( Capsule );
#endif
