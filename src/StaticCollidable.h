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