#ifndef _STATIC_COLLIDABLE_H_
#define _STATIC_COLLIDABLE_H_

#include "Box.h"
#include "Partitionable.h"

typedef std::unordered_set<Box *, BoxPtrHasher, BoxPtrHasher> SetOfOrderedBoxes;

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
	
};

#endif