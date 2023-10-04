#ifndef _STATIC_COLLIDABLE_H_
#define _STATIC_COLLIDABLE_H_

#include "Box.h"
#include "Partitionable.h"


class StaticCollidable : public Partitionable
{
public:
	StaticCollidable();
	virtual Vector3d GetCenter() = 0;
	virtual void CollideWithBall(Ball* ball) = 0;
	// -------------------------------------------
	// Description:
	//    Checks for collision with a ball. Calculates time of impact and normal if collision exists.
	// Arguments:
	//    p0 - location of the ball at the start of the time step.
	//    p1 - location of the ball at the end of the time step if no collision occurs.
	//    radius - radius of the ball
	//    normal (out) - A normal vector pointing in the direction of the collision impact.
	//    timeOfImpact (out) - The time of impact in the range between 0.0 and 1.0 as a fraction of the time step.
	// Return Value:
	//    True if collision takes place, false otherwise.
	// -------------------------------------------
	virtual bool CheckCollision(const Vector3d &p0, const Vector3d &p1, float radius, Vector3d& normal, double& timeOfImpact) = 0;

	void ClearBoxes();
	void DeleteFromBoxes();
	void InsertInBox(Box *box);
	
	virtual float GetBoundingRadius() = 0;
	void SetBoxActive(int boxId, bool isActive){}; // Static objects don't make a box active.
	
	double GetInflatedRadius(double dt);

	// When mId is local to the server/client (ie for negative ID cases such as mini-capsules) it will not be consistent across clients.
	// We track the parent's ID (which IS consistent) so that we can ensure equal sorting order for all clients.
	// (Within a single ball, its mini capsules/boxes are always sent in a consistent order)
	ID mParentBallId;
};

#endif