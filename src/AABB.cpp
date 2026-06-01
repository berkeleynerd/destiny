// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "AABB.h"

AABB::AABB()
{

}

AABB::AABB(Vector3d low, Vector3d high)
{
	mLowCorner = low;
	mHighCorner = high;
}

void AABB::Update(const Vector3d& vertex)
{
	if ( vertex.x > mHighCorner.x )
	{
		mHighCorner.x = vertex.x;
	}
	else if ( vertex.x < mLowCorner.x )
	{
		mLowCorner.x = vertex.x;
	}
	if ( vertex.y > mHighCorner.y )
	{
		mHighCorner.y = vertex.y;
	}
	else if ( vertex.y < mLowCorner.y )
	{
		mLowCorner.y = vertex.y;
	}
	if ( vertex.z > mHighCorner.z )
	{
		mHighCorner.z = vertex.z;
	}
	else if ( vertex.z < mLowCorner.z )
	{
		mLowCorner.z = vertex.z;
	}
}

bool AABB::CanExcludeCollision(const Vector3d& p0, const Vector3d& p1, double radius)
{
	// If the sphere is on one side of the aligned box before and after moving
	// it definitely does NOT collide with the box.
	return	( p0.x - radius > mHighCorner.x && p1.x - radius > mHighCorner.x ) ||
			( p0.x + radius < mLowCorner.x  && p1.x + radius < mLowCorner.x  ) ||
			( p0.y - radius > mHighCorner.y && p1.y - radius > mHighCorner.y ) ||
			( p0.y + radius < mLowCorner.y  && p1.y + radius < mLowCorner.y  ) ||
			( p0.z - radius > mHighCorner.z && p1.z - radius > mHighCorner.z ) ||
			( p0.z + radius < mLowCorner.z  && p1.z + radius < mLowCorner.z  );
}

double AABB::GetLongestWidth()
{
	double dx = mHighCorner.x - mLowCorner.x;
	double dy = mHighCorner.y - mLowCorner.y;
	double dz = mHighCorner.z - mLowCorner.z;
	return std::max(dx, std::max(dy, dz));
}