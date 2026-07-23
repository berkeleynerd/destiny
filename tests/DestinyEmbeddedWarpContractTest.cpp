// Copyright (c) 2026 CCP Games

#include "StdAfx.h"

#include "DestinyEmbedded.h"
#include "DstConstants.h"

#include <Blue.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

const char* g_moduleName = "DestinyEmbeddedWarpContractTest";

BLUE_DEFINE_INTERFACE( ITriVectorFunction );
BLUE_DEFINE_INTERFACE( ITriQuaternionFunction );
BLUE_DEFINE_INTERFACE( IEveReferencePoint );
BLUE_DEFINE_INTERFACE( IEveBallpark );

namespace
{
constexpr Be::Time kTicksPerSecond = 10000000;
constexpr uint64_t kFrameCount = 2700; // 45 native ticks: align + ~20 s warp + settle
constexpr uint64_t kCommandFrame = 180; // effective tick 3, the PL convention

// PL-11C fixture: warp from the stargate anchor to the New Eden planet.
// Position is the accepted PL-12 stargate-anchored value; warpFactor 5000
// is 5.0 AU/s from the Astero's recorded warpSpeedMultiplier 5.0
// (tests/data/pl11a-provenance.json); the minimum range clears the
// 2,630,000 m planet radius at the client's ~1e7 m arrival convention.
constexpr double kWarpTarget[3] = { 1083758787326.0, -205372890997.0, -787280443197.0 };
constexpr double kMinimumRange = 1.0e7;
constexpr int32_t kWarpFactor = 5000;

// Recovered sim constants (destiny/src/Ballpark.cpp:66-69, Ballpark.h:31).
constexpr double kAu = 0.1495978707e12;
constexpr double kWarpFactorToAuPerSecond = 0.001;
constexpr double kWarpFactorToAcceleration = 1.0 / 1000.0;
constexpr double kWarpFactorToDeceleration = 1.0 / 3000.0;
constexpr double kMaximumVelocity = 312.0;

struct ReferenceSample
{
	std::array<double, 3> position;
	std::array<double, 3> velocity;
	std::array<double, 3> acceleration;
};

struct ScenarioResult
{
	std::vector<ReferenceSample> rawMotion;
	uint64_t aligningTicks = 0;
	int64_t activationEvolve = -1;
	int64_t dropoutEvolve = -1;
	double totalWarpDistance = 0.0;
};

int Fail( const std::string& message )
{
	std::fprintf( stderr, "DestinyEmbeddedWarpContractTest: %s\n", message.c_str() );
	return 1;
}

bool Near( double a, double b, double epsilon )
{
	return std::abs( a - b ) <= epsilon;
}

// Position magnitudes reach 1.4e12 m, where the double ULP is 2.4e-4 m;
// pure absolute tolerances in the PL-11A style are meaningless there. The
// corpus gate is therefore mixed absolute/relative.
bool NearMixed( double a, double b, double absolute, double relative )
{
	return std::abs( a - b ) <= std::max( absolute, relative * std::abs( b ) );
}

// Verbatim replication of Ballpark::SetupWarpConstants (Ballpark.cpp:4185).
void SetupWarpConstants(
	double warpFactor,
	double warpDistance,
	double& accelDuration,
	double& cruiseDuration,
	double& decelDuration,
	double& accelDistance,
	double& cruiseDistance,
	double& decelDistance,
	double& accelRate,
	double& decelRate,
	double& warpSpeed )
{
	warpSpeed = warpFactor * kWarpFactorToAuPerSecond * kAu;
	accelRate = warpFactor * kWarpFactorToAcceleration;
	decelRate = std::min( warpFactor * kWarpFactorToDeceleration, 2.0 );
	warpSpeed = std::min( warpSpeed, ( warpDistance + 1 ) * accelRate * decelRate / ( accelRate + decelRate ) );
	accelDuration = std::log( warpSpeed / accelRate ) / accelRate;
	cruiseDuration = ( warpDistance / warpSpeed ) - ( 1.0 / accelRate ) - ( 1.0 / decelRate ) + ( 1.0 / warpSpeed );
	decelDuration = std::log( warpSpeed / decelRate ) / decelRate;
	accelDistance = warpSpeed / accelRate;
	cruiseDistance = warpSpeed * cruiseDuration;
	decelDistance = warpSpeed / decelRate - 1;
}

// Verbatim replication of the warp-phase kinematics of
// Ballpark::WarpDistance (Ballpark.cpp:4278): given the previous-tick
// position/velocity and the phase time, produce this tick's expected
// position and velocity. Returns false when the sim would drop out of warp
// at this tick instead of producing a warp sample.
bool ExpectedWarpStep(
	const double previousPosition[3],
	const double previousVelocity[3],
	const double gotoPoint[3],
	double warpDistance,
	double t,
	double expectedPosition[3],
	double expectedVelocity[3] )
{
	double accelDuration, cruiseDuration, decelDuration;
	double accelDistance, cruiseDistance, decelDistance;
	double accelRate, decelRate, warpSpeed;
	SetupWarpConstants(
		static_cast<double>( kWarpFactor ), warpDistance, accelDuration, cruiseDuration, decelDuration,
		accelDistance, cruiseDistance, decelDistance, accelRate, decelRate, warpSpeed );

	double dir[3] = {
		gotoPoint[0] - previousPosition[0],
		gotoPoint[1] - previousPosition[1],
		gotoPoint[2] - previousPosition[2],
	};
	const double dirLength = std::sqrt( dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2] );
	for( double& component : dir )
		component /= dirLength;

	double speed = 0.0;
	double distance = 0.0;
	if( t < accelDuration )
	{
		speed = accelRate * std::exp( accelRate * t );
		const double currentSpeed = std::sqrt(
			previousVelocity[0] * previousVelocity[0] + previousVelocity[1] * previousVelocity[1] +
			previousVelocity[2] * previousVelocity[2] );
		if( currentSpeed > speed )
			speed = currentSpeed;
		distance = std::exp( accelRate * t );
	}
	else if( ( t - accelDuration ) < cruiseDuration )
	{
		speed = warpSpeed;
		distance = warpSpeed * ( t - accelDuration ) + accelDistance;
	}
	else
	{
		speed = warpSpeed * std::exp( -decelRate * ( t - cruiseDuration - accelDuration ) );
		distance = warpSpeed / decelRate -
			( warpSpeed / decelRate ) * std::exp( -decelRate * ( t - cruiseDuration - accelDuration ) ) +
			accelDistance + cruiseDistance;
		if( speed < std::min( kMaximumVelocity / 2.0, 100.0 ) )
			return false; // dropout tick: mode flips to STOP and drag integrates
	}
	for( size_t axis = 0; axis < 3; ++axis )
	{
		expectedVelocity[axis] = speed * dir[axis];
		expectedPosition[axis] = gotoPoint[axis] -
			( accelDistance + cruiseDistance + decelDistance - distance ) * dir[axis];
	}
	return true;
}

bool LoadWarpCorpus( std::vector<ReferenceSample>& samples )
{
	std::ifstream input( DESTINY_MOTION_CORPUS_DIR "/pl11c-warp.csv" );
	std::string line;
	if( !input || !std::getline( input, line ) )
		return false;
	while( std::getline( input, line ) )
	{
		std::replace( line.begin(), line.end(), ',', ' ' );
		std::istringstream row( line );
		uint32_t tick = 0;
		ReferenceSample sample = {};
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
	return !samples.empty();
}

bool AddNewEdenCelestials( DestinyEmbeddedSession* session, std::string& error )
{
	// The accepted PL-12/PL-12B celestial set: warp must not perturb the
	// trajectory when these balls are present, and vice versa.
	const struct
	{
		int64_t ballId;
		float radius;
		double position[3];
	} celestials[] = {
		{ 40334263, 158400000.0f, { 1069486940160.0, -202669301760.0, -831868968960.0 } },
		{ 40334264, 2630000.0f, { 1083758787326.0, -205372890997.0, -787280443197.0 } },
		{ 900001, 1.0f, { 1096057063200.0, -201867615160.0, -832210688752.0 } },
	};
	for( const auto& celestial : celestials )
	{
		DestinyEmbeddedCelestialConfig config = {};
		config.ballId = celestial.ballId;
		config.radius = celestial.radius;
		for( size_t axis = 0; axis < 3; ++axis )
			config.position[axis] = celestial.position[axis];
		char celestialError[512] = {};
		if( !Destiny_AddEmbeddedCelestial( session, &config, celestialError, sizeof( celestialError ) ) )
		{
			error = std::string( "celestial setup failed: " ) + celestialError;
			return false;
		}
	}
	return true;
}

bool RunScenario(
	DestinyEmbeddedReferenceFrame referenceFrame,
	bool withCelestials,
	const std::vector<ReferenceSample>* expected,
	ScenarioResult& result,
	std::string& error )
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
	options.referenceFrame = referenceFrame;
	options.observerBallId = 2;

	char createError[512] = {};
	DestinyEmbeddedSession* session = Destiny_CreateEmbeddedSessionWithOptions(
		&config, &options, createError, sizeof( createError ) );
	if( !session )
	{
		error = createError;
		return false;
	}

	if( withCelestials && !AddNewEdenCelestials( session, error ) )
	{
		Destiny_DestroyEmbeddedSession( session );
		return false;
	}

	// Precondition contract: wrong tick, degenerate factor, and the
	// sub-200 km downgrade zone must all be rejected outright.
	const double nearTarget[3] = { 0.0, 0.0, 50000.0 };
	if( Destiny_CommandEmbeddedWarp( session, 2 * kTicksPerSecond, kWarpTarget, kMinimumRange, kWarpFactor ) ||
		Destiny_CommandEmbeddedWarp( session, kTicksPerSecond, kWarpTarget, kMinimumRange, 0 ) ||
		Destiny_CommandEmbeddedWarp( session, kTicksPerSecond, nearTarget, 0.0, kWarpFactor ) ||
		Destiny_CommandEmbeddedWarp( session, kTicksPerSecond, kWarpTarget, -1.0, kWarpFactor ) )
	{
		error = "an invalid warp command was accepted";
		Destiny_DestroyEmbeddedSession( session );
		return false;
	}

	uint64_t previousEvolveCount = 0;
	bool sawWarpMode = false;
	bool massiveDuringWarp = false;
	bool sensorDuringWarp = false;
	double gotoPoint[3] = {};
	double previousPosition[3] = {};
	double previousVelocity[3] = {};
	bool havePrevious = false;

	for( uint64_t frame = 0; frame < kFrameCount; ++frame )
	{
		const Be::Time simulationTime = static_cast<Be::Time>( frame * kTicksPerSecond / 60 );
		if( frame == kCommandFrame &&
			!Destiny_CommandEmbeddedWarp( session, simulationTime, kWarpTarget, kMinimumRange, kWarpFactor ) )
		{
			error = "frame-180 WARP command failed";
			Destiny_DestroyEmbeddedSession( session );
			return false;
		}
		if( frame == kCommandFrame )
		{
			DestinyEmbeddedDiagnostics queued = {};
			if( !Destiny_GetEmbeddedDiagnostics( session, &queued ) || queued.mode != DSTBALL_STOP )
			{
				error = "WARP command changed mode before its effective tick";
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

		if( diagnostics.directEvolveCount == previousEvolveCount )
			continue;
		previousEvolveCount = diagnostics.directEvolveCount;
		const uint64_t evolve = previousEvolveCount;

		if( std::getenv( "DESTINY_WARP_DEBUG" ) && evolve <= 20 )
		{
			std::fprintf(
				stderr,
				"evolve=%llu mode=%d stamp=%lld dist=%.17g p=(%.17g, %.17g, %.17g) |v|=%.17g\n",
				static_cast<unsigned long long>( evolve ), diagnostics.mode,
				static_cast<long long>( diagnostics.warpEffectStamp ), diagnostics.warpTotalDistance,
				diagnostics.rawPosition[0], diagnostics.rawPosition[1], diagnostics.rawPosition[2],
				std::sqrt(
					diagnostics.rawVelocity[0] * diagnostics.rawVelocity[0] +
					diagnostics.rawVelocity[1] * diagnostics.rawVelocity[1] +
					diagnostics.rawVelocity[2] * diagnostics.rawVelocity[2] ) );
		}

		if( evolve <= 2 )
		{
			if( !Near( diagnostics.rawPosition[2], -1000.0, 1e-10 ) ||
				!Near( diagnostics.rawVelocity[2], 0.0, 1e-10 ) )
			{
				error = "STOP hold moved before the command";
				Destiny_DestroyEmbeddedSession( session );
				return false;
			}
		}

		if( diagnostics.mode == DSTBALL_WARP )
		{
			sawWarpMode = true;
			massiveDuringWarp = massiveDuringWarp ||
				( diagnostics.warpEffectStamp >= 0 && diagnostics.isMassive );
			sensorDuringWarp = sensorDuringWarp ||
				( diagnostics.warpEffectStamp >= 0 && diagnostics.sensorActive );
			if( diagnostics.warpFactor != kWarpFactor ||
				!Near( diagnostics.warpMinRange, kMinimumRange, 0.0 ) )
			{
				error = "warp factor or minimum range did not echo the command";
				Destiny_DestroyEmbeddedSession( session );
				return false;
			}
			if( diagnostics.warpEffectStamp < 0 )
			{
				++result.aligningTicks;
			}
			else
			{
				if( result.activationEvolve < 0 )
				{
					result.activationEvolve = static_cast<int64_t>( evolve );
					result.totalWarpDistance = diagnostics.warpTotalDistance;
					for( size_t axis = 0; axis < 3; ++axis )
						gotoPoint[axis] = diagnostics.gotoPoint[axis];
				}
				// Per-step closed-form replication from the previous sample:
				// same equations, same operation order, so agreement is a few
				// ULP even at 1.4e12 m.
				if( havePrevious )
				{
					const double t =
						static_cast<double>( static_cast<int64_t>( evolve ) - 1 - diagnostics.warpEffectStamp );
					double expectedPosition[3];
					double expectedVelocity[3];
					if( t >= 0.0 &&
						ExpectedWarpStep(
							previousPosition, previousVelocity, gotoPoint,
							result.totalWarpDistance, t, expectedPosition, expectedVelocity ) )
					{
						// Warp positions are differences of ~1.4e12 m
						// quantities (p = goto - remaining*dir), so every
						// result carries the ~2.4e-4 m double ULP of that
						// scale regardless of its own magnitude; the
						// replication therefore agrees to a few ULP of the
						// LEG scale, not of the sample. The 5e-3 floor is
						// ~20 ULP at 1.4e12; any phase or equation error
						// diverges by meters to AU and still fails loudly.
						for( size_t axis = 0; axis < 3; ++axis )
						{
							if( !NearMixed( diagnostics.rawPosition[axis], expectedPosition[axis], 5e-3, 1e-9 ) ||
								!NearMixed( diagnostics.rawVelocity[axis], expectedVelocity[axis], 5e-3, 1e-9 ) )
							{
								char detail[512] = {};
								std::snprintf(
									detail, sizeof( detail ),
									"warp closed form diverged at evolve %llu axis %zu: %.17g vs %.17g",
									static_cast<unsigned long long>( evolve ), axis,
									diagnostics.rawPosition[axis], expectedPosition[axis] );
								error = detail;
								Destiny_DestroyEmbeddedSession( session );
								return false;
							}
						}
					}
				}
			}
		}
		else if( sawWarpMode && result.dropoutEvolve < 0 && diagnostics.mode == DSTBALL_STOP )
		{
			result.dropoutEvolve = static_cast<int64_t>( evolve );
			if( !diagnostics.isMassive || !diagnostics.sensorActive )
			{
				error = "dropout did not restore collision and sensor participation";
				Destiny_DestroyEmbeddedSession( session );
				return false;
			}
		}

		for( size_t axis = 0; axis < 3; ++axis )
		{
			previousPosition[axis] = diagnostics.rawPosition[axis];
			previousVelocity[axis] = diagnostics.rawVelocity[axis];
		}
		havePrevious = true;

		if( evolve >= 3 )
		{
			ReferenceSample sample = {};
			for( size_t axis = 0; axis < 3; ++axis )
			{
				sample.position[axis] = diagnostics.rawPosition[axis];
				sample.velocity[axis] = diagnostics.rawVelocity[axis];
				sample.acceleration[axis] = diagnostics.rawAcceleration[axis];
			}
			const size_t motionIndex = static_cast<size_t>( evolve - 3 );
			if( expected && motionIndex < expected->size() )
			{
				for( size_t axis = 0; axis < 3; ++axis )
				{
					const ReferenceSample& reference = ( *expected )[motionIndex];
					if( !NearMixed( sample.position[axis], reference.position[axis], 1e-5, 1e-12 ) ||
						!NearMixed( sample.velocity[axis], reference.velocity[axis], 1e-5, 1e-12 ) ||
						!NearMixed( sample.acceleration[axis], reference.acceleration[axis], 1e-5, 1e-12 ) )
					{
						char detail[512] = {};
						std::snprintf(
							detail, sizeof( detail ),
							"warp corpus diverged at tick %zu axis %zu: %.17g vs %.17g",
							motionIndex + 1, axis, sample.position[axis],
							reference.position[axis] );
						error = detail;
						Destiny_DestroyEmbeddedSession( session );
						return false;
					}
				}
			}
			result.rawMotion.push_back( sample );
		}
	}

	DestinyEmbeddedDiagnostics finalDiagnostics = {};
	const bool diagnosticsOk = Destiny_GetEmbeddedDiagnostics( session, &finalDiagnostics );
	Destiny_DestroyEmbeddedSession( session );
	if( !diagnosticsOk || finalDiagnostics.directEvolveCount != 44 || finalDiagnostics.commandCount != 1 ||
		finalDiagnostics.lastCommandTime != 3 * kTicksPerSecond || finalDiagnostics.mode != DSTBALL_STOP )
	{
		error = "final warp counters failed";
		return false;
	}
	if( !sawWarpMode || result.activationEvolve < 0 || result.dropoutEvolve < 0 ||
		result.aligningTicks == 0 || massiveDuringWarp || sensorDuringWarp )
	{
		error = "warp phase structure failed (align/activation/dropout or collision suspension)";
		return false;
	}
	const double expectedLegLength = std::sqrt(
		kWarpTarget[0] * kWarpTarget[0] + kWarpTarget[1] * kWarpTarget[1] +
		( kWarpTarget[2] + 1000.0 ) * ( kWarpTarget[2] + 1000.0 ) );
	if( !NearMixed( result.totalWarpDistance, expectedLegLength - kMinimumRange, 1e6, 1e-3 ) )
	{
		error = "total warp distance is inconsistent with the commanded leg";
		return false;
	}
	return true;
}

bool WriteCorpus( const ScenarioResult& result )
{
	std::ofstream output( DESTINY_MOTION_CORPUS_DIR "/pl11c-warp.csv" );
	if( !output )
		return false;
	output << "tick,px,py,pz,vx,vy,vz,ax,ay,az\n";
	char row[512];
	for( size_t i = 0; i < result.rawMotion.size(); ++i )
	{
		const ReferenceSample& sample = result.rawMotion[i];
		std::snprintf(
			row, sizeof( row ), "%zu,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g\n",
			i + 1,
			sample.position[0], sample.position[1], sample.position[2],
			sample.velocity[0], sample.velocity[1], sample.velocity[2],
			sample.acceleration[0], sample.acceleration[1], sample.acceleration[2] );
		output << row;
	}
	return output.good();
}
}

int main( int argc, char** argv )
{
	if( !Py_IsInitialized() )
		Py_Initialize();
	BlueModuleStartup();
	if( !BeClasses )
		return Fail( "Blue startup did not initialize BeClasses" );

	DestinyEmbeddedRegistration registration = {};
	if( !Destiny_RegisterBlueClasses( &registration ) || registration.discoveredClassCount != 10 )
		return Fail( "embedded registration failed" );

	const bool record = argc > 1 && std::strcmp( argv[1], "--record" ) == 0;

	std::vector<ReferenceSample> expected;
	if( !record )
	{
		if( !LoadWarpCorpus( expected ) ||
			!std::ifstream( DESTINY_MOTION_CORPUS_DIR "/pl11c-provenance.json" ) )
		{
			return Fail( "warp reference corpus is missing or invalid" );
		}
	}

	ScenarioResult primaryA, primaryB, observer, celestial;
	std::string error;
	const std::vector<ReferenceSample>* reference = record ? nullptr : &expected;
	if( !RunScenario( DESTINY_EMBEDDED_PRIMARY_EGO, false, reference, primaryA, error ) )
		return Fail( error );

	if( record )
	{
		if( !WriteCorpus( primaryA ) )
			return Fail( "corpus write failed" );
		std::printf(
			"Destiny PL-11C warp corpus recorded: samples=%zu align=%llu activation=%lld dropout=%lld distance=%.6f AU\n",
			primaryA.rawMotion.size(),
			static_cast<unsigned long long>( primaryA.aligningTicks ),
			static_cast<long long>( primaryA.activationEvolve ),
			static_cast<long long>( primaryA.dropoutEvolve ),
			primaryA.totalWarpDistance / kAu );
		return 0;
	}

	if( primaryA.rawMotion.size() != expected.size() )
		return Fail( "live run and corpus have different lengths" );

	if( !RunScenario( DESTINY_EMBEDDED_PRIMARY_EGO, false, reference, primaryB, error ) ||
		!RunScenario( DESTINY_EMBEDDED_FIXED_OBSERVER, false, reference, observer, error ) ||
		!RunScenario( DESTINY_EMBEDDED_PRIMARY_EGO, true, reference, celestial, error ) )
	{
		return Fail( error );
	}

	if( primaryA.rawMotion.size() != primaryB.rawMotion.size() ||
		primaryA.rawMotion.size() != observer.rawMotion.size() ||
		primaryA.rawMotion.size() != celestial.rawMotion.size() )
	{
		return Fail( "warp trajectories have different lengths" );
	}
	for( size_t i = 0; i < primaryA.rawMotion.size(); ++i )
	{
		const auto& a = primaryA.rawMotion[i];
		const auto& b = primaryB.rawMotion[i];
		const auto& c = observer.rawMotion[i];
		const auto& d = celestial.rawMotion[i];
		if( a.position != b.position || a.velocity != b.velocity || a.acceleration != b.acceleration ||
			a.position != c.position || a.velocity != c.velocity || a.acceleration != c.acceleration )
		{
			return Fail( "warp trajectory is not deterministic across runs and reference frames" );
		}
		if( a.position != d.position || a.velocity != d.velocity || a.acceleration != d.acceleration )
		{
			return Fail( "celestial balls perturbed the warp trajectory" );
		}
	}

	std::printf(
		"Destiny PL-11C warp contract: samples=%zu align=%llu activation=%lld dropout=%lld "
		"distance=%.6f AU frames=primary+observer+celestial\n",
		primaryA.rawMotion.size(),
		static_cast<unsigned long long>( primaryA.aligningTicks ),
		static_cast<long long>( primaryA.activationEvolve ),
		static_cast<long long>( primaryA.dropoutEvolve ),
		primaryA.totalWarpDistance / kAu );
	return 0;
}
