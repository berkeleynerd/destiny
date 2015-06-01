
#ifndef _PARTITIONABLE_H_
#define _PARTITIONABLE_H_


#include "Vector3d.h"
class Box;
class Ballpark;
#include "Hashers.h"

typedef std::unordered_set<Box *, BoxPtrHasher, BoxPtrHasher> SetOfOrderedBoxes;
typedef int64_t ID;

class Partitionable
{
public:
	Partitionable();

	virtual void ClearBoxes() = 0;
	virtual void DeleteFromBoxes() = 0;
	virtual void InsertInBox(Box *box) = 0;
	virtual float GetBoundingRadius() = 0;
	virtual double GetInflatedRadius(double dt) = 0; // Get the radius of the largest sphere the object can occupy during the timestep
	virtual void SetBoxActive(int boxId, bool isActive) = 0;
	virtual void InsertInBoxes(Box* box1, Box* top, long newBubbleId) = 0;

	ID mId;
	bool mHandled;

	Vector3d mNewPos; // current position of ball
	Vector3d mOldPos; // position of ball one tick interval ago
	long mNewBubble; // current bubble ID
	long mOldBubble; // old bubble Id
	bool isInteractive; // true if ball is associated with a user
	
	SetOfOrderedBoxes mBoxes;
	Box *mMyBox;
	Ballpark *mPark;

	long mEffectStamp; // Time counter stamp for effect
	bool isMoribund; // Flags that tells whehther this ball is soon to die
	
};

#endif