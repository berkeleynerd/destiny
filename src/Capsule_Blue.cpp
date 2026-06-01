// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "Capsule.h"

BLUE_DEFINE( Capsule );

const Be::ClassInfo* Capsule::ExposeToBlue()
{

	EXPOSURE_BEGIN( Capsule, "A capsule shaped collision object" )
		MAP_INTERFACE( IRoot )
		MAP_INTERFACE( Capsule )

		MAP_ATTRIBUTE( "id", mId, "The ID of the capsule", Be::READWRITE )
		MAP_ATTRIBUTE( "ax", mHemisphereA.x, "The x value of the location of the center of the sphere on side A of the capsule", Be::READWRITE )
		MAP_ATTRIBUTE( "ay", mHemisphereA.y, "The y value of the location of the center of the sphere on side A of the capsule", Be::READWRITE )
		MAP_ATTRIBUTE( "az", mHemisphereA.z, "The z value of the location of the center of the sphere on side A of the capsule", Be::READWRITE )
		MAP_ATTRIBUTE( "bx", mHemisphereB.x, "The x value of the location of the center of the sphere on side B of the capsule", Be::READWRITE )
		MAP_ATTRIBUTE( "by", mHemisphereB.y, "The y value of the location of the center of the sphere on side B of the capsule", Be::READWRITE )
		MAP_ATTRIBUTE( "bz", mHemisphereB.z, "The z value of the location of the center of the sphere on side B of the capsule", Be::READWRITE )
		MAP_ATTRIBUTE( "radius", mRadius, "The radius of the capsule", Be::READWRITE )
		MAP_ATTRIBUTE( "park", mPark, "The ballpark", Be::READ )

		MAP_ATTRIBUTE("isMoribund",isMoribund,"True if capsule is moribund",Be::READ)

	EXPOSURE_END()
}