#include "StdAfx.h"

class UnitCubeTest : public ::testing::Test
{
protected:
	BoxShape box;
	virtual void SetUp()
	{
		Vector3d corner(0.0, 0.0, 0.0);
		Vector3d local_x(1.0, 0.0, 0.0);
		Vector3d local_y(0.0, 1.0, 0.0);
		Vector3d local_z(0.0, 0.0, 1.0);
		box = BoxShape(corner, local_x, local_y, local_z);
	}
	
};

TEST_F(UnitCubeTest, CanCreate)
{
	ASSERT_EQ(Vector3d(1.0, 0.0, 0.0), box.m_localX);
	ASSERT_EQ(Vector3d(0.0, 1.0, 0.0), box.m_localY);
	ASSERT_EQ(Vector3d(0.0, 0.0, 1.0), box.m_localZ);

	ASSERT_EQ(Vector3d(0.0, 0.0, 0.0), box.m_corners[A]);
	ASSERT_EQ(Vector3d(1.0, 0.0, 0.0), box.m_corners[B]);
	ASSERT_EQ(Vector3d(0.0, 1.0, 0.0), box.m_corners[C]);
	ASSERT_EQ(Vector3d(0.0, 0.0, 1.0), box.m_corners[D]);

	ASSERT_EQ(Vector3d(1.0, 1.0, 0.0), box.m_corners[E]);
	ASSERT_EQ(Vector3d(0.0, 1.0, 1.0), box.m_corners[F]);
	ASSERT_EQ(Vector3d(1.0, 0.0, 1.0), box.m_corners[G]);
	ASSERT_EQ(Vector3d(1.0, 1.0, 1.0), box.m_corners[H]);
}

TEST_F(UnitCubeTest, GetClosestCornerToPointTopRightBack)
{
	Vector3d p(2.0, 2.0, 0.0);
	BoxCorner closest = box.GetClosestCornerToPoint(p);
	ASSERT_EQ(E, closest);
}

TEST_F(UnitCubeTest, GetClosestCornerToPointTopRightFront)
{
	Vector3d p(2.0, 2.0, 2.0);
	BoxCorner closest = box.GetClosestCornerToPoint(p);
	ASSERT_EQ(H, closest);
}

TEST_F(UnitCubeTest, GetCenter)
{
	ASSERT_EQ(Vector3d(0.5, 0.5, 0.5), box.GetCenter());
}

TEST_F(UnitCubeTest, GetBoundingRadius)
{
	ASSERT_EQ(sqrt(3.0)/2.0, box.GetBoundingRadius());
}

class RectangleSphereCollisionTest : public ::testing::Test
{
protected:
	RectangleShape side;
	virtual void SetUp()
	{
		Vector3d a(0.0, 0.0, 0.0);
		Vector3d b(10.0, 0.0, 0.0);
		Vector3d c(10.0, 10.0, 0.0);
		Vector3d d(0.0, 10.0, 0.0);
		side = RectangleShape(a, b, c, d);
	}
};

TEST_F(RectangleSphereCollisionTest, CanCreate)
{
	ASSERT_EQ(Vector3d(0.0, 0.0, 0.0), side.m_a);
	ASSERT_EQ(Vector3d(10.0, 0.0, 0.0), side.m_b);
	ASSERT_EQ(Vector3d(10.0, 10.0, 0.0), side.m_c);
	ASSERT_EQ(Vector3d(0.0, 10.0, 0.0), side.m_d);
	ASSERT_EQ(Vector3d(0.0, 0.0, 1.0), side.m_normal);
}

TEST_F(RectangleSphereCollisionTest, CenterOfRectangle)
{
	Vector3d sphere_position(5.0, 5.0, 6.0);
	Vector3d sphere_velocity(0.0, 0.0, -10.0);
	double sphere_radius = 1.0;
	
	Vector3d collision_point;
	double collision_time;
	Vector3d normal;
	bool collision_exists = side.CollideWithSphere(sphere_position, sphere_radius, sphere_velocity, collision_point, collision_time, normal);

	ASSERT_TRUE(collision_exists);
	ASSERT_EQ(0.5, collision_time);
	ASSERT_EQ(Vector3d(5.0, 5.0, 0.0), collision_point);
	ASSERT_EQ(Vector3d(0.0, 0.0, 1.0), normal);

}

TEST_F(RectangleSphereCollisionTest, GrazeLeftSide)
{
	Vector3d sphere_position(-1.0, 5.0, 5.0);
	Vector3d sphere_velocity(0.0, 0.0, -10.0);
	double sphere_radius = 1.0;

	Vector3d collision_point;
	double collision_time;
	Vector3d normal;
	bool collision_exists = side.CollideWithSphere(sphere_position, sphere_radius, sphere_velocity, collision_point, collision_time, normal);

	ASSERT_TRUE(collision_exists);
	ASSERT_EQ(0.5, collision_time);
	ASSERT_EQ(Vector3d(0.0, 5.0, 0.0), collision_point);
	ASSERT_EQ(Vector3d(-1.0, 0.0, 0.0), normal);
}

TEST_F(RectangleSphereCollisionTest, GrazeRightSide)
{
	Vector3d sphere_position(11.0, 5.0, 5.0);
	Vector3d sphere_velocity(0.0, 0.0, -10.0);
	double sphere_radius = 1.0;

	Vector3d collision_point;
	double collision_time;
	Vector3d normal;
	bool collision_exists = side.CollideWithSphere(sphere_position, sphere_radius, sphere_velocity, collision_point, collision_time, normal);

	ASSERT_TRUE(collision_exists);
	ASSERT_EQ(0.5, collision_time);
	ASSERT_EQ(Vector3d(10.0, 5.0, 0.0), collision_point);
	ASSERT_EQ(Vector3d(1.0, 0.0, 0.0), normal);
}

TEST_F(RectangleSphereCollisionTest, GrazeTopSide)
{
	Vector3d sphere_position(5.0, 11.0, 5.0);
	Vector3d sphere_velocity(0.0, 0.0, -10.0);
	double sphere_radius = 1.0;

	Vector3d collision_point;
	double collision_time;
	Vector3d normal;
	bool collision_exists = side.CollideWithSphere(sphere_position, sphere_radius, sphere_velocity, collision_point, collision_time, normal);

	ASSERT_TRUE(collision_exists);
	ASSERT_EQ(0.5, collision_time);
	ASSERT_EQ(Vector3d(5.0, 10.0, 0.0), collision_point);
	ASSERT_EQ(Vector3d(0.0, 1.0, 0.0), normal);
}

TEST_F(RectangleSphereCollisionTest, GrazeBottomSide)
{
	Vector3d sphere_position(5.0, -1.0, 5.0);
	Vector3d sphere_velocity(0.0, 0.0, -10.0);
	double sphere_radius = 1.0;

	Vector3d collision_point;
	double collision_time;
	Vector3d normal;
	bool collision_exists = side.CollideWithSphere(sphere_position, sphere_radius, sphere_velocity, collision_point, collision_time, normal);

	ASSERT_TRUE(collision_exists);
	ASSERT_EQ(0.5, collision_time);
	ASSERT_EQ(Vector3d(5.0, 0.0, 0.0), collision_point);
	ASSERT_EQ(Vector3d(0.0, -1.0, 0.0), normal);
}

class CubeCollisionTest : public ::testing::Test
{
protected:
	BoxShape box;
	virtual void SetUp()
	{
		Vector3d corner(0.0, 0.0, 0.0);
		Vector3d local_x(10.0, 0.0, 0.0);
		Vector3d local_y(0.0, 10.0, 0.0);
		Vector3d local_z(0.0, 0.0, 10.0);
		box = BoxShape(corner, local_x, local_y, local_z);
	}
};

TEST_F(CubeCollisionTest, FrontCenter)
{
	Vector3d sphere_position(5.0, 5.0, 16.0);
	Vector3d sphere_velocity(0.0, 0.0, -10.0);
	double sphere_radius = 1.0;

	Vector3d collision_point;
	double collision_time;
	Vector3d normal;
	bool collision_exists = box.CollideWithSphere(sphere_position, sphere_radius, sphere_velocity, collision_point, collision_time, normal);

	ASSERT_TRUE(collision_exists);
	ASSERT_EQ(0.5, collision_time);
	ASSERT_EQ(Vector3d(5.0, 5.0, 10.0), collision_point);
	ASSERT_EQ(Vector3d(0.0, 0.0, 1.0), normal);
}

TEST_F(CubeCollisionTest, BackCenter)
{
	Vector3d sphere_position(5.0, 5.0, -6.0);
	Vector3d sphere_velocity(0.0, 0.0, 10.0);
	double sphere_radius = 1.0;

	Vector3d collision_point;
	double collision_time;
	Vector3d normal;
	bool collision_exists = box.CollideWithSphere(sphere_position, sphere_radius, sphere_velocity, collision_point, collision_time, normal);

	ASSERT_TRUE(collision_exists);
	ASSERT_EQ(0.5, collision_time);
	ASSERT_EQ(Vector3d(5.0, 5.0, 0.0), collision_point);
	ASSERT_EQ(Vector3d(0.0, 0.0, -1.0), normal);
}

TEST_F(CubeCollisionTest, LeftCenter)
{
	Vector3d sphere_position(-6.0, 5.0, 5.0);
	Vector3d sphere_velocity(10.0, 0.0, 0.0);
	double sphere_radius = 1.0;

	Vector3d collision_point;
	double collision_time;
	Vector3d normal;
	bool collision_exists = box.CollideWithSphere(sphere_position, sphere_radius, sphere_velocity, collision_point, collision_time, normal);

	ASSERT_TRUE(collision_exists);
	ASSERT_EQ(0.5, collision_time);
	ASSERT_EQ(Vector3d(0.0, 5.0, 5.0), collision_point);
	ASSERT_EQ(Vector3d(-1.0, 0.0, 0.0), normal);
}

TEST_F(CubeCollisionTest, RightCenter)
{
	Vector3d sphere_position(16.0, 5.0, 5.0);
	Vector3d sphere_velocity(-10.0, 0.0, 0.0);
	double sphere_radius = 1.0;

	Vector3d collision_point;
	double collision_time;
	Vector3d normal;
	bool collision_exists = box.CollideWithSphere(sphere_position, sphere_radius, sphere_velocity, collision_point, collision_time, normal);

	ASSERT_TRUE(collision_exists);
	ASSERT_EQ(0.5, collision_time);
	ASSERT_EQ(Vector3d(10.0, 5.0, 5.0), collision_point);
	ASSERT_EQ(Vector3d(1.0, 0.0, 0.0), normal);
}

TEST_F(CubeCollisionTest, TopCenter)
{
	Vector3d sphere_position(5.0, 16.0, 5.0);
	Vector3d sphere_velocity(0.0, -10.0, 0.0);
	double sphere_radius = 1.0;

	Vector3d collision_point;
	double collision_time;
	Vector3d normal;
	bool collision_exists = box.CollideWithSphere(sphere_position, sphere_radius, sphere_velocity, collision_point, collision_time, normal);

	ASSERT_TRUE(collision_exists);
	ASSERT_EQ(0.5, collision_time);
	ASSERT_EQ(Vector3d(5.0, 10.0, 5.0), collision_point);
	ASSERT_EQ(Vector3d(0.0, 1.0, 0.0), normal);
}

TEST_F(CubeCollisionTest, BottomCenter)
{
	Vector3d sphere_position(5.0, -6.0, 5.0);
	Vector3d sphere_velocity(0.0, 10.0, 0.0);
	double sphere_radius = 1.0;

	Vector3d collision_point;
	double collision_time;
	Vector3d normal;
	bool collision_exists = box.CollideWithSphere(sphere_position, sphere_radius, sphere_velocity, collision_point, collision_time, normal);

	ASSERT_TRUE(collision_exists);
	ASSERT_EQ(0.5, collision_time);
	ASSERT_EQ(Vector3d(5.0, 0.0, 5.0), collision_point);
	ASSERT_EQ(Vector3d(0.0, -1.0, 0.0), normal);
}