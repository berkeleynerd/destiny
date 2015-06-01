#include "StdAfx.h"
#include "StaticCollidable.h"


size_t StaticCollidablePtrHasher::operator ()(const StaticCollidable* c) const
{
	return std::hash<ID>()(c->mId);
}

bool StaticCollidablePtrHasher::operator () (const StaticCollidable* r, const StaticCollidable* l) const
{
	return r->mId == l->mId;
}

StaticCollidable::StaticCollidable()
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
	SetOfBoxes::iterator it;
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