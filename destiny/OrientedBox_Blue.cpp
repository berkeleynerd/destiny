#include "StdAfx.h"
#include "OrientedBox.h"

#include "StdAfx.h"
#include "Capsule.h"

BLUE_DEFINE( OrientedBox );

const Be::ClassInfo* OrientedBox::ExposeToBlue()
{

	EXPOSURE_BEGIN( OrientedBox, "A box shaped collision object" )
		MAP_INTERFACE( IRoot )
		MAP_INTERFACE( OrientedBox )

		MAP_ATTRIBUTE( "id", mId, "The ID of the box", Be::READWRITE )
		MAP_ATTRIBUTE( "park", mPark, "The ballpark", Be::READ )
		MAP_ATTRIBUTE
		(
			"corner_x",
			m_boxShape.m_corners[A].x,
			"x position of the origin corner in global space",
			Be::READ
		)
		MAP_ATTRIBUTE("isMoribund",isMoribund,"True if box is moribund",Be::READ)

	EXPOSURE_END()
}