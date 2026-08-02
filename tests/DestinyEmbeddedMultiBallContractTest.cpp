// Copyright (c) 2026 CCP Games

#include "StdAfx.h"

#include "DestinyEmbedded.h"
#include "DstConstants.h"

#include <Blue.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

const char* g_moduleName = "DestinyEmbeddedMultiBallContractTest";

BLUE_DEFINE_INTERFACE( ITriVectorFunction );
BLUE_DEFINE_INTERFACE( ITriQuaternionFunction );
BLUE_DEFINE_INTERFACE( IEveReferencePoint );
BLUE_DEFINE_INTERFACE( IEveBallpark );

namespace
{
constexpr Be::Time kTick = 10000000;
constexpr int64_t kSolarSystemId = 30005286;
constexpr int64_t kAsteroId = 1;
constexpr int64_t kVentureId = 2;

int Fail( const std::string& message )
{
	std::fprintf( stderr, "DestinyEmbeddedMultiBallContractTest: %s\n", message.c_str() );
	return 1;
}

DestinyEmbeddedBallConfig MakeBall( int64_t ballId, double forwardPosition, float radius )
{
	DestinyEmbeddedBallConfig config = {};
	config.ballId = ballId;
	config.solarSystemId = kSolarSystemId;
	config.mass = ballId == kAsteroId ? 975000.0 : 1200000.0;
	config.radius = radius;
	config.maximumVelocity = ballId == kAsteroId ? 312.0f : 335.0f;
	config.maximumAngularVelocity = 1.0f;
	config.position[2] = forwardPosition;
	config.rotation[3] = 1.0f;
	config.agility = ballId == kAsteroId ? 2.87f : 3.0f;
	config.rotationalAgility = 1.0f;
	config.speedFraction = 1.0f;
	config.isFree = true;
	config.isMassive = true;
	config.isInteractive = true;
	return config;
}

DestinyEmbeddedSessionOptions MakeOptions()
{
	DestinyEmbeddedSessionOptions options = {};
	options.orientationPolicy = DESTINY_EMBEDDED_NATIVE_ORIENTATION;
	options.referenceFrame = DESTINY_EMBEDDED_PRIMARY_EGO;
	options.orbitPolicy = DESTINY_EMBEDDED_ORBIT_FRONTIER_NEW;
	return options;
}

bool WritePacket( DestinyEmbeddedSession* session, std::vector<uint8_t>& packet )
{
	size_t measured = 0;
	if( !Destiny_MeasureEmbeddedFullState( session, &measured ) || measured == 0 )
		return false;
	packet.assign( measured, 0 );
	size_t written = 0;
	return Destiny_WriteEmbeddedFullState( session, packet.data(), packet.size(), &written ) &&
		written == measured;
}

bool SameState( const DestinyEmbeddedBallState& first, const DestinyEmbeddedBallState& second )
{
	return std::memcmp( &first, &second, sizeof( first ) ) == 0;
}

bool CheckStationaryTargetCurve( DestinyEmbeddedSession* session )
{
	ITriVectorFunction* curve = Destiny_GetEmbeddedBallPosition( session, kVentureId );
	if( !curve )
		return false;
	for( Be::Time time : { Be::Time( 0 ), Be::Time( 166667 ), Be::Time( 119 * 166667 ) } )
	{
		Vector3 value;
		curve->GetValueAt( &value, time );
		if( std::abs( value.x ) > 1.0e-6f || std::abs( value.y ) > 1.0e-6f ||
			std::abs( value.z - 5000.0f ) > 1.0e-4f )
		{
			return false;
		}
	}
	return true;
}
}

int main()
{
	if( !Py_IsInitialized() )
		Py_Initialize();
	BlueModuleStartup();
	DestinyEmbeddedRegistration registration = {};
	if( !Destiny_RegisterBlueClasses( &registration ) )
		return Fail( "registration failed" );

	const DestinyEmbeddedBallConfig astero = MakeBall( kAsteroId, 0.0, 35.0f );
	const DestinyEmbeddedBallConfig venture = MakeBall( kVentureId, 5000.0, 38.0f );
	const DestinyEmbeddedSessionOptions options = MakeOptions();
	char error[256] = {};
	DestinyEmbeddedSession* direct = Destiny_CreateEmbeddedSessionWithOptions(
		&astero, &options, error, sizeof( error ) );
	if( !direct )
		return Fail( std::string( "direct session failed: " ) + error );
	std::vector<uint8_t> beforeRejectedBalls;
	if( !WritePacket( direct, beforeRejectedBalls ) )
		return Fail( "pre-rejection full-state write failed" );
	DestinyEmbeddedBallConfig negative = venture;
	negative.ballId = -2;
	DestinyEmbeddedBallConfig global = venture;
	global.ballId = 3;
	global.isGlobal = true;
	if( Destiny_AddEmbeddedDynamicBall( direct, &negative, error, sizeof( error ) ) ||
		Destiny_AddEmbeddedDynamicBall( direct, &global, error, sizeof( error ) ) ||
		Destiny_GetEmbeddedBallPosition( direct, negative.ballId ) ||
		Destiny_GetEmbeddedBallPosition( direct, global.ballId ) )
	{
		return Fail( "non-ordinary dynamic ball was accepted or allocated" );
	}
	std::vector<uint8_t> afterRejectedBalls;
	if( !WritePacket( direct, afterRejectedBalls ) || afterRejectedBalls != beforeRejectedBalls )
		return Fail( "rejected dynamic ball changed serialized state" );
	if( !Destiny_AddEmbeddedDynamicBall( direct, &venture, error, sizeof( error ) ) )
		return Fail( std::string( "Venture addition failed: " ) + error );
	if( Destiny_AddEmbeddedDynamicBall( direct, &venture, error, sizeof( error ) ) )
		return Fail( "duplicate dynamic ball accepted" );
	DestinyEmbeddedBallConfig wrongSystem = venture;
	wrongSystem.ballId = 3;
	wrongSystem.solarSystemId += 1;
	if( Destiny_AddEmbeddedDynamicBall( direct, &wrongSystem, error, sizeof( error ) ) )
		return Fail( "cross-system dynamic ball accepted" );

	DestinyEmbeddedBallState directStates[3][2] = {};
	if( !Destiny_GetEmbeddedBallPosition( direct, kAsteroId ) ||
		!Destiny_GetEmbeddedBallRotation( direct, kAsteroId ) ||
		!Destiny_GetEmbeddedBallPosition( direct, kVentureId ) ||
		!Destiny_GetEmbeddedBallRotation( direct, kVentureId ) ||
		Destiny_GetEmbeddedBallPosition( direct, 99 ) ||
		!Destiny_GetEmbeddedBallState( direct, kAsteroId, &directStates[0][0] ) ||
		!Destiny_GetEmbeddedBallState( direct, kVentureId, &directStates[0][1] ) ||
		directStates[0][0].mode != DSTBALL_STOP || directStates[0][1].mode != DSTBALL_STOP ||
		!CheckStationaryTargetCurve( direct ) )
	{
		return Fail( "initial two-ball state or curve contract failed" );
	}

	std::vector<uint8_t> packet;
	if( !WritePacket( direct, packet ) )
		return Fail( "initial full-state write failed" );
	for( size_t evolve = 1; evolve <= 2; ++evolve )
	{
		if( !Destiny_AdvanceEmbeddedSession( direct, static_cast<Be::Time>( evolve ) * kTick ) ||
			!Destiny_GetEmbeddedBallState( direct, kAsteroId, &directStates[evolve][0] ) ||
			!Destiny_GetEmbeddedBallState( direct, kVentureId, &directStates[evolve][1] ) )
		{
			return Fail( "direct evolve failed" );
		}
	}
	DestinyEmbeddedBallConfig late = venture;
	late.ballId = 3;
	if( Destiny_AddEmbeddedDynamicBall( direct, &late, error, sizeof( error ) ) )
		return Fail( "late dynamic ball accepted" );
	Destiny_DestroyEmbeddedSession( direct );

	DestinyEmbeddedFullStateDescriptor descriptor = {};
	descriptor.wireProfile = DESTINY_EMBEDDED_DYNAMIC_ORIENTATION_V1;
	descriptor.solarSystemId = kSolarSystemId;
	descriptor.primaryBallId = kAsteroId;
	descriptor.egoBallId = kAsteroId;
	DestinyEmbeddedSession* restored = Destiny_CreateEmbeddedSessionFromFullState(
		packet.data(), packet.size(), &descriptor, &options, error, sizeof( error ) );
	if( !restored )
		return Fail( std::string( "full-state restore failed: " ) + error );
	if( !CheckStationaryTargetCurve( restored ) )
		return Fail( "restored target curve moves during the initial interpolation interval" );
	std::vector<uint8_t> fixedPoint;
	if( !WritePacket( restored, fixedPoint ) || fixedPoint != packet )
		return Fail( "two-ball write-read-write fixed point failed" );
	for( size_t evolve = 0; evolve <= 2; ++evolve )
	{
		DestinyEmbeddedBallState restoredStates[2] = {};
		if( evolve != 0 &&
			!Destiny_AdvanceEmbeddedSession( restored, static_cast<Be::Time>( evolve ) * kTick ) )
		{
			return Fail( "restored evolve failed" );
		}
		if( !Destiny_GetEmbeddedBallState( restored, kAsteroId, &restoredStates[0] ) ||
			!Destiny_GetEmbeddedBallState( restored, kVentureId, &restoredStates[1] ) ||
			!SameState( directStates[evolve][0], restoredStates[0] ) ||
			!SameState( directStates[evolve][1], restoredStates[1] ) )
		{
			return Fail( "restored two-ball state diverged" );
		}
	}
	Destiny_DestroyEmbeddedSession( restored );

	std::printf(
		"Destiny PL-C0 two-ball contract: stop-balls=2 curves=distinct "
		"late-rejection=true duplicate-rejection=true ordinary-rejection=true fixed-point=true evolves=2\n" );
	return 0;
}
