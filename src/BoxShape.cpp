// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "BoxShape.h"
#include "Collision.h"
#include <array>

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

bool RectangleShape::CollideWithSphere(Vector3d spherePosition, double sphereRadius, Vector3d sphereVelocity, Vector3d & collisionPoint, double & collisionTime, Vector3d& normal)
{
	Vector3d pa = m_a - spherePosition;
	Vector3d pc = m_c - spherePosition;
	Vector3d m = pc.Cross(pa);
	double v = sphereVelocity * m;
	Triangle triangle(m_a, m_c, m_d);
	if (v <= 0.0)
	{
		triangle = Triangle(m_a, m_b, m_c);
	}
	return IntersectSphereTriangle(spherePosition, sphereRadius, sphereVelocity, triangle, collisionPoint, collisionTime, normal);
}

BoxShape::BoxShape(Vector3d corner, Vector3d localX, Vector3d localY, Vector3d localZ)
{
	m_corners[CORNER_A] = corner;
	m_corners[CORNER_B] = corner + localX;
	m_corners[CORNER_C] = corner + localY;
	m_corners[CORNER_D] = corner + localZ;

	m_corners[CORNER_E] = m_corners[CORNER_B] + localY;
	m_corners[CORNER_F] = m_corners[CORNER_C] + localZ;
	m_corners[CORNER_G] = m_corners[CORNER_B] + localZ;
	m_corners[CORNER_H] = m_corners[CORNER_E] + localZ;

	m_sides[SIDE_FRONT] = RectangleShape(m_corners[CORNER_H], m_corners[CORNER_F], m_corners[CORNER_D], m_corners[CORNER_G]);
	m_sides[SIDE_BACK] = RectangleShape(m_corners[CORNER_C], m_corners[CORNER_E], m_corners[CORNER_B], m_corners[CORNER_A]);
	m_sides[SIDE_LEFT] = RectangleShape(m_corners[CORNER_F], m_corners[CORNER_C], m_corners[CORNER_A], m_corners[CORNER_D]);
	m_sides[SIDE_RIGHT] = RectangleShape(m_corners[CORNER_E], m_corners[CORNER_H], m_corners[CORNER_G], m_corners[CORNER_B]);
	m_sides[SIDE_TOP] = RectangleShape(m_corners[CORNER_C], m_corners[CORNER_F], m_corners[CORNER_H], m_corners[CORNER_E]);
	m_sides[SIDE_BOTTOM] = RectangleShape(m_corners[CORNER_B], m_corners[CORNER_G], m_corners[CORNER_D], m_corners[CORNER_A]);

	m_localX = localX;
	m_localY = localY;
	m_localZ = localZ;
}

BoxCorner BoxShape::GetClosestCornerToPoint(Vector3d p)
{
	BoxCorner closestCorner = CORNER_A;
	double distSq;
	double minDistSq;
	minDistSq = (m_corners[CORNER_A] - p).LengthSq();
	for (int i = CORNER_B; i < CORNER_COUNT; ++i)
	{
		distSq = (m_corners[i] - p).LengthSq();
		if (distSq < minDistSq)
		{
			minDistSq = distSq;
			closestCorner = (BoxCorner)i;
		}
	}
	return closestCorner;
}


Vector3d BoxShape::GetClosestPointOnBox(Vector3d p)
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
	if (dist > xHalfLength)
	{
		dist = xHalfLength;
	}
	else if (dist < -xHalfLength)
	{
		dist = -xHalfLength;
	}
	q += dist * x;

	dist = d * y;
	if (dist > yHalfLength)
	{
		dist = yHalfLength;
	}
	else if (dist < -yHalfLength)
	{
		dist = -yHalfLength;
	}
	q += dist * y;

	dist = d * z;
	if (dist > zHalfLength)
	{
		dist = zHalfLength;
	}
	else if (dist < -zHalfLength)
	{
		dist = -zHalfLength;
	}
	q += dist * z;
	return q;
}

bool BoxShape::RayIntersect(const Vector3d origin, const Vector3d ray, BoxSide& side1, double& d1, BoxSide& side2, double& d2)
{
	//Use the three slabs method of Kay and Kajiya (1986)
	//http://what-when-how.com/advanced-methods-in-computer-graphics/collision-detection-advanced-methods-in-computer-graphics-part-3/
	//Basically find the "times" the ray is between the 6 planes defining the box, making sure they intersect

	//Use some vectors to simplify the algorithm
	const std::array<const Vector3d, 3> boxVectors = { m_localX, m_localY, m_localZ };
	std::array<double, 3> tmin;
	std::array<double, 3> tmax;
	std::array<char, 3> revert;
	static const std::array<std::array<BoxSide, 2>, 3> sideMapping = {
		std::array<BoxSide, 2>({ SIDE_LEFT, SIDE_RIGHT }),
		std::array<BoxSide, 2>({ SIDE_BOTTOM, SIDE_TOP }),
		std::array<BoxSide, 2>({ SIDE_BACK, SIDE_FRONT })
	};

	//Transform origin to box center
	const Vector3d originInBoxFrame = origin - GetCenter();

	for (size_t i = 0; i < 3; ++i)
	{
		revert[i] = 0;
		const auto armLengthSq = boxVectors[i].LengthSq();
		const auto armOrigin = boxVectors[i] * originInBoxFrame;
		const auto armRay = boxVectors[i] * ray;

		if (armRay < 0)
		{
			revert[i] = 1;
			tmin[i] = (0.5 * armLengthSq - armOrigin) / armRay;
			tmax[i] = (-0.5 * armLengthSq - armOrigin) / armRay;
		}
		else if (armRay > 0)
		{
			tmin[i] = (-0.5 * armLengthSq - armOrigin) / armRay;
			tmax[i] = (0.5 * armLengthSq - armOrigin) / armRay;
		}
		else
		{
			tmin[i] = std::numeric_limits<double>::lowest();
			tmax[i] = std::numeric_limits<double>::max();
		}
	}

	size_t indexMin = std::max_element(tmin.begin(), tmin.end()) - tmin.begin();
	size_t indexMax = std::min_element(tmax.begin(), tmax.end()) - tmax.begin();


	d1 = tmin[indexMin];
	d2 = tmax[indexMax];

	if (d1 < d2)
	{
		side1 = sideMapping[indexMin][0 + revert[indexMin]];
		side2 = sideMapping[indexMin][1 - revert[indexMin]];
		return true;
	}
	return false;
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

bool BoxShape::CollideWithSphere(Vector3d spherePosition, double sphereRadius, Vector3d sphereVelocity, Vector3d & collisionPoint, double & collisionTime, Vector3d& normal)
{
	BoxCorner closestCorner = GetClosestCornerToPoint( spherePosition );
	BoxSide collisionSide;
	bool couldAlreadyBeInCollision = true;
	double shortestDistance = -1.0;
	int closestHalfspace;

	for (int i = 0; i < 3; ++i)
	{
		collisionSide = CONNECTED_SIDES[closestCorner][i];

		// Check which sides of the plane we are on to see if we might already be inside the box
		// Note that even if we intersect all three planes we might still be at a corner and still not be in collision.
		Vector3d collidingPointOnSphere = GetPointOnBallThatFirstIntersectsPlane( spherePosition, sphereRadius, m_sides[collisionSide].m_normal );
		Vector3d fromSphere = m_sides[collisionSide].m_a - collidingPointOnSphere;
		double halfspaceDistance = (m_sides[collisionSide].m_normal * fromSphere);
		if (halfspaceDistance >= 0.0)
		{
			if (halfspaceDistance < shortestDistance || shortestDistance == -1.0)
			{
				shortestDistance = halfspaceDistance;
				closestHalfspace = collisionSide;
			}
		}
		else
		{
			couldAlreadyBeInCollision = false;
			break;
		}

	}
	if (couldAlreadyBeInCollision)
	{
		Vector3d cp = GetClosestPointOnBox(spherePosition);
		normal = (spherePosition - cp);
		double normalLen = normal.Length();
		if (normalLen < sphereRadius)
		{
			// Some part of the sphere is inside the box
			if (normalLen < 1e-10)
			{
				// Our center is inside the box, we want it to go out the same way it entered, if possible
				const auto sphereSpeed = sphereVelocity.Length();
				if (sphereSpeed < 1e-10)
				{
					// No direction, just pick the closest side.
					normal = m_sides[closestHalfspace].m_normal * std::max(shortestDistance, 1e-5);
				}
				else
				{
					// Find the closest side along the ray
					double d1, d2;
					BoxSide side1, side2;
					if (RayIntersect(spherePosition, sphereVelocity, side1, d1, side2, d2))
					{
						if (std::fabs(d1) < std::fabs(d2))
						{
							normal = m_sides[side1].m_normal * std::max(std::fabs(d1) * sphereSpeed, 1e-5);
						}
						else
						{
							normal = m_sides[side2].m_normal * std::max(std::fabs(d2) * sphereSpeed, 1e-5);
						}
					}
					else
					{
						// THIS SHOULD NOT HAPPEN, BUT RETURN THE CLOSEST SIDE HERE ANYWAY
						normal = m_sides[closestHalfspace].m_normal * shortestDistance;
					}
				}
			}
			else
			{
				// Our center must be outside the box
				double extricationDistance = std::abs(sphereRadius - normalLen);
				normal.Normalize();
				normal = normal * extricationDistance;
			}
			collisionTime = 0.0;
			return true;
		}
	}

	// Check for collision with the sides.
	for (int i = 0; i < 3; ++i)
	{
		collisionSide = CONNECTED_SIDES[closestCorner][i];
		bool collisionExists = m_sides[collisionSide].CollideWithSphere(spherePosition, sphereRadius, sphereVelocity, collisionPoint, collisionTime, normal);
		if (collisionExists)
		{
			return true;
		}
	}
	return false;
}

Vector3d BoxShape::GetCenter()
{
	Vector3d x = m_corners[CORNER_H] - m_corners[CORNER_A];
	return m_corners[CORNER_A] + x / 2.0;
}

double BoxShape::GetBoundingRadius()
{
	return (m_corners[CORNER_H] - m_corners[CORNER_A]).Length() / 2.0;
}