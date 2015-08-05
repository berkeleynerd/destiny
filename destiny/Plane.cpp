#include "StdAfx.h"
#include "Plane.h"

Plane::Plane(Vector3d point, Vector3d normal)
{
	d = point * normal;
	this->normal = normal;
}