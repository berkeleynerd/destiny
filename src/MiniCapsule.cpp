// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "MiniCapsule.h"


//////////////////////////////////////////////////////////////////////
//
// Public member functions
//
//////////////////////////////////////////////////////////////////////

MiniCapsule::MiniCapsule(IRoot* lockobj) :
mRadius(0.0f),
	mId(0)
{
};


//--------------------------------------------------------------------
// Destructor
//--------------------------------------------------------------------
MiniCapsule::~MiniCapsule()
{
}


//////////////////////////////////////////////////////////////////////
//
// Python thunkers
//
//////////////////////////////////////////////////////////////////////

#if DESTINY_WITH_PYTHON
PyObject* MiniCapsule::Py__init__(
	PyObject* args
	)
{
	if (!PyArg_ParseTuple(args, "|ddddddd",
		&mRadius, 
		&mHemisphereA.x, &mHemisphereA.y, &mHemisphereA.z, 
		&mHemisphereB.x, &mHemisphereB.y, &mHemisphereB.z))
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}
#endif // DESTINY_WITH_PYTHON
