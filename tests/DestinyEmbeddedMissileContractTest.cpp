// Copyright (c) 2026 CCP Games

#include "StdAfx.h"

#include "DestinyEmbedded.h"
#include "DstConstants.h"

#include <Blue.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

const char* g_moduleName = "DestinyEmbeddedMissileContractTest";

BLUE_DEFINE_INTERFACE( ITriVectorFunction );
BLUE_DEFINE_INTERFACE( ITriQuaternionFunction );
BLUE_DEFINE_INTERFACE( IEveReferencePoint );
BLUE_DEFINE_INTERFACE( IEveBallpark );

namespace
{
constexpr Be::Time kTick = 10000000;
constexpr Be::Time kLifetime = 50000000;
constexpr int64_t kSolarSystemId = 30005286;
constexpr int64_t kAsteroId = 1;
constexpr int64_t kVentureId = 2;
constexpr int64_t kMissileId = 1001;
constexpr double kCoincidentX = 12500.0;
constexpr double kCoincidentY = -8000.0;
constexpr double kCoincidentZ = 42000.0;

int Fail( const std::string& message )
{
	std::fprintf( stderr, "DestinyEmbeddedMissileContractTest: %s\n", message.c_str() );
	return 1;
}

DestinyEmbeddedBallConfig MakeShip( int64_t id, double z, float radius )
{
	DestinyEmbeddedBallConfig config = {};
	config.ballId = id;
	config.solarSystemId = kSolarSystemId;
	config.mass = id == kAsteroId ? 975000.0 : 1200000.0;
	config.radius = radius;
	config.maximumVelocity = id == kAsteroId ? 312.0f : 335.0f;
	config.maximumAngularVelocity = 1.0f;
	config.position[2] = z;
	config.rotation[3] = 1.0f;
	config.agility = 2.87f;
	config.rotationalAgility = 1.0f;
	config.speedFraction = 1.0f;
	config.isFree = true;
	config.isMassive = true;
	config.isInteractive = true;
	return config;
}

DestinyEmbeddedMissileConfig MakeMissile()
{
	DestinyEmbeddedMissileConfig config = {};
	config.ball.ballId = kMissileId;
	config.ball.solarSystemId = kSolarSystemId;
	config.ball.mass = 700.0;
	config.ball.radius = 300.0f;
	config.ball.maximumVelocity = 3750.0f;
	config.ball.maximumAngularVelocity = 1.0f;
	config.ball.rotation[3] = 1.0f;
	config.ball.agility = 0.00014449800378457667f;
	config.ball.rotationalAgility = 1.0f;
	config.ball.speedFraction = 1.0f;
	config.ball.isFree = true;
	config.ball.isMassive = true;
	config.ball.isInteractive = true;
	config.lifetime = kLifetime;
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
	size_t size = 0;
	if( !Destiny_MeasureEmbeddedFullState( session, &size ) || size == 0 )
		return false;
	packet.assign( size, 0 );
	size_t written = 0;
	return Destiny_WriteEmbeddedFullState( session, packet.data(), packet.size(), &written ) && written == size;
}

bool CheckRejectedMissileConfigs( DestinyEmbeddedSession* session )
{
	std::vector<uint8_t> before;
	if( !WritePacket( session, before ) )
		return false;
	DestinyEmbeddedMissileConfig negative = MakeMissile();
	negative.ball.ballId = -kMissileId;
	DestinyEmbeddedMissileConfig global = MakeMissile();
	global.ball.ballId = kMissileId + 10;
	global.ball.isGlobal = true;
	if( Destiny_CommandEmbeddedLaunchMissile(
			session, kTick, &negative, kAsteroId, kVentureId, true, true ) ||
		Destiny_CommandEmbeddedLaunchMissile(
			session, kTick, &global, kAsteroId, kVentureId, true, true ) ||
		Destiny_GetEmbeddedBallPosition( session, negative.ball.ballId ) ||
		Destiny_GetEmbeddedBallPosition( session, global.ball.ballId ) )
	{
		return false;
	}
	std::vector<uint8_t> after;
	return WritePacket( session, after ) && after == before;
}

struct RunResult
{
	std::vector<DestinyEmbeddedMissileState> states;
	DestinyEmbeddedBallState target = {};
	Be::Time collisionTime = 0;
};

bool RunMissile(
	DestinyEmbeddedSession* session,
	RunResult& result )
{
	const DestinyEmbeddedMissileConfig missile = MakeMissile();
	if( !Destiny_CommandEmbeddedLaunchMissile(
			session, kTick, &missile, kAsteroId, kVentureId, true, true ) )
	{
		return false;
	}
	if( Destiny_GetEmbeddedMissileState( session, kMissileId, nullptr ) )
		return false;

	for( int tick = 1; tick <= 6; ++tick )
	{
		if( !Destiny_AdvanceEmbeddedSession( session, static_cast<Be::Time>( tick ) * kTick ) )
			return false;
		DestinyEmbeddedMissileState state = {};
		if( !Destiny_GetEmbeddedMissileState( session, kMissileId, &state ) ||
			state.ball.ballId != kMissileId || state.ownerBallId != kAsteroId ||
			state.targetBallId != kVentureId || state.launchTime != kTick ||
			state.lifetime != kLifetime )
		{
			return false;
		}
		result.states.push_back( state );
		if( tick == 1 && ( state.ball.mode != DSTBALL_MISSILE || !state.initialStraightFlight ||
			Destiny_RemoveEmbeddedMissile( session, kMissileId ) ) )
		{
			return false;
		}
		if( tick >= 2 && state.initialStraightFlight )
			return false;
		if( state.collided )
		{
			result.collisionTime = state.firstCollisionTime;
			break;
		}
	}
	if( result.collisionTime == 0 || result.collisionTime > kTick + kLifetime )
		return false;
	if( !Destiny_GetEmbeddedBallState( session, kVentureId, &result.target ) ||
		result.target.mode != DSTBALL_STOP || std::abs( result.target.position[2] - 5000.0 ) > 1.0e-6 ||
		!Destiny_RemoveEmbeddedMissile( session, kMissileId ) ||
		Destiny_GetEmbeddedBallState( session, kMissileId, &result.target ) )
	{
		return false;
	}
	DestinyEmbeddedEventDiagnostics eventDiagnostics = {};
	const bool eventDiagnosticsOk = Destiny_GetEmbeddedEventDiagnostics(
		session, &eventDiagnostics, sizeof( eventDiagnostics ) );
	if( !eventDiagnosticsOk || eventDiagnostics.suppressedSendEventAttemptCount != 4 ||
		eventDiagnostics.suppressedPostEventAttemptCount != 0 ||
		eventDiagnostics.deliveredWarpEventCallbackCount != 0 )
	{
		return false;
	}
	DestinyEmbeddedMissileState removed = {};
	return Destiny_GetEmbeddedMissileState( session, kMissileId, &removed ) && removed.removed &&
		!removed.active && removed.collided && !Destiny_RemoveEmbeddedMissile( session, kMissileId );
}

bool RunExpiry( DestinyEmbeddedSession* session )
{
	DestinyEmbeddedMissileConfig missile = MakeMissile();
	missile.ball.ballId = kMissileId + 1;
	missile.ball.maximumVelocity = 100.0f;
	if( !Destiny_CommandEmbeddedLaunchMissile(
			session, kTick, &missile, kAsteroId, kVentureId, true, true ) )
	{
		return false;
	}
	DestinyEmbeddedMissileState state = {};
	for( int tick = 1; tick <= 6; ++tick )
	{
		if( !Destiny_AdvanceEmbeddedSession( session, static_cast<Be::Time>( tick ) * kTick ) ||
			!Destiny_GetEmbeddedMissileState( session, missile.ball.ballId, &state ) )
		{
			return false;
		}
	}
	if( state.collided || !state.expired || state.removed ||
		!Destiny_RemoveEmbeddedMissile( session, missile.ball.ballId ) )
	{
		return false;
	}
	return Destiny_GetEmbeddedMissileState( session, missile.ball.ballId, &state ) &&
		state.expired && state.removed && !state.active;
}

struct CoincidentResult
{
	DestinyEmbeddedMissileState state = {};
	std::vector<uint8_t> activePacket;
};

bool RunCoincidentMissile( DestinyEmbeddedSession* session, CoincidentResult& result )
{
	const DestinyEmbeddedMissileConfig missile = MakeMissile();
	if( !Destiny_CommandEmbeddedLaunchMissile(
		session, kTick, &missile, kAsteroId, kVentureId, true, true ) )
	{
		return false;
	}
	for( int tick = 1; tick <= 6; ++tick )
	{
		if( !Destiny_AdvanceEmbeddedSession( session, static_cast<Be::Time>( tick ) * kTick ) ||
			!Destiny_GetEmbeddedMissileState( session, kMissileId, &result.state ) )
		{
			return false;
		}
		if( result.state.collided )
			break;
	}
	if( !result.state.active || !result.state.collided || result.state.expired ||
		result.state.ball.mode != DSTBALL_MISSILE || result.state.ownerBallId != kAsteroId ||
		result.state.targetBallId != kVentureId || result.state.firstCollisionBallId != kVentureId ||
		std::abs( result.state.ball.position[0] - kCoincidentX ) > 1.0e-6 ||
		std::abs( result.state.ball.position[1] - kCoincidentY ) > 1.0e-6 ||
		std::abs( result.state.ball.position[2] - kCoincidentZ ) > 1.0e-6 ||
		!WritePacket( session, result.activePacket ) ||
		!Destiny_RemoveEmbeddedMissile( session, kMissileId ) )
	{
		return false;
	}
	DestinyEmbeddedMissileState removed = {};
	return Destiny_GetEmbeddedMissileState( session, kMissileId, &removed ) &&
		removed.removed && removed.collided && !removed.active;
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

	const DestinyEmbeddedBallConfig astero = MakeShip( kAsteroId, 0.0, 35.0f );
	const DestinyEmbeddedBallConfig venture = MakeShip( kVentureId, 5000.0, 38.0f );
	const DestinyEmbeddedSessionOptions options = MakeOptions();
	char error[256] = {};
	DestinyEmbeddedSession* record = Destiny_CreateEmbeddedSessionWithOptions(
		&astero, &options, error, sizeof( error ) );
	if( !record || !Destiny_AddEmbeddedDynamicBall( record, &venture, error, sizeof( error ) ) )
		return Fail( std::string( "record fixture failed: " ) + error );

	std::vector<uint8_t> initialSnapshot;
	if( !WritePacket( record, initialSnapshot ) )
		return Fail( "initial two-ball snapshot failed" );
	if( !CheckRejectedMissileConfigs( record ) )
		return Fail( "non-ordinary missile config was accepted, allocated, or changed serialized state" );
	RunResult first;
	if( !RunMissile( record, first ) )
		return Fail( "native launch, straight-flight, homing, collision, or removal contract failed" );
	Destiny_DestroyEmbeddedSession( record );

	DestinyEmbeddedFullStateDescriptor descriptor = {};
	descriptor.wireProfile = DESTINY_EMBEDDED_DYNAMIC_ORIENTATION_V1;
	descriptor.solarSystemId = kSolarSystemId;
	descriptor.primaryBallId = kAsteroId;
	descriptor.egoBallId = kAsteroId;
	DestinyEmbeddedSession* replay = Destiny_CreateEmbeddedSessionFromFullState(
		initialSnapshot.data(), initialSnapshot.size(), &descriptor, &options, error, sizeof( error ) );
	if( !replay )
		return Fail( std::string( "replay fixture failed: " ) + error );
	std::vector<uint8_t> replaySnapshot;
	if( !WritePacket( replay, replaySnapshot ) || replaySnapshot != initialSnapshot )
		return Fail( "initial two-ball snapshot changed before replay" );
	RunResult second;
	if( !RunMissile( replay, second ) || first.collisionTime != second.collisionTime ||
		first.states.size() != second.states.size() ||
		std::memcmp( first.states.data(), second.states.data(),
			first.states.size() * sizeof( DestinyEmbeddedMissileState ) ) != 0 )
	{
		return Fail( "replayed missile trajectory diverged" );
	}
	Destiny_DestroyEmbeddedSession( replay );

	DestinyEmbeddedSession* expiry = Destiny_CreateEmbeddedSessionFromFullState(
		initialSnapshot.data(), initialSnapshot.size(), &descriptor, &options, error, sizeof( error ) );
	if( !expiry || !RunExpiry( expiry ) )
		return Fail( "missile expiry and guarded removal contract failed" );
	Destiny_DestroyEmbeddedSession( expiry );

	DestinyEmbeddedBallConfig coincidentAstero = astero;
	coincidentAstero.position[0] = kCoincidentX;
	coincidentAstero.position[1] = kCoincidentY;
	coincidentAstero.position[2] = kCoincidentZ;
	DestinyEmbeddedBallConfig coincidentVenture = venture;
	coincidentVenture.position[0] = kCoincidentX;
	coincidentVenture.position[1] = kCoincidentY;
	coincidentVenture.position[2] = kCoincidentZ;
	DestinyEmbeddedSession* coincidentRecord = Destiny_CreateEmbeddedSessionWithOptions(
		&coincidentAstero, &options, error, sizeof( error ) );
	if( !coincidentRecord ||
		!Destiny_AddEmbeddedDynamicBall( coincidentRecord, &coincidentVenture, error, sizeof( error ) ) )
	{
		return Fail( std::string( "coincident record fixture failed: " ) + error );
	}
	std::vector<uint8_t> coincidentSnapshot;
	if( !WritePacket( coincidentRecord, coincidentSnapshot ) )
		return Fail( "coincident initial snapshot failed" );
	CoincidentResult coincidentFirst;
	if( !RunCoincidentMissile( coincidentRecord, coincidentFirst ) )
		return Fail( "coincident aimed launch failed to enter normal missile collision behavior" );
	Destiny_DestroyEmbeddedSession( coincidentRecord );

	DestinyEmbeddedSession* coincidentReplay = Destiny_CreateEmbeddedSessionFromFullState(
		coincidentSnapshot.data(), coincidentSnapshot.size(), &descriptor, &options, error, sizeof( error ) );
	std::vector<uint8_t> coincidentFixedPoint;
	if( !coincidentReplay || !WritePacket( coincidentReplay, coincidentFixedPoint ) ||
		coincidentFixedPoint != coincidentSnapshot )
	{
		return Fail( "coincident initial snapshot fixed point failed" );
	}
	CoincidentResult coincidentSecond;
	if( !RunCoincidentMissile( coincidentReplay, coincidentSecond ) )
		return Fail( "coincident replay launch, advance, collision, or removal failed" );
	if( std::memcmp( &coincidentFirst.state, &coincidentSecond.state, sizeof( coincidentFirst.state ) ) != 0 )
		return Fail( "coincident missile state replay identity failed" );
	Destiny_DestroyEmbeddedSession( coincidentReplay );

	std::printf(
		"Destiny PL-C1 missile contract: mode=MISSILE owner=1 target=2 straight-ms=800 "
		"collision-tick=%lld expiry-ms=5000 expiry-removal=true coincident=accepted replay=identical\n",
		static_cast<long long>( first.collisionTime / kTick ) );
	return 0;
}
