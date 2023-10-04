#include "StdAfx.h"
#include "Collision.h"

Vector3d GetPointOnBallThatFirstIntersectsPlane(Vector3d ballPos, double ballRadius, Vector3d planeNormal)
{
	return ballPos - planeNormal * ballRadius;
}

bool IntersectSegmentPlane(Vector3d p0, Vector3d p1, Planed plane, Vector3d & collisionPoint, double & collisionTime)
{
	Vector3d p0p1 = p1 - p0;
	double planeNormalDotAB = plane.normal * p0p1;
	if( planeNormalDotAB == 0.0 )
	{
		// The line segment is parallel to the plane
		return false;
	}
	collisionTime = (plane.d - plane.normal * p0) / planeNormalDotAB;
	if( collisionTime >= 0.0 && collisionTime <= 1.0 )
	{
		collisionPoint = p0 + p0p1 * collisionTime;
		return true;
	}
	return false;
}

bool IntersectSegmentSphere(Vector3d p0, Vector3d p1, Vector3d spherePos, double sphereRad, Vector3d & collisionPoint, double & collisionTime)
{
	Vector3d direction = p1 - p0;
	Vector3d normalizedDirection = direction;
	normalizedDirection.Normalize();

	Vector3d m = p0 - spherePos;
	double b = m * normalizedDirection;
	double c = m * m - sphereRad * sphereRad;

	if( c > 0.0 && b > 0.0 )
	{
		return false;
	}

	double discriminant = b*b - c;
	if( discriminant < 0.0 )
	{
		return false;
	}

	collisionTime = -b - sqrt(discriminant);
	if( collisionTime < 0.0 )
	{
		collisionTime = 0.0;
	}
	else
	{
		collisionTime /= direction.Length();
	}
	if( collisionTime > 1.0 )
	{
		return false;
	}
	collisionPoint = p0 + direction * collisionTime;
	return true;
}

bool IntersectSphereTriangle(Vector3d spherePosition, double sphereRadius, Vector3d sphereVelocity, Triangle triangle, Vector3d & collisionPoint, double & collisionTime, Vector3d&normal)
{
	Vector3d triangleNormal = triangle.GetNormal();
	Vector3d p0 = GetPointOnBallThatFirstIntersectsPlane(spherePosition, sphereRadius, triangleNormal);
	Vector3d p1 = p0 + sphereVelocity;
	Planed plane(triangle.a, triangleNormal);

	bool collisionExists = IntersectSegmentPlane(p0, p1, plane, collisionPoint, collisionTime);
	if( !collisionExists )
	{
		return false;
	}
	if( triangle.ContainsPoint(collisionPoint) )
	{
		normal = triangleNormal;
		return true;
	}

	// The collision is not inside the triangle. If there is a collision, it will be on the point on the triangle that
	// is closest to the collision point.

	collisionPoint = triangle.GetClosestPoint(collisionPoint);
	
	// Cast a ray from the possible intersection point towards the sphere to find out if the latter intersects
	// the former.

	p0 = collisionPoint;
	p1 = collisionPoint - sphereVelocity;
	normal = -sphereVelocity;
	Vector3d impactPointOnSphere;
	collisionExists = IntersectSegmentSphere(p0, p1, spherePosition, sphereRadius, impactPointOnSphere, collisionTime);
	if( !collisionExists )
	{
		return false;
	}
	normal = (spherePosition - impactPointOnSphere);
	normal.Normalize();
	return true;
	
}

double CollideTwoSpheres(const Vector3d& p0, const Vector3d& p1, const Vector3d& q0, const Vector3d& q1, const double collRadius)
{
	Vector3d p0q0 = p0 - q0;

	double p0q0_2 = p0q0 * p0q0;

	// We are already in collision, return current time as collision time
	if (p0q0_2 <= collRadius * collRadius)
		return 0.0;

	Vector3d dpq = (p1 - p0) - (q1 - q0);
	double dpq_2 = dpq * dpq;
	double p0q0dpq = p0q0 * dpq;
	double s1, s2 = -1.0;

	if (Quadratic(s1, s2, dpq_2, 2.0 * p0q0dpq, p0q0_2 - collRadius * collRadius))
	{
		// We have a bona fide collision. Just a question of when
		// Use the fact that s2 <= s1, always, and that we are not starting in overlap, meaning the collisions are both positive or both negative.
		if (s2 >= 0.0 && s2 <= 1.0)
		{
			return s2;
		}
	}

	// No collision, return -1.0
	return -1.0;
}

bool Quadratic(double& v1, double& v2, double a, double b, double c)
{
	if (a == 0.0)
	{
		//Probably never happens, but we might have a linear equation
		if (b == 0.0)
			return false;
		else
		{
			v1 = v2 = -c / b;
		}
	}

	double det = b * b - 4.0 * a * c;
	if (det < 0.0)
		return false;

	det = std::sqrt(det);
	v1 = (-b + det) * 0.5 / a;
	v2 = (-b - det) * 0.5 / a;

	return true;
}
