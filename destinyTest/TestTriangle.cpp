#include "StdAfx.h"

TEST(Triangle, CanCreate)
{
	Vector3d a(1.0, 2.0, 3.0);
	Vector3d b(4.0, 5.0, 6.0);
	Vector3d c(7.0, 8.0, 9.0);
	Triangle t(a, b, c);
	ASSERT_EQ(a, t.a);
	ASSERT_EQ(b, t.b);
	ASSERT_EQ(c, t.c);
}

TEST(Triangle, GetNormal)
{
	Vector3d a(0.0, 0.0, 1.0);
	Vector3d b(1.0, 0.0, 2.0);
	Vector3d c(2.0, 0.0, 1.0);
	Triangle t(a, b, c);
	Vector3d expected = Vector3d(0.0, 1.0, 0.0);
	Vector3d actual = t.GetNormal();
	ASSERT_EQ(expected.x, actual.x);
	ASSERT_EQ(expected.y, actual.y);
	ASSERT_EQ(expected.z, actual.z);
}

TEST(PointInTriangle, CenterOfTriangle)
{
	Vector3d a(0.0, 0.0, 1.0);
	Vector3d b(1.0, 0.0, 2.0);
	Vector3d c(2.0, 0.0, 1.0);
	Triangle t(a, b, c);
	Vector3d p(1.0, 0.0, 1.0);
	ASSERT_TRUE(t.ContainsPoint(p));
}

TEST(PointInTriangle, OutsideAB)
{
	Vector3d a(0.0, 0.0, 1.0);
	Vector3d b(1.0, 0.0, 2.0);
	Vector3d c(2.0, 0.0, 1.0);
	Triangle t(a, b, c);
	Vector3d p(1.5, 0.0, 2.0);
	ASSERT_FALSE(t.ContainsPoint(p));
}

TEST(PointInTriangle, OutsideAC)
{
	Vector3d a(0.0, 0.0, 1.0);
	Vector3d b(1.0, 0.0, 2.0);
	Vector3d c(2.0, 0.0, 1.0);
	Triangle t(a, b, c);
	Vector3d p(1.0, 0.0, 0.5);
	ASSERT_FALSE(t.ContainsPoint(p));
}

TEST(PointInTriangle, OutsideBC)
{
	Vector3d a(0.0, 0.0, 1.0);
	Vector3d b(1.0, 0.0, 2.0);
	Vector3d c(2.0, 0.0, 1.0);
	Triangle t(a, b, c);
	Vector3d p(2.5, 2.0, 0.0);
	ASSERT_FALSE(t.ContainsPoint(p));
}

TEST(GetClosestPoint, PointAboveTriangle)
{
	Vector3d a(0.0, 1.0, 0.0);
	Vector3d b(1.0, 2.0, 0.0);
	Vector3d c(2.0, 1.0, 0.0);
	Triangle t(a, b, c);
	Vector3d p(1.0, 1.5, 1.0);
	Vector3d expected(1.0, 1.5, 0.0);
	Vector3d actual = t.GetClosestPoint(p);
	ASSERT_EQ(expected, actual);
}

TEST(GetClosestPoint, PointLeftOfTriangle)
{
	Vector3d a(10.0, 10.0, 0.0);
	Vector3d b(0.0, 10.0, 0.0);
	Vector3d c(0.0, 0.0, 0.0);
	Triangle t(a, b, c);
	Vector3d p(-1.0, 5.0, 0.0);
	Vector3d expected(0.0, 5.0, 0.0);
	Vector3d actual = t.GetClosestPoint(p);
	ASSERT_EQ(expected, actual);
}

TEST(GetClosestPoint, PointBottomOfTriangle)
{
	Vector3d a(0.0, 0.0, 0.0);
	Vector3d b(10.0, 0.0, 0.0);
	Vector3d c(10.0, 10.0, 0.0);
	Triangle t(a, b, c);
	Vector3d p(5.0, -1.0, 0.0);
	Vector3d expected(5.0, 0.0, 0.0);
	Vector3d actual = t.GetClosestPoint(p);
	ASSERT_EQ(expected, actual);
}

TEST(GetClosestPoint, PointRightOfTriangle)
{
	Vector3d a(0.0, 0.0, 0.0);
	Vector3d b(10.0, 0.0, 0.0);
	Vector3d c(10.0, 10.0, 0.0);
	Triangle t(a, b, c);
	Vector3d p(11.0, 5.0, 0.0);
	Vector3d expected(10.0, 5.0, 0.0);
	Vector3d actual = t.GetClosestPoint(p);
	ASSERT_EQ(expected, actual);
}

TEST(GetClosestPoint, PointTopOfTriangle)
{
	Vector3d a(0.0, 0.0, 0.0);
	Vector3d b(0.0, 10.0, 0.0);
	Vector3d c(10.0, 10.0, 0.0);
	Triangle t(a, b, c);
	Vector3d p(5.0, 11.0, 0.0);
	Vector3d expected(5.0, 10.0, 0.0);
	Vector3d actual = t.GetClosestPoint(p);
	ASSERT_EQ(expected, actual);
}

TEST(GetClosestPoint, PointOffTheLongSide)
{
	Vector3d a(0.0, 0.0, 0.0);
	Vector3d b(10.0, 0.0, 0.0);
	Vector3d c(10.0, 10.0, 0.0);
	Triangle t(a, b, c);
	Vector3d p(0.0, 10.0, 0.0);
	Vector3d expected(5.0, 5.0, 0.0);
	Vector3d actual = t.GetClosestPoint(p);
	ASSERT_EQ(expected, actual);
}
