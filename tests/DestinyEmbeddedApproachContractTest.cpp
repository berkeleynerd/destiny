// Copyright (c) 2026 CCP Games

#include "StdAfx.h"

#include "DestinyEmbedded.h"
#include "DstConstants.h"

#include <Blue.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

const char* g_moduleName = "DestinyEmbeddedApproachContractTest";

BLUE_DEFINE_INTERFACE( ITriVectorFunction );
BLUE_DEFINE_INTERFACE( ITriQuaternionFunction );
BLUE_DEFINE_INTERFACE( IEveReferencePoint );
BLUE_DEFINE_INTERFACE( IEveBallpark );

namespace
{
constexpr Be::Time kTicksPerSecond = 10000000;
constexpr uint64_t kFrameCount = 3780;
constexpr uint64_t kCommandFrame = 180;
constexpr int64_t kTargetBallId = 3;
constexpr float kTargetRadius = 35.0f;
constexpr float kShipRadius = 35.0f;
constexpr float kFollowRange = 500.0f;
constexpr size_t kCorpusSamples = 60;

// The recovered follow solver constants (Ballpark::EvolveFollow +
// Ballpark::GotoThrust + the friction integrator): center-distance setpoint
// is the surface range plus BOTH radii; thrust is the GOTO solver with a
// per-tick recomputed destination and quartic homing falloff inside one
// tick's travel; the mode never leaves DSTBALL_FOLLOW (station-keeping).
constexpr double kFriction = 1000000.0;
constexpr double kDt = 1.0;
const double kEffectiveMass = 975000.0 * static_cast<double>( 2.87f );
const double kMaxVelocity = static_cast<double>( 312.0f );
constexpr double kSpeedFraction = 1.0;
const double kCenterSetpoint = static_cast<double>( kFollowRange ) +
	static_cast<double>( kShipRadius ) + static_cast<double>( kTargetRadius );

struct ReferenceSample
{
	std::array<double, 3> position;
	std::array<double, 3> velocity;
	std::array<double, 3> acceleration;

	bool operator==( const ReferenceSample& other ) const
	{
		return position == other.position && velocity == other.velocity &&
			acceleration == other.acceleration;
	}
};

struct ScenarioResult
{
	std::vector<ReferenceSample> rawMotion;
	double minimumCenterDistance = 1e30;
	double finalCenterDistance = 0.0;
	double finalSpeed = 0.0;
	int32_t finalMode = -1;
	int64_t finalFollowBallId = 0;
	float finalFollowRange = -1.0f;
};

int Fail( const std::string& message )
{
	std::fprintf( stderr, "DestinyEmbeddedApproachContractTest: %s\n", message.c_str() );
	return 1;
}

bool Near( double a, double b, double epsilon )
{
	return std::abs( a - b ) <= epsilon;
}

bool NearMixed( double a, double b, double absolute, double relative )
{
	return std::abs( a - b ) <= std::max( absolute, relative * std::max( std::abs( a ), std::abs( b ) ) );
}

// Ballpark::EvolveFollow replicated verbatim for a fixed target at the
// origin: goto point at kCenterSetpoint from the target center along the
// current separation direction, then GotoThrust with the missile flag false.
std::array<double, 3> ExpectedFollowAcceleration( const std::array<double, 3>& position )
{
	const double dist = std::sqrt(
		position[0] * position[0] + position[1] * position[1] + position[2] * position[2] );
	std::array<double, 3> target;
	if( dist == 0.0 )
	{
		target = { kCenterSetpoint, 0.0, 0.0 };
	}
	else
	{
		for( size_t axis = 0; axis < 3; ++axis )
			target[axis] = position[axis] * kCenterSetpoint / dist;
	}
	std::array<double, 3> thrust;
	for( size_t axis = 0; axis < 3; ++axis )
		thrust[axis] = target[axis] - position[axis];
	const double lengthSquared =
		thrust[0] * thrust[0] + thrust[1] * thrust[1] + thrust[2] * thrust[2];
	const double homingDistance = kSpeedFraction * kMaxVelocity * kDt;
	const double maxThrust =
		kFriction * kSpeedFraction * kMaxVelocity / ( 975000.0 * static_cast<double>( 2.87f ) );
	const double length = std::sqrt( lengthSquared );
	if( length == 0.0 )
		return { 0.0, 0.0, 0.0 };
	double scale = maxThrust / length;
	if( lengthSquared < homingDistance * homingDistance )
	{
		const double coefficient = lengthSquared / ( homingDistance * homingDistance );
		scale *= coefficient * coefficient;
	}
	// The sim stores the thrust in the ball's float acceleration vector
	// (mLastG) and the integrator consumes the stored value, so every
	// component carries one float round-trip. Verified at recording: the
	// corpus acceleration is bit-exactly float(double thrust).
	return {
		static_cast<double>( static_cast<float>( thrust[0] * scale ) ),
		static_cast<double>( static_cast<float>( thrust[1] * scale ) ),
		static_cast<double>( static_cast<float>( thrust[2] * scale ) ),
	};
}

// The friction integrator closed form accepted under PL-11A: advance one
// tick of position and velocity under constant acceleration with linear
// drag (terminal velocity = speedFraction * maxVel).
void IntegrateStep( std::array<double, 3>& position, std::array<double, 3>& velocity,
	const std::array<double, 3>& acceleration )
{
	const double timeFactor = std::exp( -kFriction / kEffectiveMass );
	const double frictionSquared = kFriction * kFriction;
	for( size_t axis = 0; axis < 3; ++axis )
	{
		const double a = acceleration[axis];
		position[axis] = ( kEffectiveMass *
							 ( kEffectiveMass * a * ( timeFactor - 1.0 ) +
							   kFriction * ( a + velocity[axis] - timeFactor * velocity[axis] ) ) +
						 position[axis] * frictionSquared ) /
			frictionSquared;
		velocity[axis] =
			( kEffectiveMass * a - ( kEffectiveMass * a - velocity[axis] * kFriction ) * timeFactor ) /
			kFriction;
	}
}

bool LoadCorpus( std::vector<ReferenceSample>& samples )
{
	std::ifstream input( DESTINY_MOTION_CORPUS_DIR "/pl11d-approach.csv" );
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
	return samples.size() == kCorpusSamples;
}

// Closed-form corpus validation on the PL-11A precedent: integrate every
// RECORDED acceleration through the friction integrator and require the
// recorded position/velocity to follow bit-tight. The thrust itself is
// orientation-coupled (EvolveBehaviorForBall rewrites the solver output as
// heading * cos^2(theta) * |a| and zeroes it when the desired thrust points
// behind the nose), so full thrust re-derivation would replicate the
// quaternion torque solver; instead the orientation contract is gated as an
// envelope: |a| never exceeds the float-quantized maximum thrust, the
// pre-crossing full-thrust phase matches the unconstrained solver exactly,
// and coast ticks (a == 0) only occur after the range crossing.
bool ValidateClosedFormCorpus( const std::vector<ReferenceSample>& samples, std::string& error )
{
	const double maxThrust = static_cast<double>( static_cast<float>(
		kFriction * kSpeedFraction * kMaxVelocity / ( 975000.0 * static_cast<double>( 2.87f ) ) ) );
	std::array<double, 3> position = { 0.0, 0.0, -2570.0 };
	std::array<double, 3> velocity = { 0.0, 0.0, 0.0 };
	bool crossedSetpoint = false;
	for( size_t i = 0; i < samples.size(); ++i )
	{
		if( i > 0 )
		{
			position = samples[i - 1].position;
			velocity = samples[i - 1].velocity;
		}
		const std::array<double, 3> previousPosition = position;
		const double previousDistance = std::sqrt(
			previousPosition[0] * previousPosition[0] + previousPosition[1] * previousPosition[1] +
			previousPosition[2] * previousPosition[2] );
		crossedSetpoint = crossedSetpoint || previousDistance < kCenterSetpoint;

		const double recordedMagnitude = std::sqrt(
			samples[i].acceleration[0] * samples[i].acceleration[0] +
			samples[i].acceleration[1] * samples[i].acceleration[1] +
			samples[i].acceleration[2] * samples[i].acceleration[2] );
		if( recordedMagnitude > maxThrust * ( 1.0 + 1e-9 ) )
		{
			char detail[256] = {};
			std::snprintf( detail, sizeof( detail ),
				"recorded thrust exceeds the solver maximum at tick %zu: %.17g > %.17g", i + 1,
				recordedMagnitude, maxThrust );
			error = detail;
			return false;
		}
		if( recordedMagnitude == 0.0 && !crossedSetpoint && i > 0 )
		{
			char detail[256] = {};
			std::snprintf( detail, sizeof( detail ),
				"coast tick %zu before the range crossing violates the orientation contract", i + 1 );
			error = detail;
			return false;
		}
		if( !crossedSetpoint )
		{
			// Straight-line full-thrust phase: the heading is aligned, so the
			// unconstrained solver must match the recording exactly.
			const std::array<double, 3> derived = ExpectedFollowAcceleration( previousPosition );
			for( size_t axis = 0; axis < 3; ++axis )
			{
				if( !NearMixed( derived[axis], samples[i].acceleration[axis], 1e-8, 1e-10 ) )
				{
					char detail[512] = {};
					std::snprintf( detail, sizeof( detail ),
						"pre-crossing thrust divergence at tick %zu axis %zu: %.17g/%.17g", i + 1,
						axis, derived[axis], samples[i].acceleration[axis] );
					error = detail;
					return false;
				}
			}
		}
		IntegrateStep( position, velocity, samples[i].acceleration );
		for( size_t axis = 0; axis < 3; ++axis )
		{
			if( !NearMixed( position[axis], samples[i].position[axis], 1e-8, 1e-10 ) ||
				!NearMixed( velocity[axis], samples[i].velocity[axis], 1e-8, 1e-10 ) )
			{
				char detail[512] = {};
				std::snprintf( detail, sizeof( detail ),
					"integrator divergence at tick %zu axis %zu: p=%.17g/%.17g v=%.17g/%.17g",
					i + 1, axis, position[axis], samples[i].position[axis], velocity[axis],
					samples[i].velocity[axis] );
				error = detail;
				return false;
			}
		}
	}
	return true;
}

bool AddNewEdenCelestials( DestinyEmbeddedSession* session, std::string& error )
{
	// The accepted PL-12/PL-12B celestial set: approach must not perturb the
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
	config.radius = kShipRadius;
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

	DestinyEmbeddedFixedTargetConfig target = {};
	target.ballId = kTargetBallId;
	target.radius = kTargetRadius;
	if( !Destiny_AddEmbeddedFixedTarget( session, &target, createError, sizeof( createError ) ) ||
		Destiny_AddEmbeddedFixedTarget( session, &target, createError, sizeof( createError ) ) )
	{
		error = "fixed-target contract failed";
		Destiny_DestroyEmbeddedSession( session );
		return false;
	}

	const bool debug = std::getenv( "DESTINY_APPROACH_DEBUG" ) != nullptr;
	uint64_t previousEvolves = 0;
	for( uint64_t frame = 0; frame < kFrameCount; ++frame )
	{
		const Be::Time simulationTime = static_cast<Be::Time>( frame * kTicksPerSecond / 60 );
		if( frame == kCommandFrame )
		{
			// Precondition negatives: wrong target, negative range, NaN
			// range, own/observer target — all rejected without consuming
			// the command slot; then the real command; then a same-tick
			// second command is rejected.
			if( Destiny_CommandEmbeddedFollow( session, simulationTime, 4, kFollowRange ) ||
				Destiny_CommandEmbeddedFollow( session, simulationTime, kTargetBallId, -1.0f ) ||
				Destiny_CommandEmbeddedFollow(
					session, simulationTime, kTargetBallId, std::nanf( "" ) ) ||
				Destiny_CommandEmbeddedFollow( session, simulationTime, 1, kFollowRange ) ||
				!Destiny_CommandEmbeddedFollow(
					session, simulationTime, kTargetBallId, kFollowRange ) ||
				Destiny_CommandEmbeddedStop( session, simulationTime ) )
			{
				error = "tick-aligned FOLLOW command contract failed";
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
		if( diagnostics.orientationPinCount != 0 || diagnostics.schedulerRegistered ||
			diagnostics.startCallCount || diagnostics.onTickCallCount ||
			diagnostics.pythonCallbackCount )
		{
			error = "embedded runtime contract failed";
			Destiny_DestroyEmbeddedSession( session );
			return false;
		}
		if( diagnostics.directEvolveCount != previousEvolves )
		{
			previousEvolves = diagnostics.directEvolveCount;
			const double centerDistance = std::sqrt(
				diagnostics.rawPosition[0] * diagnostics.rawPosition[0] +
				diagnostics.rawPosition[1] * diagnostics.rawPosition[1] +
				diagnostics.rawPosition[2] * diagnostics.rawPosition[2] );
			const double speed = std::sqrt(
				diagnostics.rawVelocity[0] * diagnostics.rawVelocity[0] +
				diagnostics.rawVelocity[1] * diagnostics.rawVelocity[1] +
				diagnostics.rawVelocity[2] * diagnostics.rawVelocity[2] );
			if( debug )
			{
				std::fprintf( stderr,
					"evolve=%llu mode=%d center=%.9f speed=%.9f\n",
					static_cast<unsigned long long>( previousEvolves ), diagnostics.mode,
					centerDistance, speed );
			}
			if( previousEvolves <= 2 )
			{
				if( !Near( diagnostics.rawPosition[2], -2570.0, 1e-10 ) ||
					diagnostics.mode != DSTBALL_STOP )
				{
					error = "STOP state moved before FOLLOW";
					Destiny_DestroyEmbeddedSession( session );
					return false;
				}
			}
			else
			{
				if( diagnostics.mode != DSTBALL_FOLLOW || diagnostics.followBallId != kTargetBallId ||
					!Near( diagnostics.followRange, kFollowRange, 1e-6 ) )
				{
					error = "FOLLOW state contract failed";
					Destiny_DestroyEmbeddedSession( session );
					return false;
				}
				result.minimumCenterDistance = std::min( result.minimumCenterDistance, centerDistance );
				const size_t index = static_cast<size_t>( previousEvolves - 3 );
				if( index >= kCorpusSamples )
				{
					error = "too many approach samples";
					Destiny_DestroyEmbeddedSession( session );
					return false;
				}
				ReferenceSample sample = {};
				for( size_t axis = 0; axis < 3; ++axis )
				{
					sample.position[axis] = diagnostics.rawPosition[axis];
					sample.velocity[axis] = diagnostics.rawVelocity[axis];
					sample.acceleration[axis] = diagnostics.rawAcceleration[axis];
				}
				if( expected )
				{
					for( size_t axis = 0; axis < 3; ++axis )
					{
						if( !Near( sample.position[axis], ( *expected )[index].position[axis], 1e-5 ) ||
							!Near( sample.velocity[axis], ( *expected )[index].velocity[axis], 1e-5 ) ||
							!Near( sample.acceleration[axis], ( *expected )[index].acceleration[axis],
								1e-5 ) )
						{
							char detail[512] = {};
							std::snprintf( detail, sizeof( detail ),
								"approach tick %zu axis %zu diverged: p=%.12g/%.12g v=%.12g/%.12g a=%.12g/%.12g",
								index + 1, axis, sample.position[axis],
								( *expected )[index].position[axis], sample.velocity[axis],
								( *expected )[index].velocity[axis], sample.acceleration[axis],
								( *expected )[index].acceleration[axis] );
							error = detail;
							Destiny_DestroyEmbeddedSession( session );
							return false;
						}
					}
				}
				result.rawMotion.push_back( sample );
			}
		}
	}

	DestinyEmbeddedDiagnostics final = {};
	const bool finalOk = Destiny_GetEmbeddedDiagnostics( session, &final );
	Destiny_DestroyEmbeddedSession( session );
	if( !finalOk )
	{
		error = "final diagnostic query failed";
		return false;
	}
	result.finalMode = final.mode;
	result.finalFollowBallId = final.followBallId;
	result.finalFollowRange = final.followRange;
	result.finalCenterDistance = std::sqrt(
		final.rawPosition[0] * final.rawPosition[0] + final.rawPosition[1] * final.rawPosition[1] +
		final.rawPosition[2] * final.rawPosition[2] );
	result.finalSpeed = std::sqrt(
		final.rawVelocity[0] * final.rawVelocity[0] + final.rawVelocity[1] * final.rawVelocity[1] +
		final.rawVelocity[2] * final.rawVelocity[2] );
	if( final.directEvolveCount != 62 || final.commandCount != 1 ||
		final.mode != DSTBALL_FOLLOW || final.followBallId != kTargetBallId ||
		!Near( final.followRange, kFollowRange, 1e-6 ) )
	{
		error = "final FOLLOW state failed";
		return false;
	}
	// Station-keeping contract: the ball never transitions to STOP; it damps
	// asymptotically toward the center setpoint with a small residual gap
	// (thrust falls off quartically inside one tick's travel). The bounds
	// are fixture facts measured at recording, generous enough for the
	// closed-form floor and tight enough to catch a solver regression.
	// Measured fixture facts at recording: deepest overshoot carries the
	// center to ~37.36 m (the target is non-massive; no collision), and the
	// ball settles back from inside to ~568.3 m — just under the setpoint.
	if( result.minimumCenterDistance < 30.0 || result.minimumCenterDistance > 50.0 ||
		result.finalCenterDistance < kCenterSetpoint - 10.0 ||
		result.finalCenterDistance > kCenterSetpoint + 10.0 || result.finalSpeed > 0.01 )
	{
		char detail[512] = {};
		std::snprintf( detail, sizeof( detail ),
			"station-keeping contract failed: min=%.6f final=%.6f speed=%.9f",
			result.minimumCenterDistance, result.finalCenterDistance, result.finalSpeed );
		error = detail;
		return false;
	}
	return true;
}

bool WriteCorpus( const ScenarioResult& result )
{
	std::ofstream output( DESTINY_MOTION_CORPUS_DIR "/pl11d-approach.csv" );
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
	std::string error;
	if( !record )
	{
		if( !LoadCorpus( expected ) ||
			!std::ifstream( DESTINY_MOTION_CORPUS_DIR "/pl11d-provenance.json" ) )
		{
			return Fail( "approach reference corpus is missing or invalid" );
		}
		if( !ValidateClosedFormCorpus( expected, error ) )
			return Fail( error );
	}

	ScenarioResult primaryA, primaryB, observer, celestial;
	const std::vector<ReferenceSample>* reference = record ? nullptr : &expected;
	if( !RunScenario( DESTINY_EMBEDDED_PRIMARY_EGO, false, reference, primaryA, error ) )
		return Fail( error );

	if( record )
	{
		if( !WriteCorpus( primaryA ) )
			return Fail( "corpus write failed" );
		std::printf(
			"Destiny PL-11D approach corpus recorded: samples=%zu min-center=%.6f final-center=%.6f final-speed=%.9f\n",
			primaryA.rawMotion.size(), primaryA.minimumCenterDistance, primaryA.finalCenterDistance,
			primaryA.finalSpeed );
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
		return Fail( "approach trajectories have different lengths" );
	}
	for( size_t i = 0; i < primaryA.rawMotion.size(); ++i )
	{
		const auto& a = primaryA.rawMotion[i];
		const auto& b = primaryB.rawMotion[i];
		const auto& c = observer.rawMotion[i];
		const auto& d = celestial.rawMotion[i];
		if( !( a == b ) || !( a == c ) )
			return Fail( "approach trajectory is not deterministic across runs and reference frames" );
		if( !( a == d ) )
			return Fail( "celestial balls perturbed the approach trajectory" );
	}

	std::printf(
		"Destiny PL-11D approach contract: samples=%zu min-center=%.6f final-center=%.6f "
		"final-speed=%.9f frames=primary+observer+celestial\n",
		primaryA.rawMotion.size(), primaryA.minimumCenterDistance, primaryA.finalCenterDistance,
		primaryA.finalSpeed );
	return 0;
}
