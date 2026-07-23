// Copyright (c) 2026 CCP Games
//
// D-03 seam contract: the C++ semantics preserved from the retired Python
// test suites (test_ball, test_cleanup, test_configure). Compiles with
// DESTINY_WITH_PYTHON=0 and DESTINY_EMBEDDED=1 so this translation unit sees
// exactly the ABI of the destinyEmbedded archive it links.

#include "StdAfx.h"

#include "DestinyEmbedded.h"
#include "DstConstants.h"
#include "MiniBox.h"
#include "MiniCapsule.h"
#include "Settings.h"

#include <Blue.h>

#include <cmath>
#include <cstdio>
#include <string>

const char* g_moduleName = "DestinyEmbeddedSeamTest";

BLUE_DEFINE_INTERFACE( ITriVectorFunction );
BLUE_DEFINE_INTERFACE( ITriQuaternionFunction );
BLUE_DEFINE_INTERFACE( IEveReferencePoint );
BLUE_DEFINE_INTERFACE( IEveBallpark );

namespace
{

int Fail( const std::string& message )
{
	std::fprintf( stderr, "DestinyEmbeddedSeamTest: %s\n", message.c_str() );
	return 1;
}

bool Near( double actual, double expected, double tolerance )
{
	return std::fabs( actual - expected ) <= tolerance;
}

DestinyEmbeddedBallConfig MakeConfig()
{
	DestinyEmbeddedBallConfig config = {};
	config.ballId = 1;
	config.solarSystemId = 30005286;
	config.mass = 975000.0;
	config.radius = 35.0f;
	config.maximumVelocity = 312.0f;
	config.maximumAngularVelocity = 1.0f;
	// Identity rotation on purpose: the rotated-vector check below pins the
	// no-rotation identity the retired test_ball asserted.
	config.rotation[3] = 1.0f;
	config.agility = 2.87f;
	config.rotationalAgility = 1.0f;
	config.speedFraction = 1.0f;
	config.isFree = true;
	config.isMassive = true;
	config.isInteractive = true;
	return config;
}

}

int main()
{
	if( !Py_IsInitialized() )
		Py_Initialize();
	BlueModuleStartup();

	DestinyEmbeddedRegistration registration = {};
	if( !Destiny_RegisterBlueClasses( &registration ) )
		return Fail( "embedded registration failed" );

	// --- settings schema (test_configure port): defaults hold before any
	// session and the tunables round-trip as plain globals.
	if( g_useDynamicalOrientation != g_useDynamicalOrientationDefault ||
		g_useNewOrbit != g_useNewOrbitDefault ||
		g_collisionMaxIterations != g_collisionMaxIterationsDefault )
		return Fail( "settings globals do not hold their defaults at startup" );

	g_collisionMaxIterations = 123;
	if( g_collisionMaxIterations != 123 )
		return Fail( "collision iteration knob did not round-trip" );
	g_collisionMaxIterations = g_collisionMaxIterationsDefault;

	DestinyEmbeddedSessionOptions options = {};
	options.orientationPolicy = DESTINY_EMBEDDED_NATIVE_ORIENTATION;
	options.referenceFrame = DESTINY_EMBEDDED_PRIMARY_EGO;
	options.orbitPolicy = DESTINY_EMBEDDED_ORBIT_FRONTIER_NEW;

	DestinyEmbeddedBallConfig config = MakeConfig();
	char error[256] = {};
	DestinyEmbeddedSession* session =
		Destiny_CreateEmbeddedSessionWithOptions( &config, &options, error, sizeof( error ) );
	if( !session )
		return Fail( std::string( "session creation failed: " ) + error );

	if( !g_useDynamicalOrientation || !g_useNewOrbit )
		return Fail( "session options did not project onto the settings globals" );

	IEveBallpark* eveBallpark = Destiny_GetEmbeddedBallpark( session );
	if( !eveBallpark )
		return Fail( "embedded ballpark unavailable" );
	// The embedded position function is implemented by the primary ClientBall.
	ITriVectorFunction* primaryPosition = Destiny_GetEmbeddedPosition( session );
	if( !primaryPosition )
		return Fail( "embedded position function unavailable" );
	Ball* ball = static_cast<ClientBall*>( primaryPosition );

	// --- live-count bookkeeping (test_cleanup port)
	const Be::ClassInfo* parkInfo = eveBallpark->ClassType();
	const Be::ClassInfo* ballInfo = ( (IBall*)ball )->ClassType();
	const uint32_t parkLive = parkInfo->mLiveCount;
	const uint32_t ballLive = ballInfo->mLiveCount;
	if( parkLive == 0 || ballLive == 0 )
		return Fail( "live counts missing for park or ball" );

	// --- compound collision shapes (test_ball port)
	if( ball->mMiniBalls.size() != 0 || ball->mMiniBoxes.size() != 0 || ball->mMiniCapsules.size() != 0 )
		return Fail( "fresh ball already carries mini shapes" );

	ball->AddMiniBall( 1.0, 2.0, 3.0, 1.0f );
	ball->AddMiniCapsule( 1.0, 0.0, 0.0, 2.0, 0.0, 0.0, 1.0f );
	ball->AddMiniBox(
		Vector3d( -5.0, 0.0, 0.0 ),
		Vector3d( 10.0, 0.0, 0.0 ),
		Vector3d( 0.0, 10.0, 0.0 ),
		Vector3d( 0.0, 0.0, 10.0 ) );
	if( ball->mMiniBalls.size() != 1 || ball->mMiniCapsules.size() != 1 || ball->mMiniBoxes.size() != 1 )
		return Fail( "mini shape lists did not record the additions" );

	const Be::ClassInfo* miniBallInfo = ball->mMiniBalls[ 0 ]->ClassType();
	const Be::ClassInfo* miniBoxInfo = ball->mMiniBoxes[ 0 ]->ClassType();
	const Be::ClassInfo* miniCapsuleInfo = ball->mMiniCapsules[ 0 ]->ClassType();
	if( miniBallInfo->mLiveCount == 0 || miniBoxInfo->mLiveCount == 0 || miniCapsuleInfo->mLiveCount == 0 )
		return Fail( "mini shape live counts missing" );

	// --- rotated vector identity (test_ball port)
	Vector3 vec( 1.0f, 2.0f, 3.0f );
	ball->GetRotatedVector( vec );
	if( !Near( vec.x, 1.0, 1e-6 ) || !Near( vec.y, 2.0, 1e-6 ) || !Near( vec.z, 3.0, 1e-6 ) )
		return Fail( "identity rotation perturbed the rotated vector" );

	// --- formation slots (test_ball port): no formation assigned yields -1
	if( ball->ReserveFormationSlot() != -1 )
		return Fail( "formation slot reserved without a formation" );

	Destiny_DestroyEmbeddedSession( session );

	if( g_useDynamicalOrientation != g_useDynamicalOrientationDefault ||
		g_useNewOrbit != g_useNewOrbitDefault )
		return Fail( "session destruction did not restore the settings globals" );

	if( parkInfo->mLiveCount != parkLive - 1 )
		return Fail( "ballpark leaked after session destruction" );
	if( ballInfo->mLiveCount != ballLive - 1 )
		return Fail( "primary ball leaked after session destruction" );
	if( miniBallInfo->mLiveCount != 0 || miniBoxInfo->mLiveCount != 0 || miniCapsuleInfo->mLiveCount != 0 )
		return Fail( "mini shapes leaked after session destruction" );

	// --- second session: checkout-default orbit policy keeps the legacy solver
	DestinyEmbeddedSessionOptions checkoutOptions = {};
	checkoutOptions.orientationPolicy = DESTINY_EMBEDDED_PIN_INITIAL;
	checkoutOptions.referenceFrame = DESTINY_EMBEDDED_PRIMARY_EGO;
	checkoutOptions.orbitPolicy = DESTINY_EMBEDDED_ORBIT_CHECKOUT_DEFAULT;
	DestinyEmbeddedBallConfig secondConfig = MakeConfig();
	DestinyEmbeddedSession* second =
		Destiny_CreateEmbeddedSessionWithOptions( &secondConfig, &checkoutOptions, error, sizeof( error ) );
	if( !second )
		return Fail( std::string( "second session creation failed: " ) + error );
	if( g_useNewOrbit )
		return Fail( "checkout-default orbit policy enabled the frontier solver" );
	Destiny_DestroyEmbeddedSession( second );

	std::printf(
		"Destiny D-03 seam contract: mini-shapes=3 live-counts=clean settings=restored formation-slot=-1\n" );
	return 0;
}
