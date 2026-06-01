// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Partition.h"
#include "Vector3d.h"

PyObject* PyGetBoxCenter(
	PyObject* module,
	PyObject* args )
{
	Vector3d pos;
	int level;

	if( !PyArg_ParseTuple( args, "iddd", &level, &pos.x, &pos.y, &pos.z ) )
		return NULL;

	Partition partition;
	if( level < 0 || level >= partition.mNumberOfLevels )
	{
		PyErr_SetString( PyExc_TypeError, "Illegal level" );
		return 0;
	}

	partition.GetBoxCenter( level, pos );

	PyObject* ret = PyTuple_New( 3 );
	PyTuple_SET_ITEM( ret, 0, PyFloat_FromDouble( pos.x ) );
	PyTuple_SET_ITEM( ret, 1, PyFloat_FromDouble( pos.y ) );
	PyTuple_SET_ITEM( ret, 2, PyFloat_FromDouble( pos.z ) );

	return ret;
}

MAP_FUNCTION( "GetBoxCenter", PyGetBoxCenter, "Gives you back a partition centered coordinate" );
