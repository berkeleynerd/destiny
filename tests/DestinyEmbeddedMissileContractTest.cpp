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

	std::printf(
		"Destiny PL-C1 missile contract: mode=MISSILE owner=1 target=2 straight-ms=800 "
		"collision-tick=%lld expiry-ms=5000 expiry-removal=true replay=identical\n",
		static_cast<long long>( first.collisionTime / kTick ) );
	return 0;
}
