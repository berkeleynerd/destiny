#ifndef _AABB_H_
#define _AABB_H_
#include "Vector3d.h"

class AABB
{
public:
	AABB();
	AABB(Vector3d low, Vector3d high);
	void Update(const Vector3d& vertex);
	bool CanExcludeCollision(const Vector3d& p0, const Vector3d& p1, double radius);
	double GetLongestWidth();
	Vector3d mLowCorner;
	Vector3d mHighCorner;
};
#endif