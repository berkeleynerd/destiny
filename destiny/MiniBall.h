/*
	*************************************************************************

	Ball.h

	Author:    Kjartan Pierre Emilsson
	Created:   Nov. 2001
	OS:        Win32
	Project:   Destiny

	Description:

		Destiny schmestiny sample object


	Dependencies:

		Blue

	(c) CCP 2000, 2001, 2002

	*************************************************************************
*/

#ifndef _MINIBALL_H_
#define _MINIBALL_H_

#include <blue/include/blue.h>
#include "include/IBall.h"
#include "include/IDstConstants.h"
#include "Vector3d.h"
#include "DstConstants.h"

typedef int64_t ID;

class MiniBall :
	public IRoot
{

public:
	EXPOSE_TO_BLUE();

	MiniBall(IRoot* lockobj = NULL);
	~MiniBall();

	ID mId;
	float mRadius;
	Vector3d mPos;

public:
	PyObject* Py__init__ ( PyObject* args );

};

TYPEDEF_BLUECLASS(MiniBall);

#endif
