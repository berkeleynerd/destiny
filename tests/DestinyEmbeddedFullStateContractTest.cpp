// Copyright (c) 2026 CCP Games
//
// PL-14G1 construction contract: a validated timestamp-zero full state births
// a fresh embedded session without consuming the single-session slot on any
// preflight failure.

#include "StdAfx.h"

#include "DestinyEmbedded.h"
#include "DstConstants.h"

#include <Blue.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

const char* g_moduleName = "DestinyEmbeddedFullStateContractTest";

BLUE_DEFINE_INTERFACE( ITriVectorFunction );
BLUE_DEFINE_INTERFACE( ITriQuaternionFunction );
BLUE_DEFINE_INTERFACE( IEveReferencePoint );
BLUE_DEFINE_INTERFACE( IEveBallpark );

namespace
{
constexpr Be::Time kTick = 10000000;
constexpr int64_t kPrimary = 1;
constexpr int64_t kFixed = 2;
constexpr int64_t kSun = 3;
constexpr int64_t kPlanet = 4;

int Fail( const std::string& message )
{
	std::fprintf( stderr, "DestinyEmbeddedFullStateContractTest: %s\n", message.c_str() );
	return 1;
}

DestinyEmbeddedBallConfig MakePrimary()
{
	DestinyEmbeddedBallConfig config = {};
	config.ballId = kPrimary;
	config.solarSystemId = 30005286;
	config.mass = 975000.0;
	config.radius = 35.0f;
	config.maximumVelocity = 312.0f;
	config.maximumAngularVelocity = 1.0f;
	config.position[0] = 1200.0;
	config.position[1] = -300.0;
	config.position[2] = -1000.0;
	config.velocity[2] = 2.0;
	config.rotation[2] = 0.3826834323650898f;
	config.rotation[3] = 0.9238795325112867f;
	config.agility = 2.87f;
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

DestinyEmbeddedFullStateDescriptor MakeDescriptor( bool withRoles )
{
	DestinyEmbeddedFullStateDescriptor descriptor = {};
	descriptor.wireProfile = DESTINY_EMBEDDED_DYNAMIC_ORIENTATION_V1;
	descriptor.solarSystemId = 30005286;
	descriptor.initialTimestamp = 0;
	descriptor.primaryBallId = kPrimary;
	descriptor.egoBallId = kPrimary;
	if( withRoles )
	{
		descriptor.fixedTargetBallId = kFixed;
		descriptor.celestialBallIds[0] = kSun;
		descriptor.celestialBallIds[1] = kPlanet;
		descriptor.celestialBallCount = 2;
	}
	return descriptor;
}

bool WritePacket( DestinyEmbeddedSession* session, std::vector<uint8_t>& packet, std::string& error )
{
	size_t measured = 0;
	if( !Destiny_MeasureEmbeddedFullState( session, &measured ) || measured < 5 )
	{
		error = "measure failed";
		return false;
	}
	packet.assign( measured, 0 );
	size_t written = 0;
	if( !Destiny_WriteEmbeddedFullState( session, packet.data(), packet.size(), &written ) ||
		written != measured || packet[0] != DESTINY_FULLSTATE )
	{
		error = "measured write diverged";
		return false;
	}
	return true;
}

template <typename T>
void Store( std::vector<uint8_t>& packet, size_t offset, const T& value )
{
	std::memcpy( packet.data() + offset, &value, sizeof( value ) );
}

bool SameCreationDiagnostics(
	const DestinyEmbeddedFullStateCreationDiagnostics& first,
	const DestinyEmbeddedFullStateCreationDiagnostics& second )
{
	return first.creationAttemptCount == second.creationAttemptCount &&
		first.preflightAttemptCount == second.preflightAttemptCount &&
		first.preflightRejectionCount == second.preflightRejectionCount &&
		first.sessionSlotAcquisitionCount == second.sessionSlotAcquisitionCount &&
		first.sessionAllocationCount == second.sessionAllocationCount &&
		first.globalSettingsMutationCount == second.globalSettingsMutationCount &&
		first.ballparkAllocationCount == second.ballparkAllocationCount &&
		first.successfulCreationCount == second.successfulCreationCount;
}

bool ExpectRejected(
	const std::vector<uint8_t>& packet,
	const DestinyEmbeddedFullStateDescriptor& descriptor,
	const DestinyEmbeddedSessionOptions& options,
	DestinyEmbeddedFullStateRejection expectedRejection )
{
	DestinyEmbeddedFullStateCreationDiagnostics before = {};
	DestinyEmbeddedFullStateCreationDiagnostics afterInspect = {};
	DestinyEmbeddedFullStateCreationDiagnostics afterCreate = {};
	if( !Destiny_GetEmbeddedFullStateCreationDiagnostics( &before ) )
		return false;
	DestinyEmbeddedFullStatePreflightDiagnostics preflight = {};
	char error[256] = {};
	if( Destiny_InspectEmbeddedFullState(
			packet.data(),
			packet.size(),
			&descriptor,
			&options,
			&preflight,
			error,
			sizeof( error ) ) ||
		preflight.rejection != expectedRejection || !preflight.sideEffectFree ||
		preflight.rolesValidated || preflight.packetSize != packet.size() || error[0] == '\0' ||
		!Destiny_GetEmbeddedFullStateCreationDiagnostics( &afterInspect ) ||
		!SameCreationDiagnostics( before, afterInspect ) )
	{
		return false;
	}
	error[0] = '\0';
	DestinyEmbeddedSession* session = Destiny_CreateEmbeddedSessionFromFullState(
		packet.data(), packet.size(), &descriptor, &options, error, sizeof( error ) );
	if( session )
	{
		Destiny_DestroyEmbeddedSession( session );
		return false;
	}
	if( error[0] == '\0' || !Destiny_GetEmbeddedFullStateCreationDiagnostics( &afterCreate ) )
		return false;
	return afterCreate.creationAttemptCount == before.creationAttemptCount + 1 &&
		afterCreate.preflightAttemptCount == before.preflightAttemptCount + 1 &&
		afterCreate.preflightRejectionCount == before.preflightRejectionCount + 1 &&
		afterCreate.sessionSlotAcquisitionCount == before.sessionSlotAcquisitionCount &&
		afterCreate.sessionAllocationCount == before.sessionAllocationCount &&
		afterCreate.globalSettingsMutationCount == before.globalSettingsMutationCount &&
		afterCreate.ballparkAllocationCount == before.ballparkAllocationCount &&
		afterCreate.successfulCreationCount == before.successfulCreationCount;
}

bool SuccessfulCreationDelta(
	const DestinyEmbeddedFullStateCreationDiagnostics& before,
	const DestinyEmbeddedFullStateCreationDiagnostics& after )
{
	return after.creationAttemptCount == before.creationAttemptCount + 1 &&
		after.preflightAttemptCount == before.preflightAttemptCount + 1 &&
		after.preflightRejectionCount == before.preflightRejectionCount &&
		after.sessionSlotAcquisitionCount == before.sessionSlotAcquisitionCount + 1 &&
		after.sessionAllocationCount == before.sessionAllocationCount + 1 &&
		after.globalSettingsMutationCount == before.globalSettingsMutationCount + 1 &&
		after.ballparkAllocationCount == before.ballparkAllocationCount + 1 &&
		after.successfulCreationCount == before.successfulCreationCount + 1;
}

bool SameLiveState( const DestinyEmbeddedDiagnostics& first, const DestinyEmbeddedDiagnostics& second )
{
	if( first.directEvolveCount != second.directEvolveCount ||
		first.lastRequestedTime != second.lastRequestedTime || first.nextTickTime != second.nextTickTime ||
		first.mode != second.mode || first.primaryBallId != second.primaryBallId ||
		first.egoBallId != second.egoBallId || first.observerBallId != second.observerBallId ||
		first.schedulerRegistered != second.schedulerRegistered ||
		first.dynamicalOrientationEnabled != second.dynamicalOrientationEnabled ||
		first.frontierOrbitEnabled != second.frontierOrbitEnabled ||
		first.commandCount != second.commandCount ||
		first.orientationPinCount != second.orientationPinCount ||
		first.lastCommandTime != second.lastCommandTime ||
		first.orbitTargetBallId != second.orbitTargetBallId ||
		first.followBallId != second.followBallId || first.followRange != second.followRange ||
		first.orbitCenterDistance != second.orbitCenterDistance ||
		first.orbitSurfaceDistance != second.orbitSurfaceDistance ||
		first.orbitRadialVelocity != second.orbitRadialVelocity ||
		first.orbitTangentialVelocity != second.orbitTangentialVelocity ||
		first.orbitAccumulatedPhase != second.orbitAccumulatedPhase ||
		first.warpEffectStamp != second.warpEffectStamp || first.warpFactor != second.warpFactor ||
		first.warpMinRange != second.warpMinRange ||
		first.warpTotalDistance != second.warpTotalDistance ||
		first.isMassive != second.isMassive || first.sensorActive != second.sensorActive )
	{
		return false;
	}
	for( size_t i = 0; i < 3; ++i )
	{
		if( first.rawPosition[i] != second.rawPosition[i] ||
			first.rawVelocity[i] != second.rawVelocity[i] ||
			first.rawAcceleration[i] != second.rawAcceleration[i] ||
			first.absolutePosition[i] != second.absolutePosition[i] ||
			first.referencePoint[i] != second.referencePoint[i] ||
			first.position[i] != second.position[i] ||
			first.velocity[i] != second.velocity[i] ||
			first.acceleration[i] != second.acceleration[i] ||
			first.angularVelocity[i] != second.angularVelocity[i] ||
			first.gotoPoint[i] != second.gotoPoint[i] ||
			first.orbitTargetPosition[i] != second.orbitTargetPosition[i] ||
			first.orbitTargetVelocity[i] != second.orbitTargetVelocity[i] ||
			first.orbitNormal[i] != second.orbitNormal[i] )
		{
			return false;
		}
	}
	for( size_t i = 0; i < 4; ++i )
		if( first.rotation[i] != second.rotation[i] )
			return false;
	return true;
}

bool SameCelestialState(
	const DestinyEmbeddedCelestialState& first,
	const DestinyEmbeddedCelestialState& second )
{
	if( first.ballId != second.ballId || first.mode != second.mode || first.isFree != second.isFree ||
		first.isGlobal != second.isGlobal || first.isMassive != second.isMassive ||
		first.isInteractive != second.isInteractive || first.radius != second.radius )
	{
		return false;
	}
	for( size_t i = 0; i < 3; ++i )
		if( first.position[i] != second.position[i] || first.velocity[i] != second.velocity[i] )
			return false;
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

	DestinyEmbeddedBallConfig config = MakePrimary();
	DestinyEmbeddedSessionOptions options = MakeOptions();
	char error[256] = {};
	DestinyEmbeddedSession* direct = Destiny_CreateEmbeddedSessionWithOptions(
		&config, &options, error, sizeof( error ) );
	if( !direct )
		return Fail( std::string( "direct session: " ) + error );

	std::vector<uint8_t> singlePacket;
	std::string writeError;
	if( !WritePacket( direct, singlePacket, writeError ) )
		return Fail( writeError );

	DestinyEmbeddedFixedTargetConfig fixed = {};
	fixed.ballId = kFixed;
	fixed.radius = 100.0f;
	fixed.position[2] = 500000.0;
	if( !Destiny_AddEmbeddedFixedTarget( direct, &fixed, error, sizeof( error ) ) )
		return Fail( std::string( "fixed target: " ) + error );
	DestinyEmbeddedCelestialConfig sun = {};
	sun.ballId = kSun;
	sun.radius = 158400000.0f;
	sun.position[2] = 1000000000.0;
	DestinyEmbeddedCelestialConfig planet = {};
	planet.ballId = kPlanet;
	planet.radius = 6000000.0f;
	planet.position[0] = 46895000000.0;
	if( !Destiny_AddEmbeddedCelestial( direct, &sun, error, sizeof( error ) ) ||
		!Destiny_AddEmbeddedCelestial( direct, &planet, error, sizeof( error ) ) )
	{
		return Fail( std::string( "celestial: " ) + error );
	}

	std::vector<uint8_t> fullPacket;
	if( !WritePacket( direct, fullPacket, writeError ) )
		return Fail( writeError );
	DestinyEmbeddedDiagnostics directInitial = {};
	DestinyEmbeddedCelestialState directCelestialInitial[2] = {};
	if( !Destiny_GetEmbeddedDiagnostics( direct, &directInitial ) ||
		!Destiny_GetEmbeddedCelestialState( direct, kSun, &directCelestialInitial[0] ) ||
		!Destiny_GetEmbeddedCelestialState( direct, kPlanet, &directCelestialInitial[1] ) )
	{
		return Fail( "direct celestial diagnostics failed" );
	}
	DestinyEmbeddedDiagnostics directEvolves[2] = {};
	DestinyEmbeddedCelestialState directCelestialEvolves[2][2] = {};
	for( size_t i = 0; i < 2; ++i )
	{
		if( !Destiny_AdvanceEmbeddedSession( direct, static_cast<Be::Time>( i + 1 ) * kTick ) ||
			!Destiny_GetEmbeddedDiagnostics( direct, &directEvolves[i] ) ||
			!Destiny_GetEmbeddedCelestialState( direct, kSun, &directCelestialEvolves[i][0] ) ||
			!Destiny_GetEmbeddedCelestialState( direct, kPlanet, &directCelestialEvolves[i][1] ) )
		{
			return Fail( "direct evolve failed" );
		}
	}
	Destiny_DestroyEmbeddedSession( direct );

	const DestinyEmbeddedFullStateDescriptor singleDescriptor = MakeDescriptor( false );
	const DestinyEmbeddedFullStateDescriptor fullDescriptor = MakeDescriptor( true );
	DestinyEmbeddedFullStatePreflightDiagnostics acceptedPreflight = {};
	if( !Destiny_InspectEmbeddedFullState(
			fullPacket.data(),
			fullPacket.size(),
			&fullDescriptor,
			&options,
			&acceptedPreflight,
			error,
			sizeof( error ) ) ||
		acceptedPreflight.rejection != DESTINY_EMBEDDED_FULL_STATE_ACCEPTED ||
		!acceptedPreflight.sideEffectFree || !acceptedPreflight.packetParsed ||
		!acceptedPreflight.rolesValidated || acceptedPreflight.parsedBallCount != 4 ||
		acceptedPreflight.bytesConsumed != fullPacket.size() )
	{
		return Fail( std::string( "accepted preflight diagnostics failed: " ) + error );
	}

	std::vector<uint8_t> invalid = singlePacket;
	invalid.resize( 4 );
	if( !ExpectRejected(
			invalid, singleDescriptor, options, DESTINY_EMBEDDED_FULL_STATE_REJECT_SHORT_PACKET ) )
		return Fail( "short packet accepted" );
	invalid = singlePacket;
	invalid.push_back( 0 );
	if( !ExpectRejected(
			invalid, singleDescriptor, options, DESTINY_EMBEDDED_FULL_STATE_REJECT_TRAILING_BYTES ) )
		return Fail( "trailing byte accepted" );
	invalid = singlePacket;
	invalid[0] = DESTINY_BALLS;
	if( !ExpectRejected(
			invalid, singleDescriptor, options, DESTINY_EMBEDDED_FULL_STATE_REJECT_INVALID_FRAMING ) )
		return Fail( "wrong packet framing accepted" );
	invalid = singlePacket;
	const uint8_t unknownMode = 0xff;
	Store( invalid, 13, unknownMode );
	if( !ExpectRejected(
			invalid, singleDescriptor, options, DESTINY_EMBEDDED_FULL_STATE_REJECT_UNKNOWN_MODE ) )
		return Fail( "unknown mode accepted" );
	invalid = singlePacket;
	const int64_t zeroId = 0;
	Store( invalid, 5, zeroId );
	if( !ExpectRejected(
			invalid, singleDescriptor, options, DESTINY_EMBEDDED_FULL_STATE_REJECT_INVALID_ID ) )
		return Fail( "zero ID accepted" );
	invalid = singlePacket;
	invalid.insert( invalid.end(), singlePacket.begin() + 5, singlePacket.end() );
	if( !ExpectRejected(
			invalid, singleDescriptor, options, DESTINY_EMBEDDED_FULL_STATE_REJECT_DUPLICATE_ID ) )
		return Fail( "duplicate ID accepted" );
	invalid = singlePacket;
	const float nan = std::numeric_limits<float>::quiet_NaN();
	Store( invalid, 14, nan );
	if( !ExpectRejected(
			invalid, singleDescriptor, options, DESTINY_EMBEDDED_FULL_STATE_REJECT_NONFINITE_VALUE ) )
		return Fail( "nonfinite radius accepted" );
	invalid = singlePacket;
	const float outOfRangeRadius = 1.0e20f;
	Store( invalid, 14, outOfRangeRadius );
	if( !ExpectRejected(
			invalid, singleDescriptor, options, DESTINY_EMBEDDED_FULL_STATE_REJECT_OUT_OF_RANGE_VALUE ) )
		return Fail( "out-of-range radius accepted" );
	invalid = singlePacket;
	invalid[42] |= DSTBALL_HASMINIBALLS;
	if( !ExpectRejected(
			invalid, singleDescriptor, options, DESTINY_EMBEDDED_FULL_STATE_REJECT_UNSUPPORTED_FLAGS ) )
		return Fail( "unsupported compound flag accepted" );
	invalid = singlePacket;
	const uint8_t followMode = DSTBALL_FOLLOW;
	Store( invalid, 13, followMode );
	const int64_t self = kPrimary;
	const float followRange = 50.0f;
	const uint8_t* selfBytes = reinterpret_cast<const uint8_t*>( &self );
	invalid.insert( invalid.end(), selfBytes, selfBytes + sizeof( self ) );
	const uint8_t* rangeBytes = reinterpret_cast<const uint8_t*>( &followRange );
	invalid.insert( invalid.end(), rangeBytes, rangeBytes + sizeof( followRange ) );
	if( !ExpectRejected(
			invalid, singleDescriptor, options, DESTINY_EMBEDDED_FULL_STATE_REJECT_SELF_FOLLOW ) )
		return Fail( "self follow topology accepted" );
	invalid = singlePacket;
	Store( invalid, 13, followMode );
	const int64_t missing = 99;
	const uint8_t* missingBytes = reinterpret_cast<const uint8_t*>( &missing );
	invalid.insert( invalid.end(), missingBytes, missingBytes + sizeof( missing ) );
	invalid.insert( invalid.end(), rangeBytes, rangeBytes + sizeof( followRange ) );
	if( !ExpectRejected(
			invalid, singleDescriptor, options, DESTINY_EMBEDDED_FULL_STATE_REJECT_MISSING_FOLLOW ) )
		return Fail( "missing follow topology accepted" );
	invalid = singlePacket;
	const int32_t nonzeroTimestamp = 1;
	Store( invalid, 1, nonzeroTimestamp );
	if( !ExpectRejected(
			invalid, singleDescriptor, options, DESTINY_EMBEDDED_FULL_STATE_REJECT_NONZERO_TIMESTAMP ) )
		return Fail( "nonzero initial timestamp accepted" );
	DestinyEmbeddedFullStateDescriptor badRoles = fullDescriptor;
	badRoles.celestialBallIds[1] = badRoles.fixedTargetBallId;
	if( !ExpectRejected(
			fullPacket, badRoles, options, DESTINY_EMBEDDED_FULL_STATE_REJECT_CONFLICTING_ROLES ) )
		return Fail( "conflicting role IDs accepted" );
	badRoles = fullDescriptor;
	badRoles.celestialBallIds[1] = 99;
	if( !ExpectRejected(
			fullPacket, badRoles, options, DESTINY_EMBEDDED_FULL_STATE_REJECT_MISSING_ROLES ) )
		return Fail( "missing role ID accepted" );
	badRoles = fullDescriptor;
	badRoles.fixedTargetBallId = kSun;
	badRoles.celestialBallIds[0] = kPlanet;
	badRoles.celestialBallIds[1] = 0;
	badRoles.celestialBallCount = 1;
	if( !ExpectRejected(
			fullPacket, badRoles, options, DESTINY_EMBEDDED_FULL_STATE_REJECT_INCOMPATIBLE_ROLES ) )
		return Fail( "incompatible role ID accepted" );
	badRoles = fullDescriptor;
	badRoles.wireProfile = static_cast<DestinyEmbeddedWireProfile>( 99 );
	if( !ExpectRejected(
			fullPacket, badRoles, options, DESTINY_EMBEDDED_FULL_STATE_REJECT_UNKNOWN_WIRE_PROFILE ) )
		return Fail( "unknown wire profile accepted" );
	DestinyEmbeddedFullStateDescriptor rigidObserverDescriptor = fullDescriptor;
	rigidObserverDescriptor.observerBallId = kSun;
	rigidObserverDescriptor.celestialBallIds[0] = kPlanet;
	rigidObserverDescriptor.celestialBallIds[1] = 0;
	rigidObserverDescriptor.celestialBallCount = 1;
	DestinyEmbeddedSession* rigidObserver = Destiny_CreateEmbeddedSessionFromFullState(
		fullPacket.data(), fullPacket.size(), &rigidObserverDescriptor, &options, error, sizeof( error ) );
	DestinyEmbeddedDiagnostics rigidObserverDiagnostics = {};
	if( !rigidObserver || !Destiny_GetEmbeddedDiagnostics( rigidObserver, &rigidObserverDiagnostics ) ||
		rigidObserverDiagnostics.egoBallId != kPrimary || rigidObserverDiagnostics.observerBallId != kSun ||
		!Destiny_GetEmbeddedBallPosition( rigidObserver, kSun ) )
	{
		return Fail( std::string( "RIGID inactive observer restoration failed: " ) + error );
	}
	Destiny_DestroyEmbeddedSession( rigidObserver );

	DestinyEmbeddedFullStateCreationDiagnostics beforeRestored = {};
	DestinyEmbeddedFullStateCreationDiagnostics afterRestored = {};
	if( !Destiny_GetEmbeddedFullStateCreationDiagnostics( &beforeRestored ) )
		return Fail( "full-state creation diagnostics snapshot failed" );
	DestinyEmbeddedSession* restored = Destiny_CreateEmbeddedSessionFromFullState(
		fullPacket.data(), fullPacket.size(), &fullDescriptor, &options, error, sizeof( error ) );
	if( !restored || !Destiny_GetEmbeddedFullStateCreationDiagnostics( &afterRestored ) ||
		!SuccessfulCreationDelta( beforeRestored, afterRestored ) )
		return Fail( std::string( "restored session: " ) + error );
	DestinyEmbeddedDiagnostics restoredInitial = {};
	DestinyEmbeddedCelestialState restoredCelestialInitial[2] = {};
	if( !Destiny_GetEmbeddedDiagnostics( restored, &restoredInitial ) ||
		!SameLiveState( directInitial, restoredInitial ) ||
		!Destiny_GetEmbeddedCelestialState( restored, kSun, &restoredCelestialInitial[0] ) ||
		!Destiny_GetEmbeddedCelestialState( restored, kPlanet, &restoredCelestialInitial[1] ) ||
		!SameCelestialState( directCelestialInitial[0], restoredCelestialInitial[0] ) ||
		!SameCelestialState( directCelestialInitial[1], restoredCelestialInitial[1] ) )
	{
		return Fail( "restored initial celestial diagnostics diverged" );
	}
	if( !Destiny_GetEmbeddedBallPosition( restored, kPrimary ) ||
		!Destiny_GetEmbeddedBallRotation( restored, kPrimary ) ||
		!Destiny_GetEmbeddedBallPosition( restored, kFixed ) ||
		!Destiny_GetEmbeddedBallPosition( restored, kSun ) ||
		Destiny_GetEmbeddedBallPosition( restored, 99 ) )
	{
		return Fail( "generic ball curve lookup failed" );
	}
	std::vector<uint8_t> fixedPoint;
	if( !WritePacket( restored, fixedPoint, writeError ) )
		return Fail( writeError );
	if( fixedPoint != fullPacket )
	{
		size_t different = 0;
		while( different < fixedPoint.size() && different < fullPacket.size() &&
			   fixedPoint[different] == fullPacket[different] )
		{
			++different;
		}
		char detail[160] = {};
		std::snprintf(
			detail,
			sizeof( detail ),
			"write-read-write fixed point diverged (offset=%zu source-size=%zu restored-size=%zu)",
			different,
			fullPacket.size(),
			fixedPoint.size() );
		return Fail( detail );
	}
	for( size_t i = 0; i < 2; ++i )
	{
		DestinyEmbeddedDiagnostics restoredDiagnostics = {};
		DestinyEmbeddedCelestialState restoredCelestials[2] = {};
		if( !Destiny_AdvanceEmbeddedSession( restored, static_cast<Be::Time>( i + 1 ) * kTick ) ||
			!Destiny_GetEmbeddedDiagnostics( restored, &restoredDiagnostics ) ||
			!Destiny_GetEmbeddedCelestialState( restored, kSun, &restoredCelestials[0] ) ||
			!Destiny_GetEmbeddedCelestialState( restored, kPlanet, &restoredCelestials[1] ) ||
			!SameLiveState( directEvolves[i], restoredDiagnostics ) ||
			!SameCelestialState( directCelestialEvolves[i][0], restoredCelestials[0] ) ||
			!SameCelestialState( directCelestialEvolves[i][1], restoredCelestials[1] ) )
		{
			return Fail( "first-two-evolve state diverged" );
		}
	}
	if( Destiny_CommandEmbeddedFollow( restored, 3 * kTick, kSun, 50.0f ) ||
		!Destiny_CommandEmbeddedFollow( restored, 3 * kTick, kFixed, 50.0f ) )
	{
		return Fail( "manifest role command policy diverged" );
	}
	Destiny_DestroyEmbeddedSession( restored );

	DestinyEmbeddedSessionOptions observerOptions = MakeOptions();
	observerOptions.referenceFrame = DESTINY_EMBEDDED_FIXED_OBSERVER;
	observerOptions.observerBallId = 5;
	observerOptions.observerPosition[0] = 10000.0;
	DestinyEmbeddedSession* observerDirect = Destiny_CreateEmbeddedSessionWithOptions(
		&config, &observerOptions, error, sizeof( error ) );
	if( !observerDirect )
		return Fail( std::string( "observer direct session: " ) + error );
	std::vector<uint8_t> observerPacket;
	if( !WritePacket( observerDirect, observerPacket, writeError ) )
		return Fail( writeError );
	Destiny_DestroyEmbeddedSession( observerDirect );
	DestinyEmbeddedSessionOptions independentObserverOptions = MakeOptions();
	DestinyEmbeddedFullStateDescriptor independentObserverDescriptor = MakeDescriptor( false );
	independentObserverDescriptor.observerBallId = observerOptions.observerBallId;
	DestinyEmbeddedSession* independentObserver = Destiny_CreateEmbeddedSessionFromFullState(
		observerPacket.data(), observerPacket.size(), &independentObserverDescriptor, &independentObserverOptions, error, sizeof( error ) );
	DestinyEmbeddedDiagnostics independentObserverDiagnostics = {};
	if( !independentObserver ||
		!Destiny_GetEmbeddedDiagnostics( independentObserver, &independentObserverDiagnostics ) ||
		independentObserverDiagnostics.egoBallId != kPrimary ||
		independentObserverDiagnostics.observerBallId != observerOptions.observerBallId ||
		!Destiny_GetEmbeddedBallPosition( independentObserver, observerOptions.observerBallId ) )
	{
		return Fail( std::string( "independent observer restoration failed: " ) + error );
	}
	Destiny_DestroyEmbeddedSession( independentObserver );
	DestinyEmbeddedFullStateDescriptor observerDescriptor = MakeDescriptor( false );
	observerDescriptor.egoBallId = observerOptions.observerBallId;
	observerDescriptor.observerBallId = observerOptions.observerBallId;
	DestinyEmbeddedSession* observerRestored = Destiny_CreateEmbeddedSessionFromFullState(
		observerPacket.data(), observerPacket.size(), &observerDescriptor, &observerOptions, error, sizeof( error ) );
	DestinyEmbeddedDiagnostics observerDiagnostics = {};
	if( !observerRestored || !Destiny_GetEmbeddedDiagnostics( observerRestored, &observerDiagnostics ) ||
		observerDiagnostics.egoBallId != observerOptions.observerBallId ||
		observerDiagnostics.observerBallId != observerOptions.observerBallId ||
		!Destiny_GetEmbeddedBallPosition( observerRestored, observerOptions.observerBallId ) )
	{
		return Fail( std::string( "fixed-observer restoration failed: " ) + error );
	}
	Destiny_DestroyEmbeddedSession( observerRestored );

	std::printf(
		"Destiny PL-14G1 full-state contract: rejects=16 preflight-side-effect-free=true "
		"reject-before-allocation=true fixed-point=true live-diagnostics=initial+2-evolves "
		"roles=true observer=independent/fixed start-once=true\n" );
	return 0;
}
