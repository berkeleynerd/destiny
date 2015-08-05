#ifndef _BOX_SHAPE_H_
#define _BOX_SHAPE_H_
#include "Vector3d.h"

enum BoxCorner
{
	A,
	B,
	C,
	D,
	E,
	F,
	G,
	H,
	CORNER_COUNT
};

enum BoxSide
{
	FRONT,
	BACK,
	LEFT,
	RIGHT,
	TOP,
	BOTTOM,
	SIDE_COUNT
};

class RectangleShape
{
public:
	RectangleShape(){};
	RectangleShape(Vector3d a, Vector3d b, Vector3d c, Vector3d d);
	bool CollideWithSphere(Vector3d sphere_position, double sphere_radius, Vector3d sphere_velocity, Vector3d& collision_point, double& collision_time, Vector3d& normal);

	Vector3d m_a;
	Vector3d m_b;
	Vector3d m_c;
	Vector3d m_d;
	Vector3d m_normal;
};

class BoxShape
{
public:
	BoxShape(){};
	BoxShape(Vector3d corner, Vector3d local_x, Vector3d local_y, Vector3d local_z);

	BoxCorner GetClosestCornerToPoint(Vector3d p);
	bool CollideWithSphere(Vector3d sphere_position, double sphere_radius, Vector3d sphere_velocity, Vector3d& collision_point, double& collision_time, Vector3d& normal);
	Vector3d GetCenter();
	double GetBoundingRadius();

	Vector3d m_corners[CORNER_COUNT];
	RectangleShape m_sides[SIDE_COUNT];

	Vector3d m_localX;
	Vector3d m_localY;
	Vector3d m_localZ;
};
#endif