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
	m_corners[A] = corner;
	m_corners[B] = corner + local_x;
	m_corners[C] = corner + local_y;
	m_corners[D] = corner + local_z;

	m_corners[E] = m_corners[B] + local_y;
	m_corners[F] = m_corners[C] + local_z;
	m_corners[G] = m_corners[B] + local_z;
	m_corners[H] = m_corners[E] + local_z;

	m_sides[FRONT] =  RectangleShape(m_corners[H], m_corners[F], m_corners[D], m_corners[G]);
	m_sides[BACK] =   RectangleShape(m_corners[C], m_corners[E], m_corners[B], m_corners[A]);
	m_sides[LEFT] =   RectangleShape(m_corners[F], m_corners[C], m_corners[A], m_corners[D]);
	m_sides[RIGHT] =  RectangleShape(m_corners[E], m_corners[H], m_corners[G], m_corners[B]);
	m_sides[TOP] =    RectangleShape(m_corners[C], m_corners[F], m_corners[H], m_corners[E]);
	m_sides[BOTTOM] = RectangleShape(m_corners[B], m_corners[G], m_corners[D], m_corners[A]);

	m_localX = local_x;
	m_localY = local_y;
	m_localZ = local_z;
}

BoxCorner BoxShape::GetClosestCornerToPoint(Vector3d p)
{
	BoxCorner closest_corner = A;
	double dist_sq;
	double min_dist_sq;
	min_dist_sq = (m_corners[A] - p).LengthSq();
	for(int i = B; i < CORNER_COUNT; ++i)
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

static BoxSide CONNECTED_SIDES[8][3] = {
	{BACK, LEFT, BOTTOM}, // A
	{BACK, RIGHT, BOTTOM}, // B
	{BACK, LEFT, TOP}, // C
	{FRONT, LEFT, BOTTOM}, // D
	{BACK, RIGHT, TOP}, // E
	{FRONT, LEFT, TOP}, // F
	{FRONT, RIGHT, BOTTOM}, // G
	{FRONT, RIGHT, TOP}, // H
};

bool BoxShape::CollideWithSphere(Vector3d sphere_position, double sphere_radius, Vector3d sphere_velocity, Vector3d& collision_point, double& collision_time, Vector3d& normal)
{
	BoxCorner closest_corner = GetClosestCornerToPoint(sphere_position);
	bool halfspaces[3];
	double shortest_distance = -1.0;
	int closest_halfspace;
	for(int i = 0; i < 3; ++i)
	{
		BoxSide collision_side = CONNECTED_SIDES[closest_corner][i];
		bool collision_exists = m_sides[collision_side].CollideWithSphere(sphere_position, sphere_radius, sphere_velocity, collision_point, collision_time, normal);
		if( collision_exists )
		{
			return true;
		}
		
		// Check which sides of the plane we are on to see if we are already inside the box
		Vector3d colliding_point_on_sphere = GetPointOnBallThatFirstIntersectsPlane(sphere_position, sphere_radius, m_sides[collision_side].m_normal);
		Vector3d to_sphere = colliding_point_on_sphere - m_sides[collision_side].m_a;
		double halfspace_distance = (m_sides[collision_side].m_normal * to_sphere);
		halfspaces[i] = (halfspace_distance < 0.0);
		if ( halfspace_distance > shortest_distance || shortest_distance == -1.0 )
		{
			shortest_distance = halfspace_distance;
			closest_halfspace = collision_side;
		}

	}
	if(halfspaces[0] && halfspaces[1] && halfspaces[2])
	{
		// We are inside the box
		normal = m_sides[closest_halfspace].m_normal * shortest_distance;
		collision_time = 0.0;
		return true;
	}
	return false;
}

Vector3d BoxShape::GetCenter()
{
	Vector3d x = m_corners[H] - m_corners[A];
	return m_corners[A] + x/2.0;
}

double BoxShape::GetBoundingRadius()
{
	return (m_corners[H] - m_corners[A]).Length() / 2.0;
}