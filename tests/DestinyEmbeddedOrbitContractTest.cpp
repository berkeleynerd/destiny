// Copyright (c) 2026 CCP Games

#include "StdAfx.h"

#include "DestinyEmbedded.h"
#include "DstConstants.h"

#include <Blue.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

const char* g_moduleName = "DestinyEmbeddedOrbitContractTest";

BLUE_DEFINE_INTERFACE( ITriVectorFunction );
BLUE_DEFINE_INTERFACE( ITriQuaternionFunction );
BLUE_DEFINE_INTERFACE( IEveReferencePoint );
BLUE_DEFINE_INTERFACE( IEveBallpark );

namespace
{
constexpr Be::Time kTicksPerSecond = 10000000;
constexpr uint64_t kFrameCount = 3780;
constexpr uint64_t kCommandFrame = 180;

struct Sample
{
	std::array<double, 3> position;
	std::array<double, 3> velocity;
	std::array<double, 3> acceleration;

	bool operator==( const Sample& other ) const
	{
		return position == other.position && velocity == other.velocity && acceleration == other.acceleration;
	}
};

int Fail( const std::string& message )
{
	std::fprintf( stderr, "DestinyEmbeddedOrbitContractTest: %s\n", message.c_str() );
	return 1;
}

bool Near( double a, double b, double epsilon )
{
	return std::abs( a - b ) <= epsilon;
}

bool LoadCorpus( std::vector<Sample>& samples )
{
	std::ifstream input( DESTINY_MOTION_CORPUS_DIR "/pl11b-orbit-new.csv" );
	std::string line;
	if( !input || !std::getline( input, line ) )
		return false;
	while( std::getline( input, line ) )
	{
		std::replace( line.begin(), line.end(), ',', ' ' );
		std::istringstream row( line );
		uint32_t tick = 0;
		Sample sample = {};
		if( !( row >> tick ) || tick != samples.size() + 1 )
			return false;
		for( double& value : sample.position )
			if( !( row >> value ) )
				return false;
		for( double& value : sample.velocity )
			if( !( row >> value ) )
				return false;
		for( double& value : sample.acceleration )
			if( !( row >> value ) )
				return false;
		samples.push_back( sample );
	}
	return samples.size() == 60;
}

bool RunScenario(
	DestinyEmbeddedReferenceFrame referenceFrame,
	const std::vector<Sample>& expected,
	std::vector<Sample>& actual,
	std::string& error )
{
	DestinyEmbeddedBallConfig config = {};
	config.ballId = 1;
	config.solarSystemId = 30005286;
	config.mass = 975000.0;
	config.radius = 35.0f;
	config.maximumVelocity = 312.0f;
	config.maximumAngularVelocity = 1.0f;
	config.position[2] = -2570.0;
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
	options.referenceFrame = referenceFrame;
	options.orbitPolicy = DESTINY_EMBEDDED_ORBIT_FRONTIER_NEW;
	options.observerBallId = 2;
	char createError[512] = {};
	DestinyEmbeddedSession* session = Destiny_CreateEmbeddedSessionWithOptions(
		&config, &options, createError, sizeof( createError ) );
	if( !session )
	{
		error = createError;
		return false;
	}

	DestinyEmbeddedFixedTargetConfig target = {};
	target.ballId = 3;
	target.radius = 35.0f;
	if( !Destiny_AddEmbeddedFixedTarget( session, &target, createError, sizeof( createError ) ) ||
		Destiny_AddEmbeddedFixedTarget( session, &target, createError, sizeof( createError ) ) )
	{
		error = "fixed-target contract failed";
		Destiny_DestroyEmbeddedSession( session );
		return false;
	}

	uint64_t previousEvolves = 0;
	for( uint64_t frame = 0; frame < kFrameCount; ++frame )
	{
		const Be::Time simulationTime = static_cast<Be::Time>( frame * kTicksPerSecond / 60 );
		if( frame == kCommandFrame )
		{
			if( !Destiny_CommandEmbeddedOrbit( session, simulationTime, 3, 2500.0f ) ||
				Destiny_CommandEmbeddedStop( session, simulationTime ) )
			{
				error = "tick-aligned ORBIT command contract failed";
				Destiny_DestroyEmbeddedSession( session );
				return false;
			}
		}
		if( !Destiny_AdvanceEmbeddedSession( session, simulationTime ) )
		{
			error = "deterministic advance failed";
			Destiny_DestroyEmbeddedSession( session );
			return false;
		}
		DestinyEmbeddedDiagnostics diagnostics = {};
		if( !Destiny_GetEmbeddedDiagnostics( session, &diagnostics ) )
		{
			error = "diagnostic query failed";
			Destiny_DestroyEmbeddedSession( session );
			return false;
		}
		const double qLength = std::sqrt(
			diagnostics.rotation[0] * diagnostics.rotation[0] +
			diagnostics.rotation[1] * diagnostics.rotation[1] +
			diagnostics.rotation[2] * diagnostics.rotation[2] +
			diagnostics.rotation[3] * diagnostics.rotation[3] );
		if( !diagnostics.frontierOrbitEnabled || !Near( qLength, 1.0, 1e-5 ) ||
			diagnostics.orientationPinCount != 0 || diagnostics.schedulerRegistered ||
			diagnostics.startCallCount || diagnostics.onTickCallCount || diagnostics.pythonCallbackCount )
		{
			error = "embedded runtime or orientation contract failed";
			Destiny_DestroyEmbeddedSession( session );
			return false;
		}
		if( diagnostics.directEvolveCount != previousEvolves )
		{
			previousEvolves = diagnostics.directEvolveCount;
			if( previousEvolves <= 2 )
			{
				if( !Near( diagnostics.rawPosition[2], -2570.0, 1e-10 ) )
				{
					error = "STOP state moved before ORBIT";
					Destiny_DestroyEmbeddedSession( session );
					return false;
				}
			}
			else
			{
				const size_t index = static_cast<size_t>( previousEvolves - 3 );
				if( index >= expected.size() )
				{
					error = "too many orbit samples";
					Destiny_DestroyEmbeddedSession( session );
					return false;
				}
				Sample sample = {};
				for( size_t axis = 0; axis < 3; ++axis )
				{
					sample.position[axis] = diagnostics.rawPosition[axis];
					sample.velocity[axis] = diagnostics.rawVelocity[axis];
					sample.acceleration[axis] = diagnostics.rawAcceleration[axis];
					if( !Near( sample.position[axis], expected[index].position[axis], 1e-5 ) ||
						!Near( sample.velocity[axis], expected[index].velocity[axis], 1e-5 ) ||
						!Near( sample.acceleration[axis], expected[index].acceleration[axis], 1e-5 ) )
					{
						char detail[512] = {};
						std::snprintf( detail, sizeof( detail ),
							"Frontier orbit tick %zu axis %zu diverged: p=%.12g/%.12g v=%.12g/%.12g a=%.12g/%.12g",
							index + 1, axis, sample.position[axis], expected[index].position[axis],
							sample.velocity[axis], expected[index].velocity[axis],
							sample.acceleration[axis], expected[index].acceleration[axis] );
						error = detail;
						Destiny_DestroyEmbeddedSession( session );
						return false;
					}
				}
				actual.push_back( sample );
			}
		}
	}

	DestinyEmbeddedDiagnostics final = {};
	const bool finalOk = Destiny_GetEmbeddedDiagnostics( session, &final );
	Destiny_DestroyEmbeddedSession( session );
	if( !finalOk || final.directEvolveCount != 62 || final.commandCount != 1 ||
		final.mode != DSTBALL_ORBIT || final.followBallId != 3 || !Near( final.followRange, 2500.0, 1e-6 ) ||
		final.orbitTargetBallId != 3 || final.orbitAccumulatedPhase < 1.96 * 3.141592653589793 ||
		final.orbitAccumulatedPhase > 2.04 * 3.141592653589793 ||
		std::abs( final.orbitCenterDistance - 2570.0 ) / 2570.0 > 0.035 )
	{
		error = "final Frontier orbit state failed";
		return false;
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
	if( !Destiny_RegisterBlueClasses( &registration ) || registration.discoveredClassCount != 10 )
		return Fail( "embedded registration failed" );
	std::vector<Sample> expected;
	if( !LoadCorpus( expected ) || !std::ifstream( DESTINY_MOTION_CORPUS_DIR "/pl11b-provenance.json" ) )
		return Fail( "orbit corpus is missing or invalid" );

	std::vector<Sample> egoA, egoB, observer;
	std::string error;
	if( !RunScenario( DESTINY_EMBEDDED_PRIMARY_EGO, expected, egoA, error ) ||
		!RunScenario( DESTINY_EMBEDDED_PRIMARY_EGO, expected, egoB, error ) ||
		!RunScenario( DESTINY_EMBEDDED_FIXED_OBSERVER, expected, observer, error ) )
		return Fail( error );
	if( egoA != egoB || egoA != observer )
		return Fail( "orbit trajectory is not deterministic across runs and reference frames" );
	std::printf( "Destiny PL-11B Frontier orbit contract: samples=60 evolves=62 range=2500 phase=one-turn\n" );
	return 0;
}
