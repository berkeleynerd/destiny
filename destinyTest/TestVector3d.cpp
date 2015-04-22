#include "StdAfx.h"

TEST( Vector3d, InitializesToZero )
{
	Vector3d v;
	ASSERT_EQ(v.x, 0.0);
	ASSERT_EQ(v.y, 0.0);
	ASSERT_EQ(v.z, 0.0);
}