#include "StdAfx.h"

TEST( Vector3d, InitializesToZero )
{
	Vector3d v;
	ASSERT_EQ(v.x, 0.0);
	ASSERT_EQ(v.y, 0.0);
	ASSERT_EQ(v.z, 0.0);
}

TEST( Vector3d, Subtraction )
{
	Vector3d a(1.0, 2.0, 3.0);
	Vector3d b(0.0, 1.0, 2.0);
	

	ASSERT_EQ(a - b, Vector3d(1.0, 1.0, 1.0));
}

TEST( Vector3dCrossProduct, ParallelVectors )
{
	Vector3d a(2.0, 4.0, 6.0);
	Vector3d b(1.0, 2.0, 3.0);

	ASSERT_EQ(a.Cross(b), Vector3d(0.0, 0.0, 0.0));
}

TEST( Vector3dCrossProduct, OrthogonalVectors )
{
	Vector3d a(1.0, 0.0, 0.0);
	Vector3d b(0.0, 1.0, 0.0);

	ASSERT_EQ(a.Cross(b), Vector3d(0.0, 0.0, 1.0));
}