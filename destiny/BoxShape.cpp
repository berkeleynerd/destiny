#include "StdAfx.h"
#include "BoxShape.h"
#include "Collision.h"

RectangleShape::RectangleShape(Vector3d a, Vector3d b, Vector3d c, Vector3d d)
{
	m_a = a;
	m_b = b;
	m_c = c;
	m_d = d;

	Vector3d ab = b - a;
	Vector3d ac = c - a;
	m_normal = ab.Cross(ac).Normalize();
}

bool RectangleShape::CollideWithSphere(Vector3d sphere_position, double sphere_radius, Vector3d sphere_velocity, Vector3d& collision_point, double& collision_time, Vector3d& normal)
{
	Vector3d pa = m_a - sphere_position;
	Vector3d pc = m_c - sphere_position;
	Vector3d m = pc.Cross(pa);
	double v = sphere_velocity * m;
	Triangle triangle(m_a, m_c, m_d);
	if( v <= 0.0 )
	{
		triangle = Triangle(m_a, m_b, m_c);
	}
	return IntersectSphereTriangle(sphere_position, sphere_radius, sphere_velocity, triangle, collision_point, collision_time, normal);
}

BoxShape::BoxShape(Vector3d corner, Vector3d local_x, Vector3d local_y, Vector3d local_z)
{
	m_corners[CORNER_A] = corner;
	m_corners[CORNER_B] = corner + local_x;
	m_corners[CORNER_C] = corner + local_y;
	m_corners[CORNER_D] = corner + local_z;

	m_corners[CORNER_E] = m_corners[CORNER_B] + local_y;
	m_corners[CORNER_F] = m_corners[CORNER_C] + local_z;
	m_corners[CORNER_G] = m_corners[CORNER_B] + local_z;
	m_corners[CORNER_H] = m_corners[CORNER_E] + local_z;

	m_sides[SIDE_FRONT] =  RectangleShape(m_corners[CORNER_H], m_corners[CORNER_F], m_corners[CORNER_D], m_corners[CORNER_G]);
	m_sides[SIDE_BACK] =   RectangleShape(m_corners[CORNER_C], m_corners[CORNER_E], m_corners[CORNER_B], m_corners[CORNER_A]);
	m_sides[SIDE_LEFT] =   RectangleShape(m_corners[CORNER_F], m_corners[CORNER_C], m_corners[CORNER_A], m_corners[CORNER_D]);
	m_sides[SIDE_RIGHT] =  RectangleShape(m_corners[CORNER_E], m_corners[CORNER_H], m_corners[CORNER_G], m_corners[CORNER_B]);
	m_sides[SIDE_TOP] =    RectangleShape(m_corners[CORNER_C], m_corners[CORNER_F], m_corners[CORNER_H], m_corners[CORNER_E]);
	m_sides[SIDE_BOTTOM] = RectangleShape(m_corners[CORNER_B], m_corners[CORNER_G], m_corners[CORNER_D], m_corners[CORNER_A]);

	m_localX = local_x;
	m_localY = local_y;
	m_localZ = local_z;
}

BoxCorner BoxShape::GetClosestCornerToPoint(Vector3d p)
{
	BoxCorner closest_corner = CORNER_A;
	double dist_sq;
	double min_dist_sq;
	min_dist_sq = (m_corners[CORNER_A] - p).LengthSq();
	for(int i = CORNER_B; i < CORNER_COUNT; ++i)
	{
		dist_sq = (m_corners[i] - p).LengthSq();
		if( dist_sq < min_dist_sq )
		{
			min_dist_sq = dist_sq;
			closest_corner = (BoxCorner)i;
		}
	}
	return closest_corner;
}


Vector3d BoxShape::GetClosestPointOnBox( Vector3d p)
{
	Vector3d q = GetCenter();
	Vector3d d = p - q;

	double xHalfLength = m_localX.Length() / 2.0;
	double yHalfLength = m_localY.Length() / 2.0;
	double zHalfLength = m_localZ.Length() / 2.0;

	Vector3d x = m_localX;
	Vector3d y = m_localY;
	Vector3d z = m_localZ;
	x.Normalize();
	y.Normalize();
	z.Normalize();
	
	double dist = d * x;
	if( dist > xHalfLength )
	{
		dist = xHalfLength;
	}
	else if( dist < -xHalfLength )
	{ 
		dist = -xHalfLength;
	}
	q += dist*x;

	dist = d * y;
	if( dist > yHalfLength )
	{
		dist = yHalfLength;
	}
	else if( dist < -yHalfLength )
	{ 
		dist = -yHalfLength;
	}
	q += dist*y;

	dist = d * z;
	if( dist > zHalfLength )
	{
		dist = zHalfLength;
	}
	else if( dist < -zHalfLength )
	{ 
		dist = -zHalfLength;
	}
	q += dist*z;
	return q;
}

static BoxSide CONNECTED_SIDES[8][3] = {
	{SIDE_BACK, SIDE_LEFT, SIDE_BOTTOM}, // A
	{SIDE_BACK, SIDE_RIGHT, SIDE_BOTTOM}, // B
	{SIDE_BACK, SIDE_LEFT, SIDE_TOP}, // C
	{SIDE_FRONT, SIDE_LEFT, SIDE_BOTTOM}, // D
	{SIDE_BACK, SIDE_RIGHT, SIDE_TOP}, // E
	{SIDE_FRONT, SIDE_LEFT, SIDE_TOP}, // F
	{SIDE_FRONT, SIDE_RIGHT, SIDE_BOTTOM}, // G
	{SIDE_FRONT, SIDE_RIGHT, SIDE_TOP}, // H
};

bool BoxShape::CollideWithSphere(Vector3d sphere_position, double sphere_radius, Vector3d sphere_velocity, Vector3d& collision_point, double& collision_time, Vector3d& normal)
{
	BoxCorner closest_corner = GetClosestCornerToPoint(sphere_position);
	BoxSide collision_side;
	bool could_already_be_in_collision = true;
	double shortest_distance = -1.0;
	int closest_halfspace;
	
	for( int i = 0; i < 3; ++i )
	{
		collision_side = CONNECTED_SIDES[closest_corner][i];

		// Check which sides of the plane we are on to see if we might already be inside the box
		// Note that even if we intersect all three planes we might still be at a corner and still not be in collision.
		Vector3d colliding_point_on_sphere = GetPointOnBallThatFirstIntersectsPlane(sphere_position, sphere_radius, m_sides[collision_side].m_normal);
		Vector3d from_sphere = m_sides[collision_side].m_a - colliding_point_on_sphere;
		double halfspace_distance = (m_sides[collision_side].m_normal * from_sphere);
		if( halfspace_distance >= 0.0 )
		{
			if( halfspace_distance < shortest_distance || shortest_distance == -1.0 )
			{
				shortest_distance = halfspace_distance;
				closest_halfspace = collision_side;
			}
		}
		else
		{
			could_already_be_in_collision = false;
			break;
		}

	}
	if( could_already_be_in_collision )
	{
		Vector3d cp = GetClosestPointOnBox(sphere_position);
		normal = (sphere_position - cp);
		double normal_len = normal.Length();
		if( normal_len < sphere_radius )
		{
			// Some part of the sphere is inside the box
			if( normal_len == 0.0 )
			{
				// Our center is inside the box
				normal = m_sides[closest_halfspace].m_normal * shortest_distance;
			}
			else
			{
				// Our center must be outside the box
				double extrication_distance = std::abs(sphere_radius - normal_len);
				normal.Normalize();
				normal = normal * extrication_distance;
			}
			collision_time = 0.0;
			return true;
		}
	}
	
	// Check for collision with the sides.
	for( int i = 0; i < 3; ++i )
	{
		collision_side = CONNECTED_SIDES[closest_corner][i];
		bool collision_exists = m_sides[collision_side].CollideWithSphere(sphere_position, sphere_radius, sphere_velocity, collision_point, collision_time, normal);
		if( collision_exists )
		{
			return true;
		}
	}
	return false;
}

Vector3d BoxShape::GetCenter()
{
	Vector3d x = m_corners[CORNER_H] - m_corners[CORNER_A];
	return m_corners[CORNER_A] + x/2.0;
}

double BoxShape::GetBoundingRadius()
{
	return (m_corners[CORNER_H] - m_corners[CORNER_A]).Length() / 2.0;
}