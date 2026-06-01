// Copyright © 2015 CCP ehf.

#ifndef _BOX_SHAPE_H_
#define _BOX_SHAPE_H_
#include "Vector3d.h"

enum BoxCorner
{
	CORNER_A,
	CORNER_B,
	CORNER_C,
	CORNER_D,
	CORNER_E,
	CORNER_F,
	CORNER_G,
	CORNER_H,
	CORNER_COUNT
};

enum BoxSide
{
	SIDE_FRONT,
	SIDE_BACK,
	SIDE_LEFT,
	SIDE_RIGHT,
	SIDE_TOP,
	SIDE_BOTTOM,
	SIDE_COUNT
};

class RectangleShape
{
public:
	RectangleShape() {};
	RectangleShape(Vector3d a, Vector3d b, Vector3d c, Vector3d d);
	bool CollideWithSphere(Vector3d spherePosition, double sphereRadius, Vector3d sphereVelocity, Vector3d & collisionPoint, double & collisionTime, Vector3d& normal);

	Vector3d m_a;
	Vector3d m_b;
	Vector3d m_c;
	Vector3d m_d;
	Vector3d m_normal;
};

class BoxShape
{
public:
	BoxShape() {};
	BoxShape(Vector3d corner, Vector3d localX, Vector3d localY, Vector3d localZ);

	BoxCorner GetClosestCornerToPoint(Vector3d p);
	bool CollideWithSphere(Vector3d spherePosition, double sphereRadius, Vector3d sphereVelocity, Vector3d & collisionPoint, double & collisionTime, Vector3d& normal);
	Vector3d GetCenter();
	double GetBoundingRadius();
	Vector3d GetClosestPointOnBox(Vector3d p);
	// Returns true if the infinitely (within floating point reason) line p0 and p1 intersects our box.
	// In that case, return the sides that intersect and the distance at which the intersect happens in units
	// of the distance between p0 and p1.  A value between 0 and 1 means the intersect happens within p0 and p1.
	bool RayIntersect(const Vector3d p0, const Vector3d p1, BoxSide& side1, double& d1, BoxSide& side2, double& d2);

	Vector3d m_corners[CORNER_COUNT];
	RectangleShape m_sides[SIDE_COUNT];

	Vector3d m_localX;
	Vector3d m_localY;
	Vector3d m_localZ;
};
#endif