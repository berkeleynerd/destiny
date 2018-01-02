#include "StdAfx.h"

TEST(GetPointOnBallThatFirstIntersectsPlane, ArbitraryPoint)
{
	Vector3d ball_pos(0.0, 3.0, 0.0);
	double ball_radius = 1.0;
	Vector3d plane_normal(0.0, 1.0, 0.0);
	Vector3d expected(0.0, 2.0, 0.0);
	Vector3d actual = GetPointOnBallThatFirstIntersectsPlane(ball_pos, ball_radius, plane_normal);
	ASSERT_EQ(expected, actual);
}

TEST(IntersectSegmentPlane, IntersectionExists)
{
	Vector3d plane_point(0.0, 0.0, 0.0);
	Vector3d plane_normal(0.0, 1.0, 0.0);
	Planed plane(plane_point, plane_normal);
	Vector3d a(0.0, 1.0, 0.0);
	Vector3d b(0.0, -1.0, 0.0);

	Vector3d collision_point;
	double collision_time;

	bool collision_exists = IntersectSegmentPlane(a, b, plane, collision_point, collision_time);
	ASSERT_TRUE(collision_exists);
	ASSERT_EQ(Vector3d(0.0, 0.0, 0.0), collision_point);
	ASSERT_EQ(0.5, collision_time);
}

TEST(IntersectSegmentPlane, NoIntersection)
{
	Vector3d plane_point(0.0, 0.0, 0.0);
	Vector3d plane_normal(0.0, 1.0, 0.0);
	Planed plane(plane_point, plane_normal);
	Vector3d a(0.0, 3.0, 0.0);
	Vector3d b(0.0, 2.0, 0.0);

	Vector3d collision_point;
	double collision_time;
	bool collision_exists = IntersectSegmentPlane(a, b, plane, collision_point, collision_time);
	ASSERT_FALSE(collision_exists);
}

TEST(IntersectSegmentPlane, ParallelLineSegment)
{
	Vector3d plane_point(0.0, 0.0, 0.0);
	Vector3d plane_normal(0.0, 0.0, 1.0);
	Planed plane(plane_point, plane_normal);
	Vector3d a(0.0, 1.0, 0.0);
	Vector3d b(0.0, -1.0, 0.0);

	Vector3d collision_point;
	double collision_time;
	bool collision_exists = IntersectSegmentPlane(a, b, plane, collision_point, collision_time);
	ASSERT_FALSE(collision_exists);
}

TEST(IntersectSegmentSphere, HeadOn)
{
	Vector3d sphere_pos(0.0, 0.0, 0.0);
	double sphere_rad = 1.0;
	Vector3d a(10.0, 0.0, 0.0);
	Vector3d b(0.0, 0.0, 0.0);

	Vector3d collision_point;
	double collision_time;

	bool collision_exists = IntersectSegmentSphere(a, b, sphere_pos, sphere_rad, collision_point, collision_time);
	
	ASSERT_TRUE(collision_exists);
	ASSERT_EQ(Vector3d(1.0, 0.0, 0.0), collision_point);
	ASSERT_EQ(0.9, collision_time);
}

TEST(IntersectSegmentSphere, FromInside)
{
	Vector3d sphere_pos(0.0, 0.0, 0.0);
	double sphere_rad = 1.0;
	Vector3d a(0.0, 0.0, 0.0);
	Vector3d b(10.0, 0.0, 0.0);

	Vector3d collision_point;
	double collision_time;

	bool collision_exists = IntersectSegmentSphere(a, b, sphere_pos, sphere_rad, collision_point, collision_time);

	ASSERT_TRUE(collision_exists);
	ASSERT_EQ(a, collision_point);
	ASSERT_EQ(0.0, collision_time);
}

TEST(IntersectSegmentSphere, CompleteMiss)
{
	Vector3d sphere_pos(0.0, 0.0, 0.0);
	double sphere_rad = 1.0;
	Vector3d a(0.0, 0.0, 7.0);
	Vector3d b(0.0, 0.0, 8.0);

	Vector3d collision_point;
	double collision_time;

	bool collision_exists = IntersectSegmentSphere(a, b, sphere_pos, sphere_rad, collision_point, collision_time);

	ASSERT_FALSE(collision_exists);
}

TEST(IntersectSphereTriangle, CenterOfSphereIntersectsTriangle)
{
	Vector3d a(0.0, 0.0, 0.0);
	Vector3d b(2.0, 0.0, 0.0);
	Vector3d c(1.0, 2.0, 0.0);
	Triangle t(a, b, c);

	Vector3d sphere_position(1.0, 1.0, 2.0);
	Vector3d sphere_velocity(0.0, 0.0, -2.0);
	double sphere_radius = 1.0;
	
	Vector3d collision_point;
	double collision_time;
	Vector3d normal;

	bool collision_exists = IntersectSphereTriangle(sphere_position, sphere_radius, sphere_velocity, t, collision_point, collision_time, normal);

	ASSERT_TRUE(collision_exists);
	ASSERT_EQ(Vector3d(1.0, 1.0, 0.0), collision_point);
	ASSERT_EQ(collision_time, 0.5);
	ASSERT_EQ(Vector3d(0.0, 0.0, 1.0), normal);

}

TEST(IntersectSphereTriangle, SphereGrazesTriangleEdge)
{
	Vector3d a(0.0, 0.0, 0.0);
	Vector3d b(2.0, 0.0, 0.0);
	Vector3d c(1.0, 2.0, 0.0);
	Triangle t(a, b, c);

	Vector3d sphere_position(0.0, -1.0, 2.0);
	Vector3d sphere_velocity(0.0, 0.0, -2.0);
	double sphere_radius = 1.0;

	Vector3d collision_point;
	double collision_time;
	Vector3d normal;

	bool collision_exists = IntersectSphereTriangle(sphere_position, sphere_radius, sphere_velocity, t, collision_point, collision_time, normal);

	ASSERT_TRUE(collision_exists);
	ASSERT_EQ(Vector3d(0.0, 0.0, 0.0), collision_point);
	ASSERT_EQ(collision_time, 1.0);
	ASSERT_EQ(Vector3d(0.0, -1.0, 0.0), normal);
}

TEST(IntersectSphereTriangle, CenterOfSphereIntersectsTriangleFromTheWrongSide)
{
	Vector3d a(0.0, 0.0, 0.0);
	Vector3d b(2.0, 0.0, 0.0);
	Vector3d c(1.0, 2.0, 0.0);
	Triangle t(c, b, a);

	Vector3d sphere_position(1.0, 1.0, 2.0);
	Vector3d sphere_velocity(0.0, 0.0, -2.0);
	Vector3d normal;
	double sphere_radius = 1.0;

	Vector3d collision_point;
	double collision_time;

	bool collision_exists = IntersectSphereTriangle(sphere_position, sphere_radius, sphere_velocity, t, collision_point, collision_time, normal);

	ASSERT_FALSE(collision_exists);
}

TEST(IntersectSphereTriangle, SphereDoesNotIntersectPlane)
{
	Vector3d a(0.0, 0.0, 0.0);
	Vector3d b(2.0, 0.0, 0.0);
	Vector3d c(1.0, 2.0, 0.0);
	Triangle t(a, b, c);

	Vector3d sphere_position(1.0, 1.0, 2.0);
	Vector3d sphere_velocity(0.0, 100.0, 0.0);
	double sphere_radius = 1.0;

	Vector3d collision_point;
	double collision_time;
	Vector3d normal;

	bool collision_exists = IntersectSphereTriangle(sphere_position, sphere_radius, sphere_velocity, t, collision_point, collision_time, normal);

	ASSERT_FALSE(collision_exists);
}

TEST(IntersectSphereTriangle, SphereDoesNotIntersectTriangle)
{
	Vector3d a(0.0, 0.0, 0.0);
	Vector3d b(2.0, 0.0, 0.0);
	Vector3d c(1.0, 2.0, 0.0);
	Triangle t(a, b, c);

	Vector3d sphere_position(100.0, 1.0, 1.0);
	Vector3d sphere_velocity(0.0, 0.0, -2.0);
	double sphere_radius = 1.0;

	Vector3d collision_point;
	double collision_time;
	Vector3d normal;

	bool collision_exists = IntersectSphereTriangle(sphere_position, sphere_radius, sphere_velocity, t, collision_point, collision_time, normal);

	ASSERT_FALSE(collision_exists);
}