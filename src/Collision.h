#ifndef _COLLISION_H_
#define _COLLISION_H_

#include "Plane.h"
#include "Triangle.h"

Vector3d GetPointOnBallThatFirstIntersectsPlane(Vector3d ballPos, double ballRadius, Vector3d planeNormal);
bool IntersectSegmentPlane(Vector3d p0, Vector3d p1, Planed plane, Vector3d & collisionPoint, double & collisionTime);
bool IntersectSegmentSphere(Vector3d p0, Vector3d p1, Vector3d spherePos, double sphereRad, Vector3d & collisionPoint, double & collisionTime);
bool IntersectSphereTriangle(Vector3d spherePosition, double sphereRadius, Vector3d sphereVelocity, Triangle triangle, Vector3d & collisionPoint, double & collisionTime, Vector3d& normal);

// -------------------------------------------------------------
// Description:
//   Finds two solutions (roots) of a quadratic equation using
//   the quadradric formula a*x**2 + b*x + c == 0
//   https://en.wikipedia.org/wiki/Quadratic_formula
//   The roots represent the values at which the parabola
//   defined by the equation crosses the X axis.
// Arguments:
//   v1 (out) - The first solution
//   v2 (out) - The second solution
//   a - The a (quadratic) component of the quadratic equation
//   b - The b (linear) component of the quadratic equation
//   c - The c (constant) component of the quadratic equation
// Return Value:
//   True if solutions are found.
// -------------------------------------------------------------
bool Quadratic(double& v1, double& v2, double a, double b, double c);
// -------------------------------------------------------------
// Description:
//   Finds the time of collision between two spheres p and q
// Arguments:
//   p0 - The location of sphere p at the start of the tick
//   p1 - The location of sphere p at the end of the tick
//   q0 - The location of sphere q at the start of the tick
//   q1 - The location of sphere q at the end of the tick
//   collRadius - The combined radius of the two spheres
// Return Value:
//   The point in time during the time interval at which
//   collision occurs, or -1.0 if there is no collision
// -------------------------------------------------------------
double CollideTwoSpheres(const Vector3d& p0, const Vector3d& p1, const Vector3d& q0, const Vector3d& q1, const double collRadius);

#endif