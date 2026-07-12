// Copyright (c) 2026 CCP Games

#include "StdAfx.h"

#include "DestinyEmbedded.h"
#include "DstConstants.h"

#include <Blue.h>

#include <cmath>
#include <cstdio>

const char* g_moduleName = "DestinyEmbeddedContractTest";

BLUE_DEFINE_INTERFACE( ITriVectorFunction );
BLUE_DEFINE_INTERFACE( ITriQuaternionFunction );
BLUE_DEFINE_INTERFACE( IEveReferencePoint );
BLUE_DEFINE_INTERFACE( IEveBallpark );

namespace
{
bool Near( double a, double b, double epsilon = 1e-5 )
{
	return std::abs( a - b ) <= epsilon;
}

int Fail( const char* message )
{
	std::fprintf( stderr, "DestinyEmbeddedContractTest: %s\n", message );
	return 1;
}
}

int main()
{
	if( !Py_IsInitialized() )
		Py_Initialize();
	BlueModuleStartup();
	if( !BeClasses )
		return Fail( "Blue startup did not initialize BeClasses" );

	DestinyEmbeddedRegistration registration = {};
	if( !Destiny_RegisterBlueClasses( &registration ) || registration.discoveredClassCount != 10 ||
		registration.newlyRegisteredClassCount != 10 || registration.alreadyRegistered )
	{
		return Fail( "first registration did not install exactly ten Destiny classes" );
	}
	DestinyEmbeddedRegistration repeatedRegistration = {};
	if( !Destiny_RegisterBlueClasses( &repeatedRegistration ) || !repeatedRegistration.alreadyRegistered ||
		repeatedRegistration.newlyRegisteredClassCount != 0 )
	{
		return Fail( "registration is not idempotent" );
	}

	DestinyEmbeddedBallConfig config = {};
	config.ballId = 1;
	config.solarSystemId = 30005286;
	config.mass = 1.0;
	config.radius = 1.0f;
	config.maximumVelocity = 250.0f;
	config.maximumAngularVelocity = 1.0f;
	config.rotation[0] = 0.125f;
	config.rotation[1] = -0.25f;
	config.rotation[2] = 0.375f;
	config.rotation[3] = 0.883883476f;
	config.agility = 1.0f;
	config.rotationalAgility = 1.0f;
	config.speedFraction = 1.0f;
	config.isFree = true;

	char error[512] = {};
	DestinyEmbeddedSession* session = Destiny_CreateEmbeddedSession( &config, error, sizeof( error ) );
	if( !session )
		return Fail( error[0] ? error : "failed to create session" );
	if( !Destiny_GetEmbeddedBallpark( session ) || !Destiny_GetEmbeddedPosition( session ) ||
		!Destiny_GetEmbeddedRotation( session ) )
	{
		Destiny_DestroyEmbeddedSession( session );
		return Fail( "session did not expose all three borrowed interfaces" );
	}

	for( uint64_t frame = 0; frame < 180; ++frame )
	{
		const Be::Time simulationTime = static_cast<Be::Time>( frame * 10000000ULL / 60ULL );
		if( !Destiny_AdvanceEmbeddedSession( session, simulationTime ) )
		{
			Destiny_DestroyEmbeddedSession( session );
			return Fail( "deterministic advance failed" );
		}
	}

	DestinyEmbeddedDiagnostics diagnostics = {};
	if( !Destiny_GetEmbeddedDiagnostics( session, &diagnostics ) )
	{
		Destiny_DestroyEmbeddedSession( session );
		return Fail( "diagnostic query failed" );
	}
	if( diagnostics.directEvolveCount != 2 || diagnostics.mode != DSTBALL_STOP ||
		diagnostics.schedulerRegistered || diagnostics.startCallCount || diagnostics.onTickCallCount ||
		diagnostics.pythonCallbackCount )
	{
		Destiny_DestroyEmbeddedSession( session );
		return Fail( "STOP session used a forbidden scheduler or callback path" );
	}
	std::fprintf(
		stderr,
		"Destiny embedded state: position=(%.9f,%.9f,%.9f) velocity=(%.9f,%.9f,%.9f) rotation=(%.9f,%.9f,%.9f,%.9f)\n",
		diagnostics.position[0],
		diagnostics.position[1],
		diagnostics.position[2],
		diagnostics.velocity[0],
		diagnostics.velocity[1],
		diagnostics.velocity[2],
		diagnostics.rotation[0],
		diagnostics.rotation[1],
		diagnostics.rotation[2],
		diagnostics.rotation[3] );
	if( !Near( diagnostics.position[0], 0.0 ) || !Near( diagnostics.position[1], 0.0 ) ||
		!Near( diagnostics.position[2], 0.0 ) || !Near( diagnostics.velocity[0], 0.0 ) ||
		!Near( diagnostics.velocity[1], 0.0 ) || !Near( diagnostics.velocity[2], 0.0 ) )
	{
		Destiny_DestroyEmbeddedSession( session );
		return Fail( "STOP ball moved" );
	}
	for( size_t i = 0; i < 4; ++i )
	{
		if( !Near( diagnostics.rotation[i], config.rotation[i] ) )
		{
			Destiny_DestroyEmbeddedSession( session );
			return Fail( "STOP ball rotation changed" );
		}
	}

	std::printf(
		"Destiny embedded contract: classes=%u evolves=%llu scheduler=off callbacks=0 position=(0,0,0)\n",
		registration.discoveredClassCount,
		static_cast<unsigned long long>( diagnostics.directEvolveCount ) );
	Destiny_DestroyEmbeddedSession( session );
	return 0;
}
