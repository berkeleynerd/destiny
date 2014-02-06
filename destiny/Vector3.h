/* 
	*************************************************************************************

	Vector3.h

	Creator:	Halldor Fannar
	Created:	2008-11-11
	Project:	Destiny

	Bare bones Vector3 support for Destiny.

	(c) CCP 2008

	*************************************************************************************
*/

#pragma once
#ifndef VECTOR3_H
#define VECTOR3_H

#define BLUE_OVERRIDE_VECTOR_TYPES 1

struct Vector3 
{ 
	Vector3() {}
	Vector3( float vx, float vy, float vz ) : x(vx), y(vy), z(vz) {}
	operator float* () { return &x; }
	operator const float* () const { return &x; }
	float x, y, z; 
};

#endif