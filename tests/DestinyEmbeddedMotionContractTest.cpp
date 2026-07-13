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

const char* g_moduleName = "DestinyEmbeddedMotionContractTest";

BLUE_DEFINE_INTERFACE( ITriVectorFunction );
BLUE_DEFINE_INTERFACE( ITriQuaternionFunction );
BLUE_DEFINE_INTERFACE( IEveReferencePoint );
BLUE_DEFINE_INTERFACE( IEveBallpark );

namespace
{
constexpr Be::Time kTicksPerSecond = 10000000;
constexpr uint64_t kFrameCount = 1200;
constexpr uint64_t kCommandFrame = 180;

struct ReferenceSample
{
	std::array<double, 3> position;
	std::array<double, 3> velocity;
	std::array<double, 3> acceleration;
};

struct ScenarioResult
{
	std::vector<ReferenceSample> rawMotion;
	double initialRoll = 0.0;
	double finalRoll = 0.0;
};

int Fail( const std::string& message )
{
	std::fprintf( stderr, "DestinyEmbeddedMotionContractTest: %s\n", message.c_str() );
	return 1;
}

bool Near( double a, double b, double epsilon )
{
	return std::abs( a - b ) <= epsilon;
}

bool LoadGotoCorpus( std::vector<ReferenceSample>& samples )
{
	std::ifstream input( DESTINY_MOTION_CORPUS_DIR "/pl11a-goto.csv" );
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
		{
			return false;
		}
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
	return samples.size() == 17;
}

bool ValidateClosedFormCorpus( const std::vector<ReferenceSample>& samples )
{
	const double mass = 975000.0 * static_cast<double>( 2.87f );
	const double friction = 1000000.0;
	const double timeFactor = std::exp( -friction / mass );
	const double frictionSquared = friction * friction;
	std::array<double, 3> position = { 0.0, 0.0, -1000.0 };
	std::array<double, 3> velocity = {};
	for( const auto& sample : samples )
	{
		for( size_t axis = 0; axis < 3; ++axis )
		{
			const double acceleration = sample.acceleration[axis];
			position[axis] = ( mass *
							 ( mass * acceleration * ( timeFactor - 1.0 ) +
							   friction * ( acceleration + velocity[axis] - timeFactor * velocity[axis] ) ) +
						 position[axis] * frictionSquared ) /
				frictionSquared;
			velocity[axis] =
				( mass * acceleration - ( mass * acceleration - velocity[axis] * friction ) * timeFactor ) /
				friction;
			if( !Near( position[axis], sample.position[axis], 1e-8 ) ||
				!Near( velocity[axis], sample.velocity[axis], 1e-8 ) )
			{
				return false;
			}
		}
	}
	return true;
}

double RollMagnitude( const float rotation[4] )
{
	return 2.0 * std::atan2( std::abs( rotation[2] ), std::abs( rotation[3] ) );
}

bool RunScenario(
	DestinyEmbeddedReferenceFrame referenceFrame,
	const std::vector<ReferenceSample>& expected,
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

	const double target[3] = { 0.0, 0.0, 1000.0 };
	const double direction[3] = { 0.0, 0.0, 1.0 };
	const double zeroDirection[3] = { 0.0, 0.0, 0.0 };
	if( Destiny_CommandEmbeddedGoto( session, 2 * kTicksPerSecond, target ) ||
		Destiny_CommandEmbeddedGotoDirection( session, 2 * kTicksPerSecond, direction ) ||
		Destiny_CommandEmbeddedGotoDirection( session, kTicksPerSecond, zeroDirection ) )
	{
		error = "invalid point or direction GOTO command was accepted";
		Destiny_DestroyEmbeddedSession( session );
		return false;
	}

	uint64_t previousEvolveCount = 0;
	for( uint64_t frame = 0; frame < kFrameCount; ++frame )
	{
		const Be::Time simulationTime = static_cast<Be::Time>( frame * kTicksPerSecond / 60 );
		if( frame == kCommandFrame &&
			!Destiny_CommandEmbeddedGoto( session, simulationTime, target ) )
		{
			error = "frame-180 GOTO command failed";
			Destiny_DestroyEmbeddedSession( session );
			return false;
		}
		if( frame == kCommandFrame && Destiny_CommandEmbeddedStop( session, simulationTime ) )
		{
			error = "duplicate command timing was accepted";
			Destiny_DestroyEmbeddedSession( session );
			return false;
		}
		if( frame == kCommandFrame )
		{
			DestinyEmbeddedDiagnostics queued = {};
			if( !Destiny_GetEmbeddedDiagnostics( session, &queued ) || queued.mode != DSTBALL_STOP )
			{
				error = "GOTO command changed mode before its effective tick";
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
		if( frame == 0 )
			result.initialRoll = RollMagnitude( diagnostics.rotation );
		result.finalRoll = RollMagnitude( diagnostics.rotation );

		const double quaternionLength = std::sqrt(
			diagnostics.rotation[0] * diagnostics.rotation[0] +
			diagnostics.rotation[1] * diagnostics.rotation[1] +
			diagnostics.rotation[2] * diagnostics.rotation[2] +
			diagnostics.rotation[3] * diagnostics.rotation[3] );
		const double angularSpeed = std::sqrt(
			diagnostics.angularVelocity[0] * diagnostics.angularVelocity[0] +
			diagnostics.angularVelocity[1] * diagnostics.angularVelocity[1] +
			diagnostics.angularVelocity[2] * diagnostics.angularVelocity[2] );
		if( !std::isfinite( quaternionLength ) || !Near( quaternionLength, 1.0, 1e-5 ) ||
			!std::isfinite( angularSpeed ) || angularSpeed > 1.001 )
		{
			error = "native orientation became invalid";
			Destiny_DestroyEmbeddedSession( session );
			return false;
		}

		if( diagnostics.directEvolveCount != previousEvolveCount )
		{
			previousEvolveCount = diagnostics.directEvolveCount;
			if( previousEvolveCount <= 2 )
			{
				if( !Near( diagnostics.rawPosition[2], -1000.0, 1e-10 ) ||
					!Near( diagnostics.rawVelocity[2], 0.0, 1e-10 ) )
				{
					error = "STOP corpus moved before the command";
					Destiny_DestroyEmbeddedSession( session );
					return false;
				}
			}
			else
			{
				const size_t motionIndex = static_cast<size_t>( previousEvolveCount - 3 );
				bool matches = motionIndex < expected.size();
				for( size_t axis = 0; axis < 3 && matches; ++axis )
				{
					matches = Near( diagnostics.rawPosition[axis], expected[motionIndex].position[axis], 1e-5 ) &&
						Near( diagnostics.rawVelocity[axis], expected[motionIndex].velocity[axis], 1e-5 ) &&
						Near( diagnostics.rawAcceleration[axis], expected[motionIndex].acceleration[axis], 1e-5 );
				}
				if( !matches )
				{
					char detail[512] = {};
					const ReferenceSample empty = {};
					const auto& reference = motionIndex < expected.size() ? expected[motionIndex] : empty;
					std::snprintf(
						detail,
						sizeof( detail ),
						"GOTO tick %zu diverged: z position=%.12f/%.12f velocity=%.12f/%.12f acceleration=%.12f/%.12f",
						motionIndex + 1,
						diagnostics.rawPosition[2],
						reference.position[2],
						diagnostics.rawVelocity[2],
						reference.velocity[2],
						diagnostics.rawAcceleration[2],
						reference.acceleration[2] );
					error = detail;
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
				result.rawMotion.push_back( sample );
			}
		}

		const bool primaryEgo = referenceFrame == DESTINY_EMBEDDED_PRIMARY_EGO;
		if( diagnostics.primaryBallId != 1 || diagnostics.egoBallId != ( primaryEgo ? 1 : 2 ) ||
			diagnostics.orientationPinCount != 0 || diagnostics.schedulerRegistered ||
			diagnostics.startCallCount || diagnostics.onTickCallCount || diagnostics.pythonCallbackCount )
		{
			char detail[512] = {};
			std::snprintf(
				detail,
				sizeof( detail ),
				"embedded runtime mismatch: primary=%lld ego=%lld pins=%llu scheduler=%d start=%llu onTick=%llu callbacks=%llu",
				static_cast<long long>( diagnostics.primaryBallId ),
				static_cast<long long>( diagnostics.egoBallId ),
				static_cast<unsigned long long>( diagnostics.orientationPinCount ),
				diagnostics.schedulerRegistered ? 1 : 0,
				static_cast<unsigned long long>( diagnostics.startCallCount ),
				static_cast<unsigned long long>( diagnostics.onTickCallCount ),
				static_cast<unsigned long long>( diagnostics.pythonCallbackCount ) );
			error = detail;
			Destiny_DestroyEmbeddedSession( session );
			return false;
		}
		for( size_t axis = 0; axis < 3; ++axis )
		{
			const double expectedReference = primaryEgo ? diagnostics.absolutePosition[axis] : 0.0;
			const double expectedRelative = primaryEgo ? 0.0 : diagnostics.absolutePosition[axis];
			if( !Near( diagnostics.referencePoint[axis], expectedReference, 1e-5 ) ||
				!Near( diagnostics.position[axis], expectedRelative, 1e-3 ) )
			{
				error = "render-relative position did not match the selected reference frame";
				Destiny_DestroyEmbeddedSession( session );
				return false;
			}
		}
	}

	DestinyEmbeddedDiagnostics diagnostics = {};
	const bool diagnosticsOk = Destiny_GetEmbeddedDiagnostics( session, &diagnostics );
	Destiny_DestroyEmbeddedSession( session );
	if( !diagnosticsOk || diagnostics.directEvolveCount != 19 || diagnostics.commandCount != 1 ||
		diagnostics.lastCommandTime != 3 * kTicksPerSecond || diagnostics.mode != DSTBALL_GOTO ||
		result.rawMotion.size() != expected.size() || result.finalRoll >= result.initialRoll )
	{
		error = "final GOTO counters or native roll contract failed";
		return false;
	}
	return true;
}

bool ValidateGotoDirectionCommand( std::string& error )
{
	DestinyEmbeddedBallConfig config = {};
	config.ballId = 1;
	config.solarSystemId = 30005286;
	config.mass = 975000.0;
	config.radius = 35.0f;
	config.maximumVelocity = 312.0f;
	config.maximumAngularVelocity = 1.0f;
	config.agility = 2.87f;
	config.rotationalAgility = 1.0f;
	config.speedFraction = 1.0f;
	config.isFree = true;
	config.isMassive = true;
	config.isInteractive = true;

	DestinyEmbeddedSessionOptions options = {};
	options.orientationPolicy = DESTINY_EMBEDDED_NATIVE_ORIENTATION;
	options.referenceFrame = DESTINY_EMBEDDED_PRIMARY_EGO;

	char createError[512] = {};
	DestinyEmbeddedSession* session = Destiny_CreateEmbeddedSessionWithOptions(
		&config, &options, createError, sizeof( createError ) );
	if( !session )
	{
		error = createError;
		return false;
	}

	const double direction[3] = { 0.0, 0.0, 1.0 };
	const bool commanded = Destiny_CommandEmbeddedGotoDirection( session, kTicksPerSecond, direction );
	const bool advanced = commanded && Destiny_AdvanceEmbeddedSession( session, 2 * kTicksPerSecond );
	DestinyEmbeddedDiagnostics diagnostics = {};
	const bool diagnosed = advanced && Destiny_GetEmbeddedDiagnostics( session, &diagnostics );
	Destiny_DestroyEmbeddedSession( session );
	if( !diagnosed || diagnostics.mode != DESTINY_EMBEDDED_BALL_MODE_GOTO ||
		diagnostics.commandCount != 1 || diagnostics.lastCommandTime != kTicksPerSecond ||
		std::abs( diagnostics.rawVelocity[0] ) > 1e-10 || std::abs( diagnostics.rawVelocity[1] ) > 1e-10 ||
		diagnostics.rawVelocity[2] <= 0.0 )
	{
		error = "valid GotoDirection command did not enter positive-Z GOTO motion";
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
	if( !BeClasses )
		return Fail( "Blue startup did not initialize BeClasses" );

	DestinyEmbeddedRegistration registration = {};
	if( !Destiny_RegisterBlueClasses( &registration ) || registration.discoveredClassCount != 10 )
		return Fail( "embedded registration failed" );

	std::vector<ReferenceSample> expected;
	if( !LoadGotoCorpus( expected ) || !ValidateClosedFormCorpus( expected )
		|| !std::ifstream( DESTINY_MOTION_CORPUS_DIR "/pl11a-stop.csv" )
		|| !std::ifstream( DESTINY_MOTION_CORPUS_DIR "/pl11a-provenance.json" ) )
	{
		return Fail( "motion reference corpus is missing or invalid" );
	}

	ScenarioResult primaryA, primaryB, observer;
	std::string error;
	if( !RunScenario( DESTINY_EMBEDDED_PRIMARY_EGO, expected, primaryA, error ) ||
		!RunScenario( DESTINY_EMBEDDED_PRIMARY_EGO, expected, primaryB, error ) ||
		!RunScenario( DESTINY_EMBEDDED_FIXED_OBSERVER, expected, observer, error ) ||
		!ValidateGotoDirectionCommand( error ) )
	{
		return Fail( error );
	}
	if( primaryA.rawMotion.size() != primaryB.rawMotion.size() ||
		primaryA.rawMotion.size() != observer.rawMotion.size() )
	{
		return Fail( "reference-frame trajectories have different lengths" );
	}
	for( size_t i = 0; i < primaryA.rawMotion.size(); ++i )
	{
		const auto& a = primaryA.rawMotion[i];
		const auto& b = primaryB.rawMotion[i];
		const auto& c = observer.rawMotion[i];
		if( a.position != b.position || a.velocity != b.velocity || a.acceleration != b.acceleration ||
			a.position != c.position || a.velocity != c.velocity || a.acceleration != c.acceleration )
		{
			return Fail( "native trajectory is not deterministic across runs and reference frames" );
		}
	}

	std::printf(
		"Destiny PL-11A motion contract: samples=%zu evolves=19 commands=GOTO_POINT+GOTO_DIRECTION "
		"nativeRoll=%.6f->%.6f frames=primary+observer\n",
		primaryA.rawMotion.size(),
		primaryA.initialRoll,
		primaryA.finalRoll );
	return 0;
}
