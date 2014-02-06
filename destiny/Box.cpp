#include "stdafx.h"
#include "Box.h"

static int boxCount = 0;
//---------------------------------------------------------------------------------------
// Box Constructor
//---------------------------------------------------------------------------------------
Box::Box(__int64 _ix, __int64 _iy, __int64 _iz, long _level,Partition *_p)
{
	mLevel = _level;
	mPartition = _p;
	mParent = 0;
	mBubble = -1;
	mObject = 0;
	mHandled = false;
	// Find the number of grid divisions for this level
	// Basically mGridBase elevated to the power of levels
	__int64 n = mPartition->mLevelGrid[mLevel];

	mKey = Key(_ix,_iy,_iz,n);

	double boxWidth = mPartition->mLevelWidth[mLevel];
	double center = boxWidth*n*0.5f;
	// Define the bounding limits of this box
	lo[0] = mKey.ix*boxWidth - center;
	hi[0] = (mKey.ix+1)*boxWidth - center;

	lo[1] = mKey.iy*boxWidth - center;
	hi[1] = (mKey.iy+1)*boxWidth - center;

	lo[2] = mKey.iz*boxWidth - center;
	hi[2] = (mKey.iz+1)*boxWidth - center;

	SetParent();

	// insert the box in the map
	mPartition->mLevels[mLevel][mKey] = this;
}

//---------------------------------------------------------------------------------------
// Box Destructor
//---------------------------------------------------------------------------------------
Box::~Box()
{
	if(mParent)
		mParent->RemoveChildren(this);
	mPartition->mLevels[mLevel].erase(mKey);
	mChildren.clear();
	balls.clear();
	Py_XDECREF(mObject);
	mHandled = false;
}

void Box::ReleaseBox()
{
	mPartition->mBoxAllocator.ReleaseBox(this);
}




//---------------------------------------------------------------------------------------
// Box::SetParent
//---------------------------------------------------------------------------------------

void Box::SetObject(PyObject *object)
{
	if(object==mObject)
		return;

	if(mObject)
	{
		Py_DECREF(mObject);
		mObject = 0;
	}

	if(object)
	{
		Py_INCREF(object);
		mObject = object;
	}
}

//---------------------------------------------------------------------------------------
// Box::SetParent
//---------------------------------------------------------------------------------------

void Box::SetParent()
{
	if(mLevel == 0)
		return; // already top level box

	// get parent box
	mParent = mPartition->GetBox(mKey.ix >> 2 , mKey.iy >> 2 , mKey.iz >> 2  ,mLevel - 1);
	mParent->AddChildren(this);
}

//---------------------------------------------------------------------------------------
// Box::AddChildren
//---------------------------------------------------------------------------------------
void Box::AddChildren(Box *box)
{
	if(!box)
		return;

	mChildren.insert(box);
}

//---------------------------------------------------------------------------------------
// Box::AddBall
//---------------------------------------------------------------------------------------
void Box::AddBall(Ball *ball)
{
	if(!ball)
		return;

	balls.insert(ball);
}


//---------------------------------------------------------------------------------------
// Box::RemoveChildren
//---------------------------------------------------------------------------------------

void Box::RemoveChildren(Box *box)
{
	SetOfBoxes::iterator it = mChildren.find(box);
	if(it != mChildren.end())
		mChildren.erase(it); // children found, erase it

	if(mChildren.empty() && balls.empty())
	{
		ReleaseBox();
	}
}

//---------------------------------------------------------------------------------------
// Box::RemoveBall
//---------------------------------------------------------------------------------------

void Box::RemoveBall(Ball *ball)
{
	SetOfBalls::iterator it = balls.find(ball);
	if(it != balls.end())
		balls.erase(it); // ball found, erase it

	if(mChildren.empty() && balls.empty())
	{
		ReleaseBox();
	}
}

//---------------------------------------------------------------------------------------
// Box::Traverse
//
// Seed fill space partition at same level as this box with the given seed
//---------------------------------------------------------------------------------------

void Box::Traverse(long seed)
{
	if(mBubble != -1)
		return; // I have already been traversed

	// I have not been traversed. Mark myself traversed and then seed all neighbors
	mBubble = seed;

	int i,j,k;

	// Check all my 26 neighbors
	for(i= -1; i < 2; i++)
	{
		for(j= -1; j < 2; j++)
		{
			for(k= -1; k < 2; k++)
			{
				if(i==0 && j==0 && k==0)
					continue;

				Box *box = mPartition->CheckOffsetBox(this,i,j,k);
				if(box)
					box->Traverse(seed);
			}
		}
	}

	return;
}

//---------------------------------------------------------------------------------------
// Box::NearbyBubbles
//
// Seed fill space partition at same level as this box with the given seed
//---------------------------------------------------------------------------------------

long Box::NearbyBubbles()
{

	int i,j,k;

	// Check all my 26 neighbors
	for(i= -1; i < 2; i++)
	{
		for(j= -1; j < 2; j++)
		{
			for(k= -1; k < 2; k++)
			{
				if(i==0 && j==0 && k==0)
					continue;

				Box *box = mPartition->CheckOffsetBox(this,i,j,k);
				if(box && box->mBubble != -1)
					return box->mBubble;
			}
		}
	}

	return -1;
}



size_t BoxPtrHasher::operator ()(const Box* b) const
{
	return stdext::hash_compare<Key>()(b->mKey);
}

bool BoxPtrHasher::operator () (const Box* r, const Box* l) const
{
	return (r->mKey < l->mKey);
}
