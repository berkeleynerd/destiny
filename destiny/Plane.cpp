#include "StdAfx.h"
#include "Plane.h"

Planed::Planed(Vector3d point, Vector3d normal)
{
	d = point * normal;
	this->normal = normal;
}