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
#include <limits>
#include <sstream>
#include <string>
#include <vector>

const char* g_moduleName = "DestinyEmbeddedCelestialContractTest";

BLUE_DEFINE_INTERFACE( ITriVectorFunction );
BLUE_DEFINE_INTERFACE( ITriQuaternionFunction );
BLUE_DEFINE_INTERFACE( IEveReferencePoint );
BLUE_DEFINE_INTERFACE( IEveBallpark );

namespace
{
constexpr Be::Time kTicksPerSecond = 10000000;
constexpr uint64_t kFrameCount = 3780;
constexpr uint64_t kCommandFrame = 180;

constexpr int64_t kSunBallId = 40334263;
constexpr float kSunRadius = 158400000.0f;
constexpr double kSunPosition[3] = { 1069486940160.0, -202669301760.0, -831868968960.0 };
constexpr int64_t kPlanetBallId = 40334264;
constexpr float kPlanetRadius = 2630000.0f;
constexpr double kPlanetPosition[3] = { 1083758787326.0, -205372890997.0, -787280443197.0 };

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
	std::fprintf( stderr, "DestinyEmbeddedCelestialContractTest: %s\n", message.c_str() );
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

DestinyEmbeddedSession* CreateOrbitSession( DestinyEmbeddedReferenceFrame referenceFrame, std::string& error )
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
		error = createError;
	return session;
}

bool AddOrbitTarget( DestinyEmbeddedSession* session, std::string& error )
{
	DestinyEmbeddedFixedTargetConfig target = {};
	target.ballId = 3;
	target.radius = 35.0f;
	char addError[512] = {};
	if( !Destiny_AddEmbeddedFixedTarget( session, &target, addError, sizeof( addError ) ) )
	{
		error = addError;
		return false;
	}
	return true;
}

bool AddAuthoredCelestials( DestinyEmbeddedSession* session, std::string& error )
{
	char addError[512] = {};
	DestinyEmbeddedCelestialConfig sun = {};
	sun.ballId = kSunBallId;
	sun.radius = kSunRadius;
	for( size_t axis = 0; axis < 3; ++axis )
		sun.position[axis] = kSunPosition[axis];
	DestinyEmbeddedCelestialConfig planet = {};
	planet.ballId = kPlanetBallId;
	planet.radius = kPlanetRadius;
	for( size_t axis = 0; axis < 3; ++axis )
		planet.position[axis] = kPlanetPosition[axis];
	if( !Destiny_AddEmbeddedCelestial( session, &sun, addError, sizeof( addError ) ) ||
		!Destiny_AddEmbeddedCelestial( session, &planet, addError, sizeof( addError ) ) )
	{
		error = addError;
		return false;
	}
	return true;
}

bool CheckCelestialState(
	DestinyEmbeddedSession* session,
	int64_t ballId,
	float expectedRadius,
	const double expectedPosition[3],
	std::string& error )
{
	DestinyEmbeddedCelestialState state = {};
	if( !Destiny_GetEmbeddedCelestialState( session, ballId, &state ) )
	{
		error = "celestial state query failed";
		return false;
	}
	if( state.ballId != ballId || state.mode != DSTBALL_RIGID || state.isFree || !state.isGlobal ||
		state.isMassive || state.isInteractive || state.radius != expectedRadius )
	{
		error = "celestial mode, flag, or radius contract failed";
		return false;
	}
	for( size_t axis = 0; axis < 3; ++axis )
	{
		if( state.position[axis] != expectedPosition[axis] || state.velocity[axis] != 0.0 )
		{
			error = "celestial position or velocity contract failed";
			return false;
		}
	}
	return true;
}

bool CheckCelestialCurve(
	DestinyEmbeddedSession* session,
	int64_t ballId,
	const double expectedPosition[3],
	Be::Time time,
	std::string& error )
{
	ITriVectorFunction* curve = Destiny_GetEmbeddedCelestialPosition( session, ballId );
	if( !curve )
	{
		error = "celestial position curve is missing";
		return false;
	}
	DestinyEmbeddedDiagnostics diagnostics = {};
	if( !Destiny_GetEmbeddedDiagnostics( session, &diagnostics ) )
	{
		error = "diagnostic query failed";
		return false;
	}
	Vector3 value( 0.0f, 0.0f, 0.0f );
	curve->GetValueAt( &value, time );
	for( size_t axis = 0; axis < 3; ++axis )
	{
		const double actual = static_cast<double>( ( &value.x )[axis] );
		const double expected = expectedPosition[axis] - diagnostics.referencePoint[axis];
		const double tolerance = std::max( 1.0, std::abs( expected ) ) * 1e-6;
		if( !Near( actual, expected, tolerance ) )
		{
			char detail[256] = {};
			std::snprintf( detail, sizeof( detail ),
				"celestial %lld curve axis %zu diverged: %.12g expected %.12g",
				static_cast<long long>( ballId ), axis, actual, expected );
			error = detail;
			return false;
		}
	}
	return true;
}

bool CheckAdditionConstraints( std::string& error )
{
	DestinyEmbeddedSession* session = CreateOrbitSession( DESTINY_EMBEDDED_PRIMARY_EGO, error );
	if( !session )
		return false;
	char addError[512] = {};
	DestinyEmbeddedCelestialConfig celestial = {};
	celestial.ballId = kSunBallId;
	celestial.radius = kSunRadius;
	celestial.position[0] = kSunPosition[0];
	celestial.position[1] = kSunPosition[1];
	celestial.position[2] = kSunPosition[2];

	DestinyEmbeddedCelestialConfig invalid = celestial;
	invalid.ballId = 0;
	DestinyEmbeddedCelestialConfig primaryCollision = celestial;
	primaryCollision.ballId = 1;
	DestinyEmbeddedCelestialConfig nonFinite = celestial;
	nonFinite.position[1] = std::numeric_limits<double>::infinity();
	DestinyEmbeddedCelestialConfig zeroRadius = celestial;
	zeroRadius.ballId = 900;
	zeroRadius.radius = 0.0f;
	if( Destiny_AddEmbeddedCelestial( session, &invalid, addError, sizeof( addError ) ) ||
		Destiny_AddEmbeddedCelestial( session, &primaryCollision, addError, sizeof( addError ) ) ||
		Destiny_AddEmbeddedCelestial( session, &nonFinite, addError, sizeof( addError ) ) ||
		Destiny_AddEmbeddedCelestial( session, &zeroRadius, addError, sizeof( addError ) ) )
	{
		error = "invalid celestial configuration was accepted";
		Destiny_DestroyEmbeddedSession( session );
		return false;
	}
	if( !Destiny_AddEmbeddedCelestial( session, &celestial, addError, sizeof( addError ) ) ||
		Destiny_AddEmbeddedCelestial( session, &celestial, addError, sizeof( addError ) ) )
	{
		error = "duplicate celestial contract failed";
		Destiny_DestroyEmbeddedSession( session );
		return false;
	}
	DestinyEmbeddedCelestialConfig filler = celestial;
	for( int64_t ballId = 901; ballId <= 903; ++ballId )
	{
		filler.ballId = ballId;
		if( !Destiny_AddEmbeddedCelestial( session, &filler, addError, sizeof( addError ) ) )
		{
			error = "celestial capacity contract failed before the cap";
			Destiny_DestroyEmbeddedSession( session );
			return false;
		}
	}
	filler.ballId = 904;
	if( Destiny_AddEmbeddedCelestial( session, &filler, addError, sizeof( addError ) ) )
	{
		error = "celestial capacity cap was not enforced";
		Destiny_DestroyEmbeddedSession( session );
		return false;
	}
	if( !Destiny_AdvanceEmbeddedSession( session, kTicksPerSecond * 2 ) )
	{
		error = "constraint-session advance failed";
		Destiny_DestroyEmbeddedSession( session );
		return false;
	}
	filler.ballId = 905;
	const bool postAdvanceRejected =
		!Destiny_AddEmbeddedCelestial( session, &filler, addError, sizeof( addError ) );
	Destiny_DestroyEmbeddedSession( session );
	if( !postAdvanceRejected )
	{
		error = "celestial addition after advance was accepted";
		return false;
	}
	return true;
}

bool RunScenario(
	DestinyEmbeddedReferenceFrame referenceFrame,
	bool withCelestials,
	const std::vector<Sample>& expected,
	std::vector<Sample>& actual,
	std::string& error )
{
	DestinyEmbeddedSession* session = CreateOrbitSession( referenceFrame, error );
	if( !session )
		return false;
	if( !AddOrbitTarget( session, error ) )
	{
		Destiny_DestroyEmbeddedSession( session );
		return false;
	}
	if( withCelestials )
	{
		if( !AddAuthoredCelestials( session, error ) ||
			!CheckCelestialState( session, kSunBallId, kSunRadius, kSunPosition, error ) ||
			!CheckCelestialState( session, kPlanetBallId, kPlanetRadius, kPlanetPosition, error ) )
		{
			Destiny_DestroyEmbeddedSession( session );
			return false;
		}
		if( Destiny_GetEmbeddedCelestialPosition( session, 12345 ) != nullptr )
		{
			error = "unknown celestial id returned a curve";
			Destiny_DestroyEmbeddedSession( session );
			return false;
		}
	}

	uint64_t previousEvolves = 0;
	for( uint64_t frame = 0; frame < kFrameCount; ++frame )
	{
		const Be::Time simulationTime = static_cast<Be::Time>( frame * kTicksPerSecond / 60 );
		if( frame == kCommandFrame && !Destiny_CommandEmbeddedOrbit( session, simulationTime, 3, 2500.0f ) )
		{
			error = "tick-aligned ORBIT command contract failed";
			Destiny_DestroyEmbeddedSession( session );
			return false;
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
		if( diagnostics.startCallCount || diagnostics.onTickCallCount || diagnostics.pythonCallbackCount ||
			diagnostics.schedulerRegistered )
		{
			error = "embedded runtime contract failed";
			Destiny_DestroyEmbeddedSession( session );
			return false;
		}
		if( diagnostics.directEvolveCount != previousEvolves )
		{
			previousEvolves = diagnostics.directEvolveCount;
			if( previousEvolves > 2 )
			{
				Sample sample = {};
				for( size_t axis = 0; axis < 3; ++axis )
				{
					sample.position[axis] = diagnostics.rawPosition[axis];
					sample.velocity[axis] = diagnostics.rawVelocity[axis];
					sample.acceleration[axis] = diagnostics.rawAcceleration[axis];
				}
				actual.push_back( sample );
			}
			if( withCelestials && ( previousEvolves == 1 || previousEvolves == 32 || previousEvolves == 62 ) )
			{
				if( !CheckCelestialCurve( session, kSunBallId, kSunPosition, diagnostics.lastRequestedTime, error ) ||
					!CheckCelestialCurve( session, kPlanetBallId, kPlanetPosition, diagnostics.lastRequestedTime, error ) )
				{
					Destiny_DestroyEmbeddedSession( session );
					return false;
				}
			}
		}
	}

	bool finalOk = true;
	if( withCelestials )
	{
		finalOk = CheckCelestialState( session, kSunBallId, kSunRadius, kSunPosition, error ) &&
			CheckCelestialState( session, kPlanetBallId, kPlanetRadius, kPlanetPosition, error );
		if( finalOk )
		{
			char addError[512] = {};
			DestinyEmbeddedCelestialConfig late = {};
			late.ballId = 906;
			late.radius = 1.0f;
			if( Destiny_AddEmbeddedCelestial( session, &late, addError, sizeof( addError ) ) )
			{
				error = "celestial addition after commands was accepted";
				finalOk = false;
			}
		}
	}
	DestinyEmbeddedDiagnostics final = {};
	const bool diagnosticsOk = Destiny_GetEmbeddedDiagnostics( session, &final );
	Destiny_DestroyEmbeddedSession( session );
	if( !finalOk )
		return false;
	if( !diagnosticsOk || final.directEvolveCount != 62 || final.commandCount != 1 ||
		final.mode != DSTBALL_ORBIT || final.followBallId != 3 )
	{
		error = "final orbit state failed with celestials present";
		return false;
	}
	if( actual.size() != expected.size() )
	{
		error = "unexpected orbit sample count";
		return false;
	}
	for( size_t index = 0; index < expected.size(); ++index )
	{
		for( size_t axis = 0; axis < 3; ++axis )
		{
			if( !Near( actual[index].position[axis], expected[index].position[axis], 1e-5 ) ||
				!Near( actual[index].velocity[axis], expected[index].velocity[axis], 1e-5 ) ||
				!Near( actual[index].acceleration[axis], expected[index].acceleration[axis], 1e-5 ) )
			{
				error = "orbit corpus diverged";
				return false;
			}
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
	if( !Destiny_RegisterBlueClasses( &registration ) || registration.discoveredClassCount != 10 )
		return Fail( "embedded registration failed" );
	std::vector<Sample> expected;
	if( !LoadCorpus( expected ) )
		return Fail( "orbit corpus is missing or invalid" );

	std::string error;
	if( !CheckAdditionConstraints( error ) )
		return Fail( error );

	std::vector<Sample> egoBaseline, egoCelestial, observerCelestial;
	if( !RunScenario( DESTINY_EMBEDDED_PRIMARY_EGO, false, expected, egoBaseline, error ) ||
		!RunScenario( DESTINY_EMBEDDED_PRIMARY_EGO, true, expected, egoCelestial, error ) ||
		!RunScenario( DESTINY_EMBEDDED_FIXED_OBSERVER, true, expected, observerCelestial, error ) )
		return Fail( error );
	if( !( egoBaseline == egoCelestial ) || !( egoCelestial == observerCelestial ) )
		return Fail( "celestial balls perturbed the accepted orbit trajectory" );
	std::printf(
		"Destiny PL-12 celestial contract: balls=%lld,%lld rigid global fixed; orbit trajectory preserved\n",
		static_cast<long long>( kSunBallId ),
		static_cast<long long>( kPlanetBallId ) );
	return 0;
}
