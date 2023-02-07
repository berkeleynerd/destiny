#ifndef DST_BOX_H
#define DST_BOX_H

#include "Partition.h"
#include "Vector3d.h"
#include "SortedSets.h"

class Ball;

class Box
{
friend class Partition;
public:
	Box( int64_t i,  int64_t j,  int64_t k, long level,Partition *p);
	~Box();

	void RemoveChildren(Box *box);
	void AddChildren(Box *box);
	void RemoveBall(Ball *ball);
	void RemoveStaticCollidable(StaticCollidable *collidable);
	void AddBall(Ball *ball);
	void AddStaticCollidable(StaticCollidable* collidable);
	void Traverse(long seed);
	long NearbyBubbles();
	void SetObject(PyObject *object);
    bool operator == ( const Box& v) const  { return mKey==v.mKey;};

	void SetParent();

	// Bouding vectors of this box
	Vector3d lo;
	Vector3d hi;

	// key into map of box
	Key mKey;

	// balls contained in this box at this level
	SortedSetOfBalls balls;

	// Static collidable objects contained in this box at this level
	SortedSetOfStaticCollidables mStaticCollidables;

	// Boxes underneath this one in the partition
	SortedSetOfBoxes mChildren;

	// my parent in the partition hierarchy
	Box *mParent;

	// Generic python pointer
	PyObject *mObject;

private:
	Partition *mPartition;

public:
	// my own level
	long mLevel;

	// Traversal flag used to divide partition into bubbles
	long mBubble;

	// Flag used for optimization when finding nearby balls
	bool mHandled;

private:
	//Release the box from its partition allocator, whence it came
	void ReleaseBox();
};


#endif
