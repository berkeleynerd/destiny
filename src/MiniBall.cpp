// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "MiniBall.h"


//////////////////////////////////////////////////////////////////////
//
// Public member functions
//
//////////////////////////////////////////////////////////////////////

MiniBall::MiniBall(IRoot* lockobj) :
	mRadius(0.0f),
	mId(0)
{
};


//--------------------------------------------------------------------
// Destructor
//--------------------------------------------------------------------
MiniBall::~MiniBall()
{
}


//////////////////////////////////////////////////////////////////////
//
// Python thunkers
//
//////////////////////////////////////////////////////////////////////

#if DESTINY_WITH_PYTHON
PyObject* MiniBall::Py__init__(
	PyObject* args
	)
{
	if (!PyArg_ParseTuple(args, "|dddd",&mRadius, &mPos.x, &mPos.y, &mPos.z))
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}
#endif // DESTINY_WITH_PYTHON
