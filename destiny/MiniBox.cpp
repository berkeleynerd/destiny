#include "StdAfx.h"
#include "MiniBox.h"


//////////////////////////////////////////////////////////////////////
//
// Public member functions
//
//////////////////////////////////////////////////////////////////////

MiniBox::MiniBox(IRoot* lockobj) :
	mId(0)
{
};


//--------------------------------------------------------------------
// Destructor
//--------------------------------------------------------------------
MiniBox::~MiniBox()
{
}


//////////////////////////////////////////////////////////////////////
//
// Python thunkers
//
//////////////////////////////////////////////////////////////////////

PyObject* MiniBox::Py__init__(
	PyObject* args
	)
{
	if (!PyArg_ParseTuple(args, "|dddddddddddd",
		&mCorner.x, &mCorner.y, &mCorner.z,
		&mLocalX.x, &mLocalX.y, &mLocalX.z,
		&mLocalY.x, &mLocalY.y, &mLocalY.z,
		&mLocalZ.x, &mLocalZ.y, &mLocalZ.z))
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}