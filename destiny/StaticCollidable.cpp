#include "StdAfx.h"
#include "StaticCollidable.h"


StaticCollidable::StaticCollidable():
	mParentBallId(0)
{
}

//---------------------------------------------------------------------------------------
// ClearBoxes Deletes links to all boxes
//---------------------------------------------------------------------------------------

void StaticCollidable::ClearBoxes()
{
	mBoxes.clear();
	mMyBox = 0;
}

//---------------------------------------------------------------------------------------
// DeleteFromBoxes removes the collidable from all its boxes and clears the box list
//---------------------------------------------------------------------------------------
void StaticCollidable::DeleteFromBoxes()
{
	SortedSetOfBoxes::iterator it;
	Box* box;

	// Go over all boxes the ball intersects
	for(it = mBoxes.begin();it != mBoxes.end(); ++it)
	{
		box = (*it);
		box->RemoveStaticCollidable(this);
	}

	// Now clear the box list
	ClearBoxes();
}

void StaticCollidable::InsertInBox(Box *box)
{
	if(!box)
		return;

	box->AddStaticCollidable(this);
	mBoxes.insert(box);
}

double StaticCollidable::GetInflatedRadius(double dt)
{
	return GetBoundingRadius();
}

bool StaticCollidableSortComparer::operator () (const StaticCollidable* x, const StaticCollidable* y) const
{
	// returns true if X precedes and is not equal to Y in the sort order.
	// StaticCollidables use their parent ball ID as the primary sort key, and only use their own
	// ID to sort StaticCollidable within the same parent 
	if (x->mParentBallId < y->mParentBallId) return true;
	if (x->mParentBallId > y->mParentBallId) return false;
	return x->mId < y->mId;
}
