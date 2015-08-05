#include "StdAfx.h"
#include "Collision.h"

Vector3d GetPointOnBallThatFirstIntersectsPlane(Vector3d ball_pos, double ball_radius, Vector3d plane_normal)
{
	return ball_pos - plane_normal * ball_radius;
}

bool IntersectSegmentPlane(Vector3d p0, Vector3d p1, Plane plane, Vector3d& collision_point, double& collision_time)
{
	Vector3d p0p1 = p1 - p0;
	double plane_normal_dot_ab = plane.normal * p0p1;
	if( plane_normal_dot_ab == 0.0 )
	{
		// The line segment is parallel to the plane
		return false;
	}
	collision_time = (plane.d - plane.normal * p0) / plane_normal_dot_ab;
	if( collision_time >= 0.0 && collision_time <= 1.0 )
	{
		collision_point = p0 + p0p1 * collision_time;
		return true;
	}
	return false;
}

bool IntersectSegmentSphere(Vector3d p0, Vector3d p1, Vector3d sphere_pos, double sphere_rad, Vector3d& collision_point, double& collision_time)
{
	Vector3d direction = p1 - p0;
	Vector3d normalized_direction = direction;
	normalized_direction.Normalize();

	Vector3d m = p0 - sphere_pos;
	double b = m * normalized_direction;
	double c = m * m - sphere_rad * sphere_rad;

	if( c > 0.0 && b > 0.0 )
	{
		return false;
	}

	double discriminant = b*b - c;
	if( discriminant < 0.0 )
	{
		return false;
	}

	collision_time = -b - sqrt(discriminant);
	if( collision_time < 0.0 )
	{
		collision_time = 0.0;
	}
	else
	{
		collision_time /= direction.Length();
	}
	if( collision_time > 1.0 )
	{
		return false;
	}
	collision_point = p0 + direction * collision_time;
	return true;
}

bool IntersectSphereTriangle(Vector3d sphere_position, double sphere_radius, Vector3d sphere_velocity, Triangle triangle, Vector3d& collision_point, double& collision_time, Vector3d&normal)
{
	Vector3d triangle_normal = triangle.GetNormal();
	Vector3d p0 = GetPointOnBallThatFirstIntersectsPlane(sphere_position, sphere_radius, triangle_normal);
	Vector3d p1 = p0 + sphere_velocity;
	Plane plane(triangle.a, triangle_normal);

	bool collision_exists = IntersectSegmentPlane(p0, p1, plane, collision_point, collision_time);
	if( !collision_exists )
	{
		return false;
	}
	if( triangle.ContainsPoint(collision_point) )
	{
		normal = triangle_normal;
		return true;
	}

	// The collision is not inside the triangle. If there is a collision, it will be on the point on the triangle that
	// is closest to the collision point.

	collision_point = triangle.GetClosestPoint(collision_point);
	
	// Cast a ray from the possible intersection point towards the sphere to find out if the latter intersects
	// the former.

	p0 = collision_point;
	p1 = collision_point - sphere_velocity;
	normal = -sphere_velocity;
	Vector3d impactPointOnSphere;
	collision_exists = IntersectSegmentSphere(p0, p1, sphere_position, sphere_radius, impactPointOnSphere, collision_time);
	if( !collision_exists )
	{
		return false;
	}
	normal = (sphere_position - impactPointOnSphere);
	normal.Normalize();
	return true;
	
}