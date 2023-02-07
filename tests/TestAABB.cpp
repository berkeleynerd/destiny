#include "StdAfx.h"

TEST(Constructor, DefaultConstructor)
{
	AABB b;
	ASSERT_EQ(Vector3d(0.0, 0.0, 0.0), b.mLowCorner);
	ASSERT_EQ(Vector3d(0.0, 0.0, 0.0), b.mHighCorner);
}

TEST(Constructor, ConstructorWithParameters)
{
	AABB b(Vector3d(1.0, 2.0, 3.0), Vector3d(4.0, 5.0, 6.0));
	ASSERT_EQ(Vector3d(1.0, 2.0, 3.0), b.mLowCorner);
	ASSERT_EQ(Vector3d(4.0, 5.0, 6.0), b.mHighCorner);
}

TEST(Update, High)
{
	AABB b;
	b.Update(Vector3d(1.0, 2.0, 3.0));
	ASSERT_EQ(Vector3d(1.0, 2.0, 3.0), b.mHighCorner);
}

TEST(Update, Low)
{
	AABB b;
	b.Update(Vector3d(-1.0, -2.0, -3.0));
	ASSERT_EQ(Vector3d(-1.0, -2.0, -3.0), b.mLowCorner);
}

TEST(Update, HighIgnore)
{
	AABB b(Vector3d(0.0, 0.0, 0.0), Vector3d(5.0, 5.0, 5.0));
	b.Update(Vector3d(1.0, 2.0, 3.0));
	ASSERT_EQ(Vector3d(5.0, 5.0, 5.0), b.mHighCorner);
}

TEST(Update, LowIgnore)
{
	AABB b(Vector3d(-5.0, -5.0, -5.0), Vector3d(0.0, 0.0, 0.0));
	b.Update(Vector3d(-1.0, -2.0, -3.0));
	ASSERT_EQ(Vector3d(-5.0, -5.0, -5.0), b.mLowCorner);
}

TEST(CanExcludeCollision, ExcludeReturnsFalseWhenRadiusIntersects)
{
	AABB aabb(Vector3d(0.0, 0.0, 0.0), Vector3d(1.0, 1.0, 1.0));
	Vector3d p0(-1.0, -1.0, 0.0);
	Vector3d p1(-1.0, 1.0, 0.0);
	double radius = 1.0;
	ASSERT_FALSE(aabb.CanExcludeCollision(p0, p1, radius));
}

TEST(CanExcludeCollision, ExcludeLeft)
{
	AABB aabb(Vector3d(0.0, 0.0, 0.0), Vector3d(1.0, 1.0, 1.0));
	Vector3d p0(-1.0, -1.0, 0.0);
	Vector3d p1(-1.0, 1.0, 0.0);
	double radius = 0.5;
	ASSERT_TRUE(aabb.CanExcludeCollision(p0, p1, radius));
}

TEST(CanExcludeCollision, ExcludeRight)
{
	AABB aabb(Vector3d(0.0, 0.0, 0.0), Vector3d(1.0, 1.0, 1.0));
	Vector3d p0(2.0, -1.0, 0.0);
	Vector3d p1(2.0, 1.0, 0.0);
	double radius = 0.5;
	ASSERT_TRUE(aabb.CanExcludeCollision(p0, p1, radius));
}

TEST(CanExcludeCollision, ExcludeTop)
{
	AABB aabb(Vector3d(0.0, 0.0, 0.0), Vector3d(1.0, 1.0, 1.0));
	Vector3d p0(-1.0, 2.0, 0.0);
	Vector3d p1(1.0, 2.0, 0.0);
	double radius = 0.5;
	ASSERT_TRUE(aabb.CanExcludeCollision(p0, p1, radius));
}

TEST(CanExcludeCollision, ExcludeBottom)
{
	AABB aabb(Vector3d(0.0, 0.0, 0.0), Vector3d(1.0, 1.0, 1.0));
	Vector3d p0(-1.0, -1.0, 0.0);
	Vector3d p1(1.0, -1.0, 0.0);
	double radius = 0.5;
	ASSERT_TRUE(aabb.CanExcludeCollision(p0, p1, radius));
}

TEST(CanExcludeCollision, ExcludeFront)
{
	AABB aabb(Vector3d(0.0, 0.0, 0.0), Vector3d(1.0, 1.0, 1.0));
	Vector3d p0(-1.0, 0.0, 2.0);
	Vector3d p1(1.0, 0.0, 2.0);
	double radius = 0.5;
	ASSERT_TRUE(aabb.CanExcludeCollision(p0, p1, radius));
}

TEST(CanExcludeCollision, ExcludeBack)
{
	AABB aabb(Vector3d(0.0, 0.0, 0.0), Vector3d(1.0, 1.0, 1.0));
	Vector3d p0(-1.0, 0.0, -1.0);
	Vector3d p1(1.0, 0.0, -1.0);
	double radius = 0.5;
	ASSERT_TRUE(aabb.CanExcludeCollision(p0, p1, radius));
}

TEST(CanExcludeCollision, InsideBox)
{
	AABB aabb(Vector3d(0.0, 0.0, 0.0), Vector3d(1.0, 1.0, 1.0));
	Vector3d p0(0.5, 0.5, 0.5);
	Vector3d p1(0.5, 0.5, 0.5);
	double radius = 0.5;
	ASSERT_FALSE(aabb.CanExcludeCollision(p0, p1, radius));
}

TEST(GetLongestWidth, ZeroSize)
{
	AABB aabb;
	ASSERT_EQ(0.0, aabb.GetLongestWidth());
}

TEST(GetLongestWidth, XAxisIsLongest)
{
	AABB aabb(Vector3d(-0.5, -0.5, -0.5), Vector3d(1.0, 0.5, 0.5));
	ASSERT_EQ(1.5, aabb.GetLongestWidth());
}

TEST(GetLongestWidth, YAxisIsLongest)
{
	AABB aabb(Vector3d(-0.5, -0.5, -0.5), Vector3d(0.5, 1.0, 0.5));
	ASSERT_EQ(1.5, aabb.GetLongestWidth());
}

TEST(GetLongestWidth, ZAxisIsLongest)
{
	AABB aabb(Vector3d(-0.5, -0.5, -0.5), Vector3d(0.5, 0.5, 1.0));
	ASSERT_EQ(1.5, aabb.GetLongestWidth());
}