// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "MiniBall.h"

const Be::ClassInfo* MiniBall::ExposeToBlue()
{

	EXPOSURE_BEGIN(MiniBall, "")
		MAP_INTERFACE(IRoot)
		MAP_INTERFACE(MiniBall)

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
		//               radius
		MAP_ATTRIBUTE
		(
			"radius",
			mRadius,
			"Radius of ball (m)",
			Be::READWRITE | Be::NOTIFY| Be::PERSIST
		)

		/////////////////////////////////////////////////////////////////////////////
		//              x of ball
		MAP_ATTRIBUTE
		(
			"x",
			mPos.x,
			"x position of ball (m)",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		/////////////////////////////////////////////////////////////////////////////
		//              y of ball
		MAP_ATTRIBUTE
		(
			"y",
			mPos.y,
			"y position of ball (m)",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		/////////////////////////////////////////////////////////////////////////////
		//              z of ball
		MAP_ATTRIBUTE
		(
			"z",
			mPos.z,
			"z position of ball (m)",
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
