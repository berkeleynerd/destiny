#ifndef _PLANE_H_
#define _PLANE_H_

#include "Vector3d.h"

class Planed
{
public:
	Planed(Vector3d point, Vector3d normal);
	double d;
	Vector3d normal;
};

#endif