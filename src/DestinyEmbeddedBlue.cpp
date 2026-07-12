// Copyright (c) 2026 CCP Games

#include "StdAfx.h"

#include "DestinyEmbedded.h"

#include "Ball.h"
#include "Ballpark.h"
#include "Capsule.h"
#include "MiniBall.h"
#include "MiniBox.h"
#include "MiniCapsule.h"
#include "OrientedBox.h"
#include "Settings.h"

#include <IBlueClasses.h>

#include <cstring>

const char* g_destinyEmbeddedModuleName = "_destiny";

BLUE_DEFINE_INTERFACE( IBall );
BLUE_DEFINE_INTERFACE( IBallpark );

BLUE_DEFINE( Ballpark );
BLUE_DEFINE( Ball );
BLUE_DEFINE( ClientBall );
BLUE_DEFINE( MiniBall );
BLUE_DEFINE( MiniBox );
BLUE_DEFINE( MiniCapsule );
BLUE_DEFINE( Capsule );
BLUE_DEFINE( OrientedBox );
BLUE_DEFINE( GlobalSettings );
BLUE_DEFINE( SettingsConfiguration );

const Be::ClassInfo* Ballpark::ExposeToBlue()
{
	EXPOSURE_BEGIN( Ballpark, "Embedded Destiny Ballpark" )
		MAP_INTERFACE( IEveBallpark )
	EXPOSURE_END()
}

const Be::ClassInfo* Ball::ExposeToBlue()
{
	EXPOSURE_BEGIN( Ball, "Embedded Destiny Ball" )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IBall )
	EXPOSURE_END()
}

const Be::ClassInfo* ClientBall::ExposeToBlue()
{
	EXPOSURE_BEGIN( ClientBall, "Embedded Destiny ClientBall" )
		MAP_INTERFACE( ITriQuaternionFunction )
		MAP_INTERFACE( ITriVectorFunction )
		MAP_INTERFACE( IEveReferencePoint )
	EXPOSURE_CHAINTO( Ball )
}

const Be::ClassInfo* MiniBall::ExposeToBlue()
{
	EXPOSURE_BEGIN( MiniBall, "Embedded Destiny MiniBall" )
	EXPOSURE_END()
}

const Be::ClassInfo* MiniBox::ExposeToBlue()
{
	EXPOSURE_BEGIN( MiniBox, "Embedded Destiny MiniBox" )
	EXPOSURE_END()
}

const Be::ClassInfo* MiniCapsule::ExposeToBlue()
{
	EXPOSURE_BEGIN( MiniCapsule, "Embedded Destiny MiniCapsule" )
	EXPOSURE_END()
}

const Be::ClassInfo* Capsule::ExposeToBlue()
{
	EXPOSURE_BEGIN( Capsule, "Embedded Destiny Capsule" )
	EXPOSURE_END()
}

const Be::ClassInfo* OrientedBox::ExposeToBlue()
{
	EXPOSURE_BEGIN( OrientedBox, "Embedded Destiny OrientedBox" )
	EXPOSURE_END()
}

const Be::ClassInfo* GlobalSettings::ExposeToBlue()
{
	EXPOSURE_BEGIN( GlobalSettings, "Embedded Destiny settings" )
	EXPOSURE_END()
}

const Be::ClassInfo* SettingsConfiguration::ExposeToBlue()
{
	EXPOSURE_BEGIN( SettingsConfiguration, "Embedded Destiny settings configuration" )
	EXPOSURE_END()
}

extern "C" bool Destiny_RegisterBlueClasses( DestinyEmbeddedRegistration* registration )
{
	static bool registered = false;
	static ClassRegsVector destinyClasses;
	DestinyEmbeddedRegistration local = {};
	if( !BeClasses )
	{
		if( registration )
			*registration = local;
		return false;
	}

	if( destinyClasses.empty() )
	{
		for( const Be::ClassRegistration& candidate : BlueRegistration::GetClassRegs() )
		{
			if( candidate.mType && candidate.mType->mClassId &&
				std::strcmp( candidate.mType->mClassId->GetModule(), "_destiny" ) == 0 )
			{
				destinyClasses.push_back( candidate );
			}
		}
	}
	local.discoveredClassCount = static_cast<uint32_t>( destinyClasses.size() );
	local.alreadyRegistered = registered;
	local.valid = local.discoveredClassCount == 10;
	for( const Be::ClassRegistration& candidate : destinyClasses )
		local.valid = local.valid && candidate.mType && candidate.mType->mClassId && candidate.mCreateFn;
	if( local.valid && !registered )
	{
		BeClasses->RegisterClasses( destinyClasses );
		local.newlyRegisteredClassCount = local.discoveredClassCount;
		registered = true;
	}
	if( registration )
		*registration = local;
	return local.valid;
}
