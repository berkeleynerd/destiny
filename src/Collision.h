#ifndef _COLLISION_H_
#define _COLLISION_H_

#include "Plane.h"
#include "Triangle.h"

Vector3d GetPointOnBallThatFirstIntersectsPlane(Vector3d ball_pos, double ball_radius, Vector3d plane_normal);
bool IntersectSegmentPlane(Vector3d p0, Vector3d p1, Planed plane, Vector3d& collision_point, double& collision_time);
bool IntersectSegmentSphere(Vector3d p0, Vector3d p1, Vector3d sphere_pos, double sphere_rad, Vector3d& colllision_point, double& collision_time);
bool IntersectSphereTriangle(Vector3d sphere_position, double sphere_radius, Vector3d sphere_velocity, Triangle triangle, Vector3d& collision_point, double& collision_time, Vector3d& normal);

#endif