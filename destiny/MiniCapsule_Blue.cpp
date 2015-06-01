#include "StdAfx.h"
#include "MiniCapsule.h"

const Be::ClassInfo* MiniCapsule::ExposeToBlue()
{

	EXPOSURE_BEGIN(MiniCapsule, "")
		MAP_INTERFACE(IRoot)
		MAP_INTERFACE(MiniCapsule)

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
		//              x of the center of hemisphere A
		MAP_ATTRIBUTE
		(
		"ax",
		mHemisphereA.x,
		"x position of capsule hemisphere A",
		Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		/////////////////////////////////////////////////////////////////////////////
		//              y of the center of hemisphere A
		MAP_ATTRIBUTE
		(
		"ay",
		mHemisphereA.y,
		"y position of capsule hemisphere A",
		Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		/////////////////////////////////////////////////////////////////////////////
		//              z of the center of hemisphere A
		MAP_ATTRIBUTE
		(
		"az",
		mHemisphereA.z,
		"z position of capsule hemisphere A",
		Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)
		
		/////////////////////////////////////////////////////////////////////////////
		//              x of the center of hemisphere B
		MAP_ATTRIBUTE
		(
		"bx",
		mHemisphereB.x,
		"x position of capsule hemisphere B",
		Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		/////////////////////////////////////////////////////////////////////////////
		//              y of the center of hemisphere B
		MAP_ATTRIBUTE
		(
		"by",
		mHemisphereB.y,
		"y position of capsule hemisphere B",
		Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		/////////////////////////////////////////////////////////////////////////////
		//              z of the center of hemisphere B
		MAP_ATTRIBUTE
		(
		"bz",
		mHemisphereB.z,
		"z position of capsule hemisphere B (m)",
		Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		////////////////////////////////////////////////////////////////////////////
		//               __init__
		MAP_METHOD_AS_METHOD
		(
		"__init__",
		Py__init__,
		"Constructor arguments"
		)
		EXPOSURE_END()
}
