#ifndef _MINIBox_H_
#define _MINIBox_H_

#include <blue/Include/Blue.h>
#include "include/IBall.h"
#include "include/IDstConstants.h"
#include "Vector3d.h"
#include "DstConstants.h"

typedef int64_t ID;

class MiniBox :
	public IRoot
{

public:
	EXPOSE_TO_BLUE();

	MiniBox(IRoot* lockobj = NULL);
	~MiniBox();

	ID mId;
	ID mParentBallID;
	Vector3d mCorner;
	Vector3d mLocalX;
	Vector3d mLocalY;
	Vector3d mLocalZ;

public:
	PyObject* Py__init__ ( PyObject* args );

};

TYPEDEF_BLUECLASS(MiniBox);

#endif
