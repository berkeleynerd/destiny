// Copyright (c) 2026 CCP Games
//
// D-04 scripted-scenario recorder: runs the PL-11A GOTO scenario twice
// and pins determinism — both recordings must be byte-identical. Each
// recording is a header (magic, tick rate, RNG-seed-absent marker) plus
// one FULLSTATE packet per evolve, prefixed by its simulation tick.

#include "StdAfx.h"

#include "DestinyEmbedded.h"
#include "DstConstants.h"

#include <Blue.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

const char* g_moduleName = "DestinyEmbeddedRecorderTest";

BLUE_DEFINE_INTERFACE( ITriVectorFunction );
BLUE_DEFINE_INTERFACE( ITriQuaternionFunction );
BLUE_DEFINE_INTERFACE( IEveReferencePoint );
BLUE_DEFINE_INTERFACE( IEveBallpark );

namespace
{
constexpr Be::Time kTicksPerSecond = 10000000;
constexpr uint64_t kFrameCount = 1200;
constexpr uint64_t kCommandFrame = 180;
constexpr uint32_t kRecordingMagic = 0x44503034;  // "D-04"
constexpr uint8_t kRngSeedAbsent = 0;

int Fail( const std::string& message )
{
	std::fprintf( stderr, "DestinyEmbeddedRecorderTest: %s\n", message.c_str() );
	return 1;
}

void Append( std::vector<uint8_t>& out, const void* data, size_t size )
{
	const uint8_t* bytes = static_cast<const uint8_t*>( data );
	out.insert( out.end(), bytes, bytes + size );
}

bool RecordScenario( std::vector<uint8_t>& recording, uint64_t& packets, std::string& error )
{
	DestinyEmbeddedBallConfig config = {};
	config.ballId = 1;
	config.solarSystemId = 30005286;
	config.mass = 975000.0;
	config.radius = 35.0f;
	config.maximumVelocity = 312.0f;
	config.maximumAngularVelocity = 1.0f;
	config.position[2] = -1000.0;
	config.rotation[2] = 0.3826834323650898f;
	config.rotation[3] = 0.9238795325112867f;
	config.agility = 2.87f;
	config.rotationalAgility = 1.0f;
	config.speedFraction = 1.0f;
	config.isFree = true;
	config.isMassive = true;
	config.isInteractive = true;

	DestinyEmbeddedSessionOptions options = {};
	options.orientationPolicy = DESTINY_EMBEDDED_NATIVE_ORIENTATION;
	options.referenceFrame = DESTINY_EMBEDDED_PRIMARY_EGO;
	options.observerBallId = 2;

	char createError[512] = {};
	DestinyEmbeddedSession* session = Destiny_CreateEmbeddedSessionWithOptions(
		&config, &options, createError, sizeof( createError ) );
	if( !session )
	{
		error = createError;
		return false;
	}

	recording.clear();
	packets = 0;
	Append( recording, &kRecordingMagic, sizeof( kRecordingMagic ) );
	const int64_t tickRate = kTicksPerSecond;
	Append( recording, &tickRate, sizeof( tickRate ) );
	Append( recording, &kRngSeedAbsent, sizeof( kRngSeedAbsent ) );

	const double target[3] = { 0.0, 0.0, 1000.0 };
	uint64_t previousEvolve = 0;
	std::vector<uint8_t> packet( 1 << 16 );

	for( uint64_t frame = 0; frame < kFrameCount; ++frame )
	{
		const Be::Time simulationTime =
			static_cast<Be::Time>( frame * kTicksPerSecond / 60 );
		if( frame == kCommandFrame &&
			!Destiny_CommandEmbeddedGoto( session, simulationTime, target ) )
		{
			error = "GOTO command failed";
			Destiny_DestroyEmbeddedSession( session );
			return false;
		}
		if( !Destiny_AdvanceEmbeddedSession( session, simulationTime ) )
		{
			error = "advance failed";
			Destiny_DestroyEmbeddedSession( session );
			return false;
		}
		DestinyEmbeddedDiagnostics diagnostics = {};
		if( !Destiny_GetEmbeddedDiagnostics( session, &diagnostics ) )
		{
			error = "diagnostics failed";
			Destiny_DestroyEmbeddedSession( session );
			return false;
		}
		if( diagnostics.directEvolveCount == previousEvolve )
			continue;
		previousEvolve = diagnostics.directEvolveCount;

		size_t written = 0;
		if( !Destiny_WriteEmbeddedFullState(
				session, packet.data(), packet.size(), &written ) )
		{
			error = "full-state write failed";
			Destiny_DestroyEmbeddedSession( session );
			return false;
		}
		if( written < 5 || packet[0] != 0 )
		{
			error = "packet framing wrong (magic byte or size)";
			Destiny_DestroyEmbeddedSession( session );
			return false;
		}
		const int64_t tick = simulationTime;
		Append( recording, &tick, sizeof( tick ) );
		const uint32_t size32 = uint32_t( written );
		Append( recording, &size32, sizeof( size32 ) );
		Append( recording, packet.data(), written );
		++packets;
	}

	Destiny_DestroyEmbeddedSession( session );
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

	std::vector<uint8_t> first, second;
	uint64_t firstPackets = 0, secondPackets = 0;
	std::string error;
	if( !RecordScenario( first, firstPackets, error ) )
		return Fail( "first recording: " + error );
	if( !RecordScenario( second, secondPackets, error ) )
		return Fail( "second recording: " + error );

	if( firstPackets == 0 || firstPackets != secondPackets )
		return Fail( "packet counts diverged between runs" );
	if( first.size() != second.size() ||
		std::memcmp( first.data(), second.data(), first.size() ) != 0 )
		return Fail( "recordings are not byte-identical" );

	std::printf(
		"Destiny D-04 recorder contract: packets=%llu bytes=%zu deterministic=true rng-seed=absent\n",
		static_cast<unsigned long long>( firstPackets ), first.size() );
	return 0;
}
