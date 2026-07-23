// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "MiniBox.h"

const Be::ClassInfo* MiniBox::ExposeToBlue()
{

	EXPOSURE_BEGIN(MiniBox, "")
		MAP_INTERFACE(IRoot)
		MAP_INTERFACE(MiniBox)

		/////////////////////////////////////////////////////////////////////////////
		//               id
		MAP_ATTRIBUTE
		(
			"id",
			mId,
			"ID of object",
			Be::READ
		)

		/////////////////////////////////////////////////////////////////////////////
		//              Corner location in global space
		MAP_ATTRIBUTE
		(
			"c0",
			mCorner.x,
			"The first element in the corner vector",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"c1",
			mCorner.y,
			"The second element in the corner vector",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"c2",
			mCorner.z,
			"The first element in the corner vector",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		/////////////////////////////////////////////////////////////////////////////
		//              local X axis
		MAP_ATTRIBUTE
		(
			"x0",
			mLocalX.x,
			"The first element in the local X axis vector",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"x1",
			mLocalX.y,
			"The second element in the local X axis vector",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"x2",
			mLocalX.z,
			"The third element in the local X axis vector",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		/////////////////////////////////////////////////////////////////////////////
		//              local Y axis
		MAP_ATTRIBUTE
		(
			"y0",
			mLocalY.x,
			"The first element in the local Y axis vector",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"y1",
			mLocalY.y,
			"The second element in the local Y axis vector",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"y2",
			mLocalY.z,
			"The third element in the local Y axis vector",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		/////////////////////////////////////////////////////////////////////////////
		//              local Z axis
		MAP_ATTRIBUTE
		(
			"z0",
			mLocalZ.x,
			"The first element in the local Z axis vector",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"z1",
			mLocalZ.y,
			"The second element in the local Z axis vector",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"z2",
			mLocalZ.z,
			"The third element in the local Z axis vector",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		////////////////////////////////////////////////////////////////////////////
		//               __init__
#if DESTINY_WITH_PYTHON
		MAP_METHOD_AS_METHOD
		(
			"__init__",
			Py__init__,
			"Constructor arguments"
		)
#endif // DESTINY_WITH_PYTHON
		EXPOSURE_END()
}
