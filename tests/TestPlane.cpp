#include "StdAfx.h"

TEST(Plane, CanCreate)
{
	Vector3d point(1.0, 2.0, 3.0);
	Vector3d normal(0.0, 1.0, 0.0);
	Planed plane(point, normal);
	ASSERT_EQ(2.0, plane.d);
	ASSERT_EQ(normal, plane.normal);
}