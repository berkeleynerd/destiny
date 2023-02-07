#include "StdAfx.h"
#include "DstConstants.h"


void AddDstConstants(PyObject* d)
{
	Be::VarChooser* dstConstants [] = 
	{	
		DstBallMode,
		DstEventTypeChooser,
		DstConstants
	};

	for (int i = 0; i < sizeof dstConstants / sizeof dstConstants[0]; i++)
	{
		for (Be::VarChooser* j = dstConstants[i]; j->mKey; j++)
		{
			// this obviously assumes that all values are LONG
			// later perhaps call BlueToPython to convert types
			// correctly
			PyObject *value = PyInt_FromLong( j->mValue.mLong );
			PyDict_SetItemString(d, const_cast<char*>( j->mKey ), value );
			Py_DECREF( value );
		}
	}
}


const char* KeyFromVal(const Be::VarChooser* i, long val)
{
	while (i->mKey)
	{
		if(i->mValue.mLong == val)
			return i->mKey;
		i++;
	}

	return "[-not found-]";
}