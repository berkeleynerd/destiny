// Copyright © 2000 CCP ehf.

#ifndef _MINIBALL_H_
#define _MINIBALL_H_

#include <Blue.h>
#include "IBall.h"
#include "IDstConstants.h"
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
