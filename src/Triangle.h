#ifndef _TRIANGLE_H_
#define _TRIANGLE_H_
#include "Vector3d.h"


class Triangle
{
public:
	Vector3d a;
	Vector3d b;
	Vector3d c;

	Triangle(Vector3d a, Vector3d b, Vector3d c);
	Vector3d GetNormal();
	Vector3d GetClosestPoint(Vector3d p);
	bool ContainsPoint(Vector3d p);
};



#endif