#ifndef _MINICAPSULE_H_
#define _MINICAPSULE_H_

#include <Blue.h>
#include "IBall.h"
#include "IDstConstants.h"
#include "Vector3d.h"
#include "DstConstants.h"

typedef int64_t ID;

class MiniCapsule :
	public IRoot
{

public:
	EXPOSE_TO_BLUE();

	MiniCapsule(IRoot* lockobj = NULL);
	~MiniCapsule();

	ID mId;
	ID mParentBallID;
	Vector3d mHemisphereA;
	Vector3d mHemisphereB;
	float mRadius;

public:
	PyObject* Py__init__ ( PyObject* args );

};

TYPEDEF_BLUECLASS(MiniCapsule);

#endif
