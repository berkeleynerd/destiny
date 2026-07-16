// Copyright (c) 2026 CCP Games

#include "StdAfx.h"

#include "DestinyEmbedded.h"

#include "Ball.h"
#include "Ballpark.h"
#include "DstConstants.h"
#include "Settings.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <new>
#include <unordered_set>
#include <vector>

static_assert( DESTINY_EMBEDDED_BALL_MODE_FOLLOW == DSTBALL_FOLLOW, "embedded ball-mode mirror diverged" );
static_assert( DESTINY_EMBEDDED_BALL_MODE_STOP == DSTBALL_STOP, "embedded ball-mode mirror diverged" );
static_assert( DESTINY_EMBEDDED_BALL_MODE_GOTO == DSTBALL_GOTO, "embedded ball-mode mirror diverged" );
static_assert( DESTINY_EMBEDDED_BALL_MODE_ORBIT == DSTBALL_ORBIT, "embedded ball-mode mirror diverged" );
static_assert( DESTINY_EMBEDDED_BALL_MODE_RIGID == DSTBALL_RIGID, "embedded ball-mode mirror diverged" );

namespace
{
constexpr Be::Time kTicksPerSecond = 10000000;
constexpr size_t kMaxCelestialBalls = 4;
std::atomic<uint64_t> s_startCallCount = 0;
std::atomic<uint64_t> s_onTickCallCount = 0;
std::atomic<uint64_t> s_pythonCallbackCount = 0;
std::atomic<bool> s_sessionActive = false;
DestinyEmbeddedSession* s_activeSession = nullptr;
std::atomic<uint64_t> s_fullStateCreationAttemptCount = 0;
std::atomic<uint64_t> s_fullStatePreflightAttemptCount = 0;
std::atomic<uint64_t> s_fullStatePreflightRejectionCount = 0;
std::atomic<uint64_t> s_fullStateSessionSlotAcquisitionCount = 0;
std::atomic<uint64_t> s_fullStateSessionAllocationCount = 0;
std::atomic<uint64_t> s_fullStateGlobalSettingsMutationCount = 0;
std::atomic<uint64_t> s_fullStateBallparkAllocationCount = 0;
std::atomic<uint64_t> s_fullStateSuccessfulCreationCount = 0;

void SetError( char* error, size_t errorSize, const char* message )
{
	if( error && errorSize )
		std::snprintf( error, errorSize, "%s", message );
}

bool RejectFullState(
	DestinyEmbeddedFullStatePreflightDiagnostics* diagnostics,
	DestinyEmbeddedFullStateRejection rejection,
	size_t bytesConsumed,
	uint32_t parsedBallCount,
	int32_t packetTimestamp,
	char* error,
	size_t errorSize,
	const char* message )
{
	if( diagnostics )
	{
		diagnostics->rejection = rejection;
		diagnostics->bytesConsumed = bytesConsumed;
		diagnostics->parsedBallCount = parsedBallCount;
		diagnostics->packetTimestamp = packetTimestamp;
	}
	SetError( error, errorSize, message );
	return false;
}

bool IsFinite( const DestinyEmbeddedBallConfig& config )
{
	if( !std::isfinite( config.mass ) || config.mass <= 0.0 ||
		!std::isfinite( config.radius ) || config.radius <= 0.0f ||
		!std::isfinite( config.maximumVelocity ) || config.maximumVelocity < 0.0f ||
		!std::isfinite( config.maximumAngularVelocity ) || config.maximumAngularVelocity < 0.0f ||
		!std::isfinite( config.agility ) || config.agility <= 0.0f ||
		!std::isfinite( config.rotationalAgility ) || config.rotationalAgility <= 0.0f ||
		!std::isfinite( config.speedFraction ) || config.speedFraction < 0.0f )
	{
		return false;
	}
	for( double value : config.position )
		if( !std::isfinite( value ) )
			return false;
	for( double value : config.velocity )
		if( !std::isfinite( value ) )
			return false;
	for( float value : config.rotation )
		if( !std::isfinite( value ) )
			return false;
	for( float value : config.angularVelocity )
		if( !std::isfinite( value ) )
			return false;
	return true;
}

bool IsFinite( const DestinyEmbeddedSessionOptions& options )
{
	if( options.orientationPolicy < DESTINY_EMBEDDED_PIN_INITIAL ||
		options.orientationPolicy > DESTINY_EMBEDDED_NATIVE_ORIENTATION ||
		options.referenceFrame < DESTINY_EMBEDDED_PRIMARY_EGO ||
		options.referenceFrame > DESTINY_EMBEDDED_FIXED_OBSERVER ||
		options.orbitPolicy < DESTINY_EMBEDDED_ORBIT_CHECKOUT_DEFAULT ||
		options.orbitPolicy > DESTINY_EMBEDDED_ORBIT_FRONTIER_NEW )
	{
		return false;
	}
	for( double value : options.observerPosition )
		if( !std::isfinite( value ) )
			return false;
	return true;
}

struct ParsedFullStateBall
{
	int64_t id = 0;
	uint8_t mode = 0;
	uint8_t flags = 0;
	int64_t followId = 0;
	int64_t ownerId = 0;
};

struct ParsedFullState
{
	int32_t timestamp = 0;
	std::vector<ParsedFullStateBall> balls;
};

class FullStateCursor
{
public:
	FullStateCursor( const void* data, size_t size ) :
		mData( static_cast<const uint8_t*>( data ) ), mSize( size )
	{
	}

	template <typename T>
	bool Read( T& value )
	{
		if( sizeof( T ) > mSize - mPosition )
			return false;
		std::memcpy( &value, mData + mPosition, sizeof( T ) );
		mPosition += sizeof( T );
		return true;
	}

	bool Empty() const
	{
		return mPosition == mSize;
	}
	size_t Position() const
	{
		return mPosition;
	}

private:
	const uint8_t* mData = nullptr;
	size_t mSize = 0;
	size_t mPosition = 0;
};

template <typename T>
bool IsFiniteVector( const T* values, size_t count )
{
	for( size_t i = 0; i < count; ++i )
		if( !std::isfinite( values[i] ) )
			return false;
	return true;
}

template <typename T>
bool IsBoundedVector( const T* values, size_t count, double limit )
{
	if( !IsFiniteVector( values, count ) )
		return false;
	for( size_t i = 0; i < count; ++i )
		if( std::fabs( static_cast<double>( values[i] ) ) > limit )
			return false;
	return true;
}

bool IsKnownFullStateMode( uint8_t mode )
{
	switch( mode )
	{
	case DSTBALL_GOTO:
	case DSTBALL_FOLLOW:
	case DSTBALL_STOP:
	case DSTBALL_WARP:
	case DSTBALL_ORBIT:
	case DSTBALL_MISSILE:
	case DSTBALL_MUSHROOM:
	case DSTBALL_TROLL:
	case DSTBALL_FIELD:
	case DSTBALL_RIGID:
	case DSTBALL_FORMATION:
		return true;
	default:
		return false;
	}
}

bool ParseEmbeddedFullState(
	const void* packet,
	size_t packetSize,
	ParsedFullState& parsed,
	DestinyEmbeddedFullStatePreflightDiagnostics* diagnostics,
	char* error,
	size_t errorSize )
{
	parsed = {};
	constexpr size_t kMaximumPacketBytes = 64 * 1024 * 1024;
	constexpr size_t kMaximumBallCount = 65536;
	if( !packet || packetSize < sizeof( uint8_t ) + sizeof( int32_t ) ||
		packetSize > kMaximumPacketBytes )
	{
		return RejectFullState(
			diagnostics,
			packetSize > kMaximumPacketBytes ? DESTINY_EMBEDDED_FULL_STATE_REJECT_OUT_OF_RANGE_VALUE :
				DESTINY_EMBEDDED_FULL_STATE_REJECT_SHORT_PACKET,
			0,
			0,
			0,
			error,
			errorSize,
			packetSize > kMaximumPacketBytes ? "Full-state packet exceeds the bounded byte size" :
				"Full-state packet is short" );
	}
	FullStateCursor cursor( packet, packetSize );
	uint8_t packetType = 0;
	if( !cursor.Read( packetType ) || packetType != DESTINY_FULLSTATE || !cursor.Read( parsed.timestamp ) )
	{
		return RejectFullState(
			diagnostics,
			DESTINY_EMBEDDED_FULL_STATE_REJECT_INVALID_FRAMING,
			cursor.Position(),
			0,
			parsed.timestamp,
			error,
			errorSize,
			"Full-state packet framing is invalid" );
	}

	constexpr uint8_t kCompoundFlags =
		DSTBALL_HASMINIBOXES | DSTBALL_HASMINIBALLS | DSTBALL_HASMINICAPSULES;
	constexpr double kMaximumCoordinate = 1.0e15;
	constexpr double kMaximumScalar = 1.0e15;
	std::unordered_set<int64_t> ids;
	auto reject = [&]( DestinyEmbeddedFullStateRejection rejection, const char* message ) {
		return RejectFullState(
			diagnostics,
			rejection,
			cursor.Position(),
			static_cast<uint32_t>( parsed.balls.size() ),
			parsed.timestamp,
			error,
			errorSize,
			message );
	};
	while( !cursor.Empty() )
	{
		if( parsed.balls.size() >= kMaximumBallCount )
		{
			return RejectFullState(
				diagnostics,
				DESTINY_EMBEDDED_FULL_STATE_REJECT_OUT_OF_RANGE_VALUE,
				cursor.Position(),
				static_cast<uint32_t>( parsed.balls.size() ),
				parsed.timestamp,
				error,
				errorSize,
				"Full-state packet exceeds the bounded ball inventory" );
		}
		ParsedFullStateBall ball;
		float radius = 0.0f;
		double position[3] = {};
		if( !cursor.Read( ball.id ) || !cursor.Read( ball.mode ) || !cursor.Read( radius ) ||
			!cursor.Read( position[0] ) || !cursor.Read( position[1] ) || !cursor.Read( position[2] ) ||
			!cursor.Read( ball.flags ) )
		{
			return RejectFullState(
				diagnostics,
				parsed.balls.empty() ? DESTINY_EMBEDDED_FULL_STATE_REJECT_SHORT_PACKET :
					DESTINY_EMBEDDED_FULL_STATE_REJECT_TRAILING_BYTES,
				cursor.Position(),
				static_cast<uint32_t>( parsed.balls.size() ),
				parsed.timestamp,
				error,
				errorSize,
				parsed.balls.empty() ? "Full-state packet ends inside its first ball header" :
					"Full-state packet has trailing bytes after a complete ball" );
		}
		if( ball.id <= 0 )
		{
			return RejectFullState(
				diagnostics,
				DESTINY_EMBEDDED_FULL_STATE_REJECT_INVALID_ID,
				cursor.Position(),
				static_cast<uint32_t>( parsed.balls.size() ),
				parsed.timestamp,
				error,
				errorSize,
				"Full-state packet contains a zero or negative ball ID" );
		}
		if( !ids.insert( ball.id ).second )
		{
			return RejectFullState(
				diagnostics,
				DESTINY_EMBEDDED_FULL_STATE_REJECT_DUPLICATE_ID,
				cursor.Position(),
				static_cast<uint32_t>( parsed.balls.size() ),
				parsed.timestamp,
				error,
				errorSize,
				"Full-state packet contains a duplicate ball ID" );
		}
		if( !IsKnownFullStateMode( ball.mode ) )
		{
			return RejectFullState(
				diagnostics,
				DESTINY_EMBEDDED_FULL_STATE_REJECT_UNKNOWN_MODE,
				cursor.Position(),
				static_cast<uint32_t>( parsed.balls.size() ),
				parsed.timestamp,
				error,
				errorSize,
				"Full-state packet contains an unknown ball mode" );
		}
		if( ( ball.flags & kCompoundFlags ) != 0 )
		{
			return RejectFullState(
				diagnostics,
				DESTINY_EMBEDDED_FULL_STATE_REJECT_UNSUPPORTED_FLAGS,
				cursor.Position(),
				static_cast<uint32_t>( parsed.balls.size() ),
				parsed.timestamp,
				error,
				errorSize,
				"Full-state packet uses compound-shape flags outside the initial PL-14G profile" );
		}
		if( !std::isfinite( radius ) || !IsFiniteVector( position, 3 ) )
		{
			return RejectFullState(
				diagnostics,
				DESTINY_EMBEDDED_FULL_STATE_REJECT_NONFINITE_VALUE,
				cursor.Position(),
				static_cast<uint32_t>( parsed.balls.size() ),
				parsed.timestamp,
				error,
				errorSize,
				"Full-state packet contains nonfinite ball geometry" );
		}
		if( radius <= 0.0f || radius > kMaximumScalar || std::fabs( position[0] ) > kMaximumCoordinate ||
			std::fabs( position[1] ) > kMaximumCoordinate || std::fabs( position[2] ) > kMaximumCoordinate )
		{
			return RejectFullState(
				diagnostics,
				DESTINY_EMBEDDED_FULL_STATE_REJECT_OUT_OF_RANGE_VALUE,
				cursor.Position(),
				static_cast<uint32_t>( parsed.balls.size() ),
				parsed.timestamp,
				error,
				errorSize,
				"Full-state packet contains out-of-range ball geometry" );
		}

		if( ball.mode != DSTBALL_RIGID )
		{
			double mass = 0.0;
			int8_t cloaked = 0;
			int64_t harmonic = 0;
			int32_t corporation = 0;
			int32_t alliance = 0;
			if( !cursor.Read( mass ) || !cursor.Read( cloaked ) || !cursor.Read( harmonic ) ||
				!cursor.Read( corporation ) || !cursor.Read( alliance ) ||
				!std::isfinite( mass ) || mass <= 0.0 )
			{
				return reject(
					std::isfinite( mass ) ? DESTINY_EMBEDDED_FULL_STATE_REJECT_OUT_OF_RANGE_VALUE :
						DESTINY_EMBEDDED_FULL_STATE_REJECT_NONFINITE_VALUE,
					"Full-state packet contains invalid dynamic-ball metadata" );
			}
			(void)cloaked;
			(void)harmonic;
			(void)corporation;
			(void)alliance;
		}

		if( ( ball.flags & DSTBALL_ISFREE ) != 0 )
		{
			float maximumVelocity = 0.0f;
			double velocity[3] = {};
			float agility = 0.0f;
			float speedFraction = 0.0f;
			float rotation[4] = {};
			float maximumAngularVelocity = 0.0f;
			float angularVelocity[3] = {};
			float rotationalAgility = 0.0f;
			if( !cursor.Read( maximumVelocity ) ||
				!cursor.Read( velocity[0] ) || !cursor.Read( velocity[1] ) || !cursor.Read( velocity[2] ) ||
				!cursor.Read( agility ) || !cursor.Read( speedFraction ) ||
				!cursor.Read( rotation[0] ) || !cursor.Read( rotation[1] ) ||
				!cursor.Read( rotation[2] ) || !cursor.Read( rotation[3] ) ||
				!cursor.Read( maximumAngularVelocity ) ||
				!cursor.Read( angularVelocity[0] ) || !cursor.Read( angularVelocity[1] ) ||
				!cursor.Read( angularVelocity[2] ) || !cursor.Read( rotationalAgility ) ||
				!std::isfinite( maximumVelocity ) || maximumVelocity < 0.0f ||
				maximumVelocity > kMaximumScalar || !IsBoundedVector( velocity, 3, kMaximumScalar ) ||
				!std::isfinite( agility ) || agility <= 0.0f || agility > kMaximumScalar ||
				!std::isfinite( speedFraction ) || speedFraction < 0.0f || speedFraction > 1.0e6f ||
				!IsBoundedVector( rotation, 4, 2.0 ) || !std::isfinite( maximumAngularVelocity ) ||
				maximumAngularVelocity < 0.0f || maximumAngularVelocity > kMaximumScalar ||
				!IsBoundedVector( angularVelocity, 3, kMaximumScalar ) ||
				!std::isfinite( rotationalAgility ) || rotationalAgility <= 0.0f ||
				rotationalAgility > kMaximumScalar )
			{
				return reject(
					DESTINY_EMBEDDED_FULL_STATE_REJECT_OUT_OF_RANGE_VALUE,
					"Full-state packet contains invalid free-ball motion state" );
			}
			const double rotationLengthSquared =
				static_cast<double>( rotation[0] ) * rotation[0] +
				static_cast<double>( rotation[1] ) * rotation[1] +
				static_cast<double>( rotation[2] ) * rotation[2] +
				static_cast<double>( rotation[3] ) * rotation[3];
			if( rotationLengthSquared < 1.0e-12 )
			{
				return reject(
					DESTINY_EMBEDDED_FULL_STATE_REJECT_OUT_OF_RANGE_VALUE,
					"Full-state packet contains a degenerate orientation" );
			}
		}

		int8_t formationId = 0;
		if( !cursor.Read( formationId ) )
		{
			return reject(
				DESTINY_EMBEDDED_FULL_STATE_REJECT_SHORT_PACKET,
				"Full-state packet ends before the formation field" );
		}
		(void)formationId;

		float followRange = 0.0f;
		int32_t effectStamp = 0;
		double vector[3] = {};
		double scalar = 0.0;
		switch( ball.mode )
		{
		case DSTBALL_FOLLOW:
		case DSTBALL_ORBIT:
			if( !cursor.Read( ball.followId ) || !cursor.Read( followRange ) ||
				!std::isfinite( followRange ) || followRange < 0.0f || followRange > kMaximumScalar )
			{
				return reject(
					std::isfinite( followRange ) ? DESTINY_EMBEDDED_FULL_STATE_REJECT_OUT_OF_RANGE_VALUE :
						DESTINY_EMBEDDED_FULL_STATE_REJECT_NONFINITE_VALUE,
					"Full-state packet contains invalid follow state" );
			}
			break;
		case DSTBALL_FORMATION:
			if( !cursor.Read( ball.followId ) || !cursor.Read( followRange ) || !cursor.Read( effectStamp ) ||
				!std::isfinite( followRange ) || followRange < 0.0f || followRange > kMaximumScalar )
			{
				return reject(
					std::isfinite( followRange ) ? DESTINY_EMBEDDED_FULL_STATE_REJECT_OUT_OF_RANGE_VALUE :
						DESTINY_EMBEDDED_FULL_STATE_REJECT_NONFINITE_VALUE,
					"Full-state packet contains invalid formation state" );
			}
			break;
		case DSTBALL_MISSILE:
			if( !cursor.Read( ball.followId ) || !cursor.Read( followRange ) || !cursor.Read( ball.ownerId ) ||
				!cursor.Read( effectStamp ) || !cursor.Read( vector[0] ) || !cursor.Read( vector[1] ) ||
				!cursor.Read( vector[2] ) || !std::isfinite( followRange ) || followRange < 0.0f ||
				followRange > kMaximumScalar || !IsBoundedVector( vector, 3, kMaximumCoordinate ) )
			{
				return reject(
					DESTINY_EMBEDDED_FULL_STATE_REJECT_OUT_OF_RANGE_VALUE,
					"Full-state packet contains invalid missile state" );
			}
			break;
		case DSTBALL_GOTO:
			if( !cursor.Read( vector[0] ) || !cursor.Read( vector[1] ) || !cursor.Read( vector[2] ) ||
				!IsBoundedVector( vector, 3, kMaximumCoordinate ) )
			{
				return reject(
					DESTINY_EMBEDDED_FULL_STATE_REJECT_OUT_OF_RANGE_VALUE,
					"Full-state packet contains invalid GOTO state" );
			}
			break;
		case DSTBALL_WARP: {
			int64_t minimumRangeBits = 0;
			if( !cursor.Read( vector[0] ) || !cursor.Read( vector[1] ) || !cursor.Read( vector[2] ) ||
				!cursor.Read( effectStamp ) || !cursor.Read( scalar ) || !cursor.Read( minimumRangeBits ) ||
				!cursor.Read( ball.ownerId ) || !IsBoundedVector( vector, 3, kMaximumCoordinate ) ||
				!std::isfinite( scalar ) || scalar < 0.0 || scalar > kMaximumScalar )
			{
				return reject(
					DESTINY_EMBEDDED_FULL_STATE_REJECT_OUT_OF_RANGE_VALUE,
					"Full-state packet contains invalid warp state" );
			}
			double minimumRange = 0.0;
			std::memcpy( &minimumRange, &minimumRangeBits, sizeof( minimumRange ) );
			if( !std::isfinite( minimumRange ) || minimumRange < 0.0 ||
				minimumRange > kMaximumScalar || ball.ownerId < 1 )
			{
				return reject(
					std::isfinite( minimumRange ) ? DESTINY_EMBEDDED_FULL_STATE_REJECT_OUT_OF_RANGE_VALUE :
						DESTINY_EMBEDDED_FULL_STATE_REJECT_NONFINITE_VALUE,
					"Full-state packet contains invalid warp parameters" );
			}
			break;
		}
		case DSTBALL_MUSHROOM:
			if( !cursor.Read( followRange ) || !cursor.Read( scalar ) || !cursor.Read( effectStamp ) ||
				!cursor.Read( ball.ownerId ) || !std::isfinite( followRange ) ||
				std::fabs( static_cast<double>( followRange ) ) > kMaximumScalar ||
				!std::isfinite( scalar ) || std::fabs( scalar ) > kMaximumScalar )
			{
				return reject(
					DESTINY_EMBEDDED_FULL_STATE_REJECT_OUT_OF_RANGE_VALUE,
					"Full-state packet contains invalid mushroom state" );
			}
			break;
		case DSTBALL_TROLL:
			if( !cursor.Read( effectStamp ) )
			{
				return reject(
					DESTINY_EMBEDDED_FULL_STATE_REJECT_SHORT_PACKET,
					"Full-state packet contains invalid troll state" );
			}
			break;
		case DSTBALL_STOP:
		case DSTBALL_FIELD:
		case DSTBALL_RIGID:
			break;
		default:
			return reject(
				DESTINY_EMBEDDED_FULL_STATE_REJECT_UNKNOWN_MODE,
				"Full-state packet mode is unsupported" );
		}
		if( ( ball.mode == DSTBALL_FOLLOW || ball.mode == DSTBALL_ORBIT ||
			  ball.mode == DSTBALL_FORMATION || ball.mode == DSTBALL_MISSILE ) &&
			ball.followId <= 0 )
		{
			return reject(
				DESTINY_EMBEDDED_FULL_STATE_REJECT_MISSING_FOLLOW,
				"Full-state packet has a missing follow target" );
		}
		parsed.balls.push_back( ball );
	}

	if( parsed.balls.empty() )
	{
		return reject(
			DESTINY_EMBEDDED_FULL_STATE_REJECT_SHORT_PACKET,
			"Full-state packet contains no balls" );
	}
	for( const ParsedFullStateBall& ball : parsed.balls )
	{
		if( ball.followId == 0 )
			continue;
		if( ball.followId == ball.id )
		{
			return reject(
				DESTINY_EMBEDDED_FULL_STATE_REJECT_SELF_FOLLOW,
				"Full-state packet has self follow topology" );
		}
		if( ids.find( ball.followId ) == ids.end() )
		{
			return reject(
				DESTINY_EMBEDDED_FULL_STATE_REJECT_MISSING_FOLLOW,
				"Full-state packet has a missing follow topology target" );
		}
		if( ball.mode == DSTBALL_MISSILE &&
			( ball.ownerId == 0 || ball.ownerId == ball.id || ids.find( ball.ownerId ) == ids.end() ) )
		{
			return reject(
				DESTINY_EMBEDDED_FULL_STATE_REJECT_INCOMPATIBLE_ROLES,
				"Full-state packet has invalid missile ownership" );
		}
	}
	if( diagnostics )
	{
		diagnostics->rejection = DESTINY_EMBEDDED_FULL_STATE_ACCEPTED;
		diagnostics->bytesConsumed = cursor.Position();
		diagnostics->parsedBallCount = static_cast<uint32_t>( parsed.balls.size() );
		diagnostics->packetTimestamp = parsed.timestamp;
		diagnostics->packetParsed = true;
	}
	return true;
}

const ParsedFullStateBall* FindParsedBall( const ParsedFullState& parsed, int64_t ballId )
{
	for( const ParsedFullStateBall& ball : parsed.balls )
		if( ball.id == ballId )
			return &ball;
	return nullptr;
}

bool ValidateFullStateRoles(
	const ParsedFullState& parsed,
	const DestinyEmbeddedFullStateDescriptor& descriptor,
	const DestinyEmbeddedSessionOptions& options,
	DestinyEmbeddedFullStatePreflightDiagnostics* diagnostics,
	char* error,
	size_t errorSize )
{
	auto reject = [&]( DestinyEmbeddedFullStateRejection rejection, const char* message ) {
		return RejectFullState(
			diagnostics,
			rejection,
			diagnostics ? diagnostics->bytesConsumed : 0,
			static_cast<uint32_t>( parsed.balls.size() ),
			parsed.timestamp,
			error,
			errorSize,
			message );
	};
	if( descriptor.wireProfile != DESTINY_EMBEDDED_DYNAMIC_ORIENTATION_V1 )
	{
		return reject(
			DESTINY_EMBEDDED_FULL_STATE_REJECT_UNKNOWN_WIRE_PROFILE,
			"Full-state descriptor has an unknown wire profile" );
	}
	if( descriptor.initialTimestamp != 0 || parsed.timestamp != 0 )
	{
		return reject(
			DESTINY_EMBEDDED_FULL_STATE_REJECT_NONZERO_TIMESTAMP,
			"Full-state descriptor or packet has a nonzero initial timestamp" );
	}
	if( descriptor.celestialBallCount > kMaxCelestialBalls || descriptor.solarSystemId <= 0 )
	{
		return reject(
			DESTINY_EMBEDDED_FULL_STATE_REJECT_OUT_OF_RANGE_VALUE,
			"Full-state descriptor contains an out-of-range system or role count" );
	}
	if( descriptor.primaryBallId <= 0 || descriptor.egoBallId <= 0 )
	{
		return reject(
			DESTINY_EMBEDDED_FULL_STATE_REJECT_MISSING_ROLES,
			"Full-state descriptor is missing its primary or ego role" );
	}
	if( options.referenceFrame == DESTINY_EMBEDDED_PRIMARY_EGO )
	{
		if( descriptor.egoBallId != descriptor.primaryBallId ||
			descriptor.observerBallId == descriptor.primaryBallId )
		{
			return reject(
				DESTINY_EMBEDDED_FULL_STATE_REJECT_CONFLICTING_ROLES,
				"Primary-ego full-state roles conflict" );
		}
	}
	else if( descriptor.observerBallId <= 0 || descriptor.egoBallId != descriptor.observerBallId ||
			 options.observerBallId != descriptor.observerBallId || descriptor.observerBallId == descriptor.primaryBallId )
	{
		return reject(
			DESTINY_EMBEDDED_FULL_STATE_REJECT_CONFLICTING_ROLES,
			"Fixed-observer full-state roles conflict" );
	}

	const ParsedFullStateBall* primary = FindParsedBall( parsed, descriptor.primaryBallId );
	const ParsedFullStateBall* ego = FindParsedBall( parsed, descriptor.egoBallId );
	if( !primary || !ego )
	{
		return reject(
			DESTINY_EMBEDDED_FULL_STATE_REJECT_MISSING_ROLES,
			"Full-state primary or ego role is missing" );
	}
	if( ( primary->flags & DSTBALL_ISFREE ) == 0 || primary->mode != DSTBALL_STOP )
	{
		return reject(
			DESTINY_EMBEDDED_FULL_STATE_REJECT_INCOMPATIBLE_ROLES,
			"Full-state primary role is incompatible" );
	}
	if( descriptor.observerBallId != 0 )
	{
		const ParsedFullStateBall* observer = FindParsedBall( parsed, descriptor.observerBallId );
		if( !observer )
		{
			return reject(
				DESTINY_EMBEDDED_FULL_STATE_REJECT_MISSING_ROLES,
				"Full-state observer role is missing" );
		}
		if( ( observer->flags & DSTBALL_ISFREE ) != 0 ||
			( observer->mode != DSTBALL_STOP && observer->mode != DSTBALL_RIGID ) )
		{
			return reject(
				DESTINY_EMBEDDED_FULL_STATE_REJECT_INCOMPATIBLE_ROLES,
				"Full-state observer role is incompatible" );
		}
	}

	std::unordered_set<int64_t> exclusiveRoles;
	exclusiveRoles.insert( descriptor.primaryBallId );
	if( descriptor.observerBallId != 0 )
		exclusiveRoles.insert( descriptor.observerBallId );
	if( descriptor.fixedTargetBallId != 0 )
	{
		const ParsedFullStateBall* fixed = FindParsedBall( parsed, descriptor.fixedTargetBallId );
		if( !fixed )
		{
			return reject(
				DESTINY_EMBEDDED_FULL_STATE_REJECT_MISSING_ROLES,
				"Full-state fixed-target role is missing" );
		}
		if( !exclusiveRoles.insert( descriptor.fixedTargetBallId ).second )
		{
			return reject(
				DESTINY_EMBEDDED_FULL_STATE_REJECT_CONFLICTING_ROLES,
				"Full-state fixed-target role conflicts with another role" );
		}
		if( ( fixed->flags & ( DSTBALL_ISFREE | DSTBALL_ISGLOBAL ) ) != 0 || fixed->mode != DSTBALL_STOP )
		{
			return reject(
				DESTINY_EMBEDDED_FULL_STATE_REJECT_INCOMPATIBLE_ROLES,
				"Full-state fixed-target role is incompatible" );
		}
	}
	for( uint32_t i = 0; i < descriptor.celestialBallCount; ++i )
	{
		const int64_t id = descriptor.celestialBallIds[i];
		const ParsedFullStateBall* celestial = FindParsedBall( parsed, id );
		if( id <= 0 || !celestial )
		{
			return reject(
				DESTINY_EMBEDDED_FULL_STATE_REJECT_MISSING_ROLES,
				"Full-state celestial role is missing" );
		}
		if( !exclusiveRoles.insert( id ).second )
		{
			return reject(
				DESTINY_EMBEDDED_FULL_STATE_REJECT_CONFLICTING_ROLES,
				"Full-state celestial role conflicts with another role" );
		}
		if( celestial->mode != DSTBALL_RIGID || ( celestial->flags & DSTBALL_ISGLOBAL ) == 0 ||
			( celestial->flags & DSTBALL_ISFREE ) != 0 )
		{
			return reject(
				DESTINY_EMBEDDED_FULL_STATE_REJECT_INCOMPATIBLE_ROLES,
				"Full-state celestial role is incompatible" );
		}
	}
	for( uint32_t i = descriptor.celestialBallCount; i < kMaxCelestialBalls; ++i )
	{
		if( descriptor.celestialBallIds[i] != 0 )
		{
			return reject(
				DESTINY_EMBEDDED_FULL_STATE_REJECT_CONFLICTING_ROLES,
				"Full-state descriptor has undeclared celestial role IDs" );
		}
	}
	if( diagnostics )
	{
		diagnostics->rejection = DESTINY_EMBEDDED_FULL_STATE_ACCEPTED;
		diagnostics->rolesValidated = true;
	}
	return true;
}

bool InspectEmbeddedFullStateInternal(
	const void* packet,
	size_t packetSize,
	const DestinyEmbeddedFullStateDescriptor* descriptor,
	const DestinyEmbeddedSessionOptions* options,
	DestinyEmbeddedFullStatePreflightDiagnostics& diagnostics,
	ParsedFullState* parsedOutput,
	char* error,
	size_t errorSize )
{
	if( error && errorSize )
		error[0] = '\0';
	diagnostics = {};
	diagnostics.packetSize = packetSize;
	diagnostics.sideEffectFree = true;
	if( !descriptor || !options || !IsFinite( *options ) )
	{
		return RejectFullState(
			&diagnostics,
			DESTINY_EMBEDDED_FULL_STATE_REJECT_INVALID_ARGUMENT,
			0,
			0,
			0,
			error,
			errorSize,
			"Embedded full-state inspection requires finite descriptor options" );
	}

	ParsedFullState parsed;
	if( !ParseEmbeddedFullState( packet, packetSize, parsed, &diagnostics, error, errorSize ) )
		return false;
	if( !ValidateFullStateRoles( parsed, *descriptor, *options, &diagnostics, error, errorSize ) )
		return false;
	if( parsedOutput )
		*parsedOutput = parsed;
	return true;
}
}

struct DestinyEmbeddedSession
{
	BluePtr<Ballpark> ballpark;
	ClientBall* ball = nullptr;
	int64_t observerRoleId = 0;
	Ball* fixedTarget = nullptr;
	int64_t fixedTargetRoleId = 0;
	Ball* orbitTarget = nullptr;
	Ball* celestials[kMaxCelestialBalls] = {};
	int64_t celestialRoleIds[kMaxCelestialBalls] = {};
	size_t celestialCount = 0;
	std::vector<ID> fullStateOrder;
	IRoot* ballRoot = nullptr;
	Be::Time lastRequestedTime = 0;
	Be::Time nextTickTime = kTicksPerSecond;
	uint64_t directEvolveCount = 0;
	uint64_t commandCount = 0;
	uint64_t orientationPinCount = 0;
	bool previousDynamicalOrientation = false;
	bool previousUseNewOrbit = false;
	DestinyEmbeddedOrientationPolicy orientationPolicy = DESTINY_EMBEDDED_PIN_INITIAL;
	DestinyEmbeddedReferenceFrame referenceFrame = DESTINY_EMBEDDED_PRIMARY_EGO;
	Be::Time lastCommandTime = 0;
	Be::Time pendingCommandTime = 0;
	int pendingCommand = 0;
	double pendingTarget[3] = {};
	int64_t pendingTargetBallId = 0;
	float pendingRange = 0.0f;
	double pendingMinRange = 0.0;
	int32_t pendingWarpFactor = 0;
	Vector3d previousOrbitRelative;
	bool hasPreviousOrbitRelative = false;
	double orbitAccumulatedPhase = 0.0;
	Quaternion authoredRotation = Quaternion( 0.0f, 0.0f, 0.0f, 1.0f );
	DestinyEmbeddedWarpEventCallback warpEventCallback = nullptr;
	void* warpEventUserData = nullptr;

	~DestinyEmbeddedSession()
	{
		ballpark.Unlock();
		if( ballRoot )
			ballRoot->Unlock();
		g_useDynamicalOrientation = previousDynamicalOrientation;
		g_useNewOrbit = previousUseNewOrbit;
		s_activeSession = nullptr;
		s_sessionActive = false;
	}
};

namespace
{
Ball* FindEmbeddedBall( DestinyEmbeddedSession* session, int64_t ballId )
{
	return session && ballId != 0 ? session->ballpark->DestinyEmbeddedFindBall( ballId ) : nullptr;
}

bool IsCelestialRole( const DestinyEmbeddedSession* session, int64_t ballId )
{
	if( !session )
		return false;
	for( size_t i = 0; i < session->celestialCount; ++i )
		if( session->celestialRoleIds[i] == ballId )
			return true;
	return false;
}

Ball* ResolveCommandTarget(
	DestinyEmbeddedSession* session,
	int64_t ballId,
	bool allowFixed,
	bool allowCelestial )
{
	Ball* ball = FindEmbeddedBall( session, ballId );
	if( !ball )
		return nullptr;
	const bool fixed = session->fixedTargetRoleId == ballId;
	return ( ( allowFixed && fixed ) || ( allowCelestial && IsCelestialRole( session, ballId ) ) ) ? ball : nullptr;
}

void InitializeEmbeddedPrimaryHistory( DestinyEmbeddedSession* session )
{
	ClientBall* ball = session->ball;
	session->authoredRotation = ball->mNewRot;
	ball->mLastRot = ball->mNewRot;
	ball->mLastSpeed = Quaternion( 0.0f, 0.0f, 0.0f, 0.0f );
	ball->mLastAngVel = ball->mNewAngVel;
	ball->mLastPos = ball->mNewPos;
	ball->mLastVel = ball->mNewVel;
	ball->mDeltaPos = Vector3d( 0.0, 0.0, 0.0 );
	ball->mLastG = Vector3d( 0.0, 0.0, 0.0 );
	ball->mLastC = Vector3d( 0.0, 0.0, 0.0 );
	ball->mTorque = Vector3( 0.0f, 0.0f, 0.0f );
	ball->mRoll = 0.0f;
	ball->mOldTime = -static_cast<Be::Time>( session->ballpark->mTickInterval ) * 10000;
	ball->mNewTime = 0;
	ball->mRotUpdateTime = 0;
	ball->mPosUpdateTime = 0;
}
}

void DestinyEmbeddedRecordOnTick()
{
	++s_onTickCallCount;
}

void DestinyEmbeddedRecordPythonCallback()
{
	++s_pythonCallbackCount;
}

void DestinyEmbeddedRecordStart()
{
	++s_startCallCount;
}

uint64_t DestinyEmbeddedGetOnTickCount()
{
	return s_onTickCallCount.load();
}

uint64_t DestinyEmbeddedGetPythonCallbackCount()
{
	return s_pythonCallbackCount.load();
}

uint64_t DestinyEmbeddedGetStartCount()
{
	return s_startCallCount.load();
}

void DestinyEmbeddedResetRuntimeCounters()
{
	s_startCallCount = 0;
	s_onTickCallCount = 0;
	s_pythonCallbackCount = 0;
}

bool DestinyEmbeddedPostEvent(
	void* ballpark,
	const char* timerName,
	const char* eventName,
	const char* format,
	long long ballId,
	long long eventTime )
{
	// The recovered warp event contract (docs/thunker-contract-audit.md):
	// exactly three semantic events survive the Python excision, forwarded
	// to the host callback when one is registered. Unknown names are
	// silently accepted (the historical surface treated them as posted).
	( void )ballpark;
	( void )timerName;
	( void )format;
	DestinyEmbeddedSession* session = s_activeSession;
	if( !session || !session->warpEventCallback || !eventName )
		return true;
	int warpEvent = -1;
	if( std::strcmp( eventName, "OnActivatingWarp" ) == 0 )
		warpEvent = DESTINY_EMBEDDED_WARP_EVENT_ACTIVATING;
	else if( std::strcmp( eventName, "OnDeactivatingWarp" ) == 0 )
		warpEvent = DESTINY_EMBEDDED_WARP_EVENT_DEACTIVATING;
	else if( std::strcmp( eventName, "OnExitWarp" ) == 0 )
		warpEvent = DESTINY_EMBEDDED_WARP_EVENT_EXIT;
	if( warpEvent >= 0 )
		session->warpEventCallback( warpEvent, ballId, eventTime, session->warpEventUserData );
	return true;
}


namespace
{
// Fixed caller-buffer stream for the embedded recorder: just enough of
// IBlueStream for the un-gated wire cores (Write/Seek), no Blue factory.
class EmbeddedByteStream final : public IBlueStream
{
public:
	EmbeddedByteStream( void* buffer, size_t capacity ) :
		mBuffer( static_cast<char*>( buffer ) ), mCapacity( capacity )
	{
	}
	ptrdiff_t Read( void* dest, ptrdiff_t count ) override
	{
		const size_t n = std::min( size_t( count ), mEnd - mPos );
		std::memcpy( dest, mBuffer + mPos, n );
		mPos += n;
		return ptrdiff_t( n );
	}
	ptrdiff_t Write( const void* source, size_t count ) override
	{
		if( mPos + count > mCapacity )
		{
			mOverflow = true;
			return 0;
		}
		std::memcpy( mBuffer + mPos, source, count );
		mPos += count;
		mEnd = std::max( mEnd, mPos );
		return ptrdiff_t( count );
	}
	ptrdiff_t Seek( ptrdiff_t distance, SeekOrigin method ) override
	{
		if( method == SO_BEGIN )
			mPos = size_t( distance );
		else if( method == SO_CURRENT )
			mPos += size_t( distance );
		else
			mPos = mEnd + size_t( distance );
		return ptrdiff_t( mPos );
	}
	ptrdiff_t GetPosition() override { return ptrdiff_t( mPos ); }
	ptrdiff_t GetSize() override { return ptrdiff_t( mEnd ); }
	bool LockData( void** data, size_t ) override { *data = mBuffer; return true; }
	bool UnlockData() override { return true; }
	const Be::ClassInfo* ClassType() const override { return nullptr; }
	bool QueryInterface( const Be::IID&, void**, BLUEQIOPT ) override { return false; }
	void Lock() override {}
	void Unlock() override {}
	long GetFlags() override { return 0; }
	int GetRefCount() const override { return 1; }
	IRoot* GetRootObject() const override
	{
		return const_cast<EmbeddedByteStream*>( this );
	}
	bool Overflowed() const { return mOverflow; }

protected:
	void FinalDelete() override {}

private:
	char* mBuffer;
	size_t mCapacity;
	size_t mPos = 0;
	size_t mEnd = 0;
	bool mOverflow = false;
};

class EmbeddedReadStream final : public IBlueStream
{
public:
	EmbeddedReadStream( const void* buffer, size_t size ) :
		mBuffer( static_cast<const char*>( buffer ) ), mSize( size )
	{
	}
	ptrdiff_t Read( void* dest, ptrdiff_t count ) override
	{
		if( count <= 0 || mPosition >= mSize )
			return 0;
		const size_t read = std::min( static_cast<size_t>( count ), mSize - mPosition );
		std::memcpy( dest, mBuffer + mPosition, read );
		mPosition += read;
		return static_cast<ptrdiff_t>( read );
	}
	ptrdiff_t Write( const void*, size_t ) override
	{
		return 0;
	}
	ptrdiff_t Seek( ptrdiff_t distance, SeekOrigin method ) override
	{
		ptrdiff_t base = 0;
		if( method == SO_CURRENT )
			base = static_cast<ptrdiff_t>( mPosition );
		else if( method == SO_END )
			base = static_cast<ptrdiff_t>( mSize );
		const ptrdiff_t target = base + distance;
		if( target < 0 || static_cast<size_t>( target ) > mSize )
			return -1;
		mPosition = static_cast<size_t>( target );
		return target;
	}
	ptrdiff_t GetPosition() override
	{
		return static_cast<ptrdiff_t>( mPosition );
	}
	ptrdiff_t GetSize() override
	{
		return static_cast<ptrdiff_t>( mSize );
	}
	bool LockData( void** data, size_t ) override
	{
		*data = const_cast<char*>( mBuffer );
		return true;
	}
	bool UnlockData() override
	{
		return true;
	}
	const Be::ClassInfo* ClassType() const override
	{
		return nullptr;
	}
	bool QueryInterface( const Be::IID&, void**, BLUEQIOPT ) override
	{
		return false;
	}
	void Lock() override
	{
	}
	void Unlock() override
	{
	}
	long GetFlags() override
	{
		return 0;
	}
	int GetRefCount() const override
	{
		return 1;
	}
	IRoot* GetRootObject() const override
	{
		return const_cast<EmbeddedReadStream*>( this );
	}

protected:
	void FinalDelete() override
	{
	}

private:
	const char* mBuffer = nullptr;
	size_t mSize = 0;
	size_t mPosition = 0;
};

class EmbeddedMeasureStream final : public IBlueStream
{
public:
	ptrdiff_t Read( void*, ptrdiff_t ) override
	{
		return 0;
	}
	ptrdiff_t Write( const void*, size_t count ) override
	{
		mPosition += count;
		mSize = std::max( mSize, mPosition );
		return static_cast<ptrdiff_t>( count );
	}
	ptrdiff_t Seek( ptrdiff_t distance, SeekOrigin method ) override
	{
		ptrdiff_t base = 0;
		if( method == SO_CURRENT )
			base = static_cast<ptrdiff_t>( mPosition );
		else if( method == SO_END )
			base = static_cast<ptrdiff_t>( mSize );
		const ptrdiff_t target = base + distance;
		if( target < 0 )
			return -1;
		mPosition = static_cast<size_t>( target );
		mSize = std::max( mSize, mPosition );
		return target;
	}
	ptrdiff_t GetPosition() override
	{
		return static_cast<ptrdiff_t>( mPosition );
	}
	ptrdiff_t GetSize() override
	{
		return static_cast<ptrdiff_t>( mSize );
	}
	bool LockData( void**, size_t ) override
	{
		return false;
	}
	bool UnlockData() override
	{
		return true;
	}
	const Be::ClassInfo* ClassType() const override
	{
		return nullptr;
	}
	bool QueryInterface( const Be::IID&, void**, BLUEQIOPT ) override
	{
		return false;
	}
	void Lock() override
	{
	}
	void Unlock() override
	{
	}
	long GetFlags() override
	{
		return 0;
	}
	int GetRefCount() const override
	{
		return 1;
	}
	IRoot* GetRootObject() const override
	{
		return const_cast<EmbeddedMeasureStream*>( this );
	}

protected:
	void FinalDelete() override
	{
	}

private:
	size_t mPosition = 0;
	size_t mSize = 0;
};
}

extern "C" DestinyEmbeddedSession* Destiny_CreateEmbeddedSession(
	const DestinyEmbeddedBallConfig* config,
	char* error,
	size_t errorSize )

{
	DestinyEmbeddedSessionOptions options = {};
	options.orientationPolicy = DESTINY_EMBEDDED_PIN_INITIAL;
	options.referenceFrame = DESTINY_EMBEDDED_PRIMARY_EGO;
	return Destiny_CreateEmbeddedSessionWithOptions( config, &options, error, errorSize );
}

extern "C" DestinyEmbeddedSession* Destiny_CreateEmbeddedSessionWithOptions(
	const DestinyEmbeddedBallConfig* config,
	const DestinyEmbeddedSessionOptions* options,
	char* error,
	size_t errorSize )
{
	if( error && errorSize )
		error[0] = '\0';
	if( !config || !options || !IsFinite( *config ) || !IsFinite( *options ) ||
		config->ballId == 0 || !config->isFree ||
		( options->referenceFrame == DESTINY_EMBEDDED_FIXED_OBSERVER &&
		  ( options->observerBallId == 0 || options->observerBallId == config->ballId ) ) )
	{
		SetError( error, errorSize, "Embedded Destiny requires finite session options and a free STOP ball configuration" );
		return nullptr;
	}
#if BLUE_WITH_PYTHON
	if( !Py_IsInitialized() )
	{
		SetError( error, errorSize, "Embedded Destiny requires host-initialized CPython" );
		return nullptr;
	}
#endif
	bool expected = false;
	if( !s_sessionActive.compare_exchange_strong( expected, true ) )
	{
		SetError( error, errorSize, "Only one embedded Destiny session may be active" );
		return nullptr;
	}

	auto* session = new( std::nothrow ) DestinyEmbeddedSession();
	if( !session )
	{
		s_sessionActive = false;
		SetError( error, errorSize, "Failed to allocate embedded Destiny session" );
		return nullptr;
	}
	session->previousDynamicalOrientation = g_useDynamicalOrientation;
	session->previousUseNewOrbit = g_useNewOrbit;
	session->orientationPolicy = options->orientationPolicy;
	session->referenceFrame = options->referenceFrame;
	session->warpEventCallback = options->warpEventCallback;
	session->warpEventUserData = options->warpEventUserData;
	s_activeSession = session;
	g_useDynamicalOrientation = true;
	g_useNewOrbit = options->orbitPolicy == DESTINY_EMBEDDED_ORBIT_FRONTIER_NEW;
	DestinyEmbeddedResetRuntimeCounters();

	DestinyEmbeddedRegistration registration = {};
	if( !Destiny_RegisterBlueClasses( &registration ) )
	{
		SetError( error, errorSize, "Failed to register or create the embedded Destiny Ballpark" );
		delete session;
		return nullptr;
	}
	if( !session->ballpark.CreateInstance( GetBallparkClsid() ) )
	{
		SetError( error, errorSize, "Failed to register or create the embedded Destiny Ballpark" );
		delete session;
		return nullptr;
	}

	session->ballpark->mSolarsystemID = config->solarSystemId;
	session->ballpark->SetUnitBase( 1.0f );
	Ball* created = session->ballpark->AddDynamicallyOrientedBall(
		config->ballId,
		config->mass,
		config->radius,
		config->maximumVelocity,
		config->maximumAngularVelocity,
		config->isFree,
		config->isGlobal,
		config->isMassive,
		config->isInteractive,
		config->isSpaceJunk,
		config->position[0],
		config->position[1],
		config->position[2],
		config->velocity[0],
		config->velocity[1],
		config->velocity[2],
		config->rotation[0],
		config->rotation[1],
		config->rotation[2],
		config->rotation[3],
		config->angularVelocity[0],
		config->angularVelocity[1],
		config->angularVelocity[2],
		config->agility,
		config->rotationalAgility,
		config->speedFraction );
	if( !created )
	{
		SetError( error, errorSize, "Failed to add the embedded Destiny ClientBall" );
		delete session;
		return nullptr;
	}

	session->ball = static_cast<ClientBall*>( created );
	session->ballRoot = session->ball->GetRawRoot();
	session->ballRoot->Lock();
	if( options->referenceFrame == DESTINY_EMBEDDED_FIXED_OBSERVER )
	{
		Ball* observer = session->ballpark->AddOldStyleOrientedBall(
			options->observerBallId,
			1.0,
			1.0f,
			0.0f,
			false,
			false,
			false,
			false,
			false,
			options->observerPosition[0],
			options->observerPosition[1],
			options->observerPosition[2],
			0.0,
			0.0,
			0.0,
			1.0f,
			0.0f );
		if( !observer )
		{
			SetError( error, errorSize, "Failed to add the embedded fixed observer" );
			delete session;
			return nullptr;
		}
		session->ballpark->mEgo = options->observerBallId;
		session->observerRoleId = options->observerBallId;
	}
	else
	{
		session->ballpark->mEgo = config->ballId;
	}
	session->ballpark->SetBallRotation(
		config->ballId,
		config->rotation[0],
		config->rotation[1],
		config->rotation[2],
		config->rotation[3] );
	InitializeEmbeddedPrimaryHistory( session );
	return session;
}

extern "C" DestinyEmbeddedSession* Destiny_CreateEmbeddedSessionFromFullState(
	const void* packet,
	size_t packetSize,
	const DestinyEmbeddedFullStateDescriptor* descriptor,
	const DestinyEmbeddedSessionOptions* options,
	char* error,
	size_t errorSize )
{
	// All packet and role checks happen before the single-session CAS, settings
	// mutation, Blue registration, or Ballpark allocation.
	++s_fullStateCreationAttemptCount;
	++s_fullStatePreflightAttemptCount;
	DestinyEmbeddedFullStatePreflightDiagnostics preflight = {};
	ParsedFullState parsed;
	if( !InspectEmbeddedFullStateInternal(
			packet,
			packetSize,
			descriptor,
			options,
			preflight,
			&parsed,
			error,
			errorSize ) )
	{
		++s_fullStatePreflightRejectionCount;
		return nullptr;
	}
#if BLUE_WITH_PYTHON
	if( !Py_IsInitialized() )
	{
		SetError( error, errorSize, "Embedded Destiny requires host-initialized CPython" );
		return nullptr;
	}
#endif
	bool expected = false;
	if( !s_sessionActive.compare_exchange_strong( expected, true ) )
	{
		SetError( error, errorSize, "Only one embedded Destiny session may be active" );
		return nullptr;
	}
	++s_fullStateSessionSlotAcquisitionCount;

	auto* session = new( std::nothrow ) DestinyEmbeddedSession();
	if( !session )
	{
		s_sessionActive = false;
		SetError( error, errorSize, "Failed to allocate embedded Destiny full-state session" );
		return nullptr;
	}
	++s_fullStateSessionAllocationCount;
	auto fail = [&]( const char* message ) -> DestinyEmbeddedSession* {
		SetError( error, errorSize, message );
		delete session;
		return nullptr;
	};

	session->previousDynamicalOrientation = g_useDynamicalOrientation;
	session->previousUseNewOrbit = g_useNewOrbit;
	session->orientationPolicy = options->orientationPolicy;
	session->referenceFrame = options->referenceFrame;
	session->warpEventCallback = options->warpEventCallback;
	session->warpEventUserData = options->warpEventUserData;
	++s_fullStateGlobalSettingsMutationCount;
	s_activeSession = session;
	g_useDynamicalOrientation = true;
	g_useNewOrbit = options->orbitPolicy == DESTINY_EMBEDDED_ORBIT_FRONTIER_NEW;
	DestinyEmbeddedResetRuntimeCounters();

	DestinyEmbeddedRegistration registration = {};
	if( !Destiny_RegisterBlueClasses( &registration ) )
	{
		return fail( "Failed to register the embedded Destiny Ballpark classes" );
	}
	if( !session->ballpark.CreateInstance( GetBallparkClsid() ) )
		return fail( "Failed to create the embedded Destiny Ballpark" );
	++s_fullStateBallparkAllocationCount;
	session->ballpark->mSolarsystemID = descriptor->solarSystemId;
	session->ballpark->SetUnitBase( 1.0f );
	const uint8_t* bytes = static_cast<const uint8_t*>( packet );
	EmbeddedReadStream stream( bytes + sizeof( uint8_t ), packetSize - sizeof( uint8_t ) );
	session->ballpark->DestinyEmbeddedReadFullState( IBlueStreamPtr( &stream ) );
	if( stream.GetPosition() != stream.GetSize() ||
		session->ballpark->DestinyEmbeddedBallCount() != parsed.balls.size() )
	{
		return fail( "Full-state runtime inventory did not consume the validated packet exactly" );
	}
	for( const ParsedFullStateBall& expectedBall : parsed.balls )
	{
		Ball* actual = session->ballpark->DestinyEmbeddedFindBall( expectedBall.id );
		if( !actual || static_cast<uint8_t>( actual->mMode ) != expectedBall.mode ||
			actual->isFree != ( ( expectedBall.flags & DSTBALL_ISFREE ) != 0 ) ||
			actual->isGlobal != ( ( expectedBall.flags & DSTBALL_ISGLOBAL ) != 0 ) ||
			actual->isMassive != ( ( expectedBall.flags & DSTBALL_ISMASSIVE ) != 0 ) ||
			actual->isInteractive != ( ( expectedBall.flags & DSTBALL_ISINTERACTIVE ) != 0 ) ||
			actual->isSpaceJunk != ( ( expectedBall.flags & DSTBALL_ISSPACEJUNK ) != 0 ) )
		{
			return fail( "Full-state runtime inventory diverged from preflight validation" );
		}
		session->fullStateOrder.push_back( expectedBall.id );
	}

	session->ball = static_cast<ClientBall*>(
		session->ballpark->DestinyEmbeddedFindBall( descriptor->primaryBallId ) );
	session->ballRoot = session->ball->GetRawRoot();
	session->ballRoot->Lock();
	session->ballpark->mEgo = descriptor->egoBallId;
	session->observerRoleId = descriptor->observerBallId;
	if( descriptor->fixedTargetBallId != 0 )
	{
		session->fixedTarget = session->ballpark->DestinyEmbeddedFindBall( descriptor->fixedTargetBallId );
		session->fixedTargetRoleId = descriptor->fixedTargetBallId;
	}
	for( uint32_t i = 0; i < descriptor->celestialBallCount; ++i )
	{
		session->celestials[i] =
			session->ballpark->DestinyEmbeddedFindBall( descriptor->celestialBallIds[i] );
		session->celestialRoleIds[i] = descriptor->celestialBallIds[i];
	}
	session->celestialCount = descriptor->celestialBallCount;
	InitializeEmbeddedPrimaryHistory( session );
	session->ballpark->Start();
	if( DestinyEmbeddedGetStartCount() != 1 )
		return fail( "Full-state Ballpark did not start exactly once" );
	++s_fullStateSuccessfulCreationCount;
	return session;
}

extern "C" bool Destiny_InspectEmbeddedFullState(
	const void* packet,
	size_t packetSize,
	const DestinyEmbeddedFullStateDescriptor* descriptor,
	const DestinyEmbeddedSessionOptions* options,
	DestinyEmbeddedFullStatePreflightDiagnostics* diagnostics,
	char* error,
	size_t errorSize )
{
	if( !diagnostics )
	{
		SetError( error, errorSize, "Embedded full-state inspection requires diagnostics output" );
		return false;
	}
	return InspectEmbeddedFullStateInternal(
		packet,
		packetSize,
		descriptor,
		options,
		*diagnostics,
		nullptr,
		error,
		errorSize );
}

extern "C" bool Destiny_GetEmbeddedFullStateCreationDiagnostics(
	DestinyEmbeddedFullStateCreationDiagnostics* diagnostics )
{
	if( !diagnostics )
		return false;
	*diagnostics = {};
	diagnostics->creationAttemptCount = s_fullStateCreationAttemptCount.load();
	diagnostics->preflightAttemptCount = s_fullStatePreflightAttemptCount.load();
	diagnostics->preflightRejectionCount = s_fullStatePreflightRejectionCount.load();
	diagnostics->sessionSlotAcquisitionCount = s_fullStateSessionSlotAcquisitionCount.load();
	diagnostics->sessionAllocationCount = s_fullStateSessionAllocationCount.load();
	diagnostics->globalSettingsMutationCount = s_fullStateGlobalSettingsMutationCount.load();
	diagnostics->ballparkAllocationCount = s_fullStateBallparkAllocationCount.load();
	diagnostics->successfulCreationCount = s_fullStateSuccessfulCreationCount.load();
	return true;
}

extern "C" void Destiny_DestroyEmbeddedSession( DestinyEmbeddedSession* session )
{
	delete session;
}

extern "C" bool Destiny_AdvanceEmbeddedSession( DestinyEmbeddedSession* session, Be::Time simulationTime )
{
	if( !session || simulationTime < session->lastRequestedTime )
		return false;
	const Be::Time tickInterval = static_cast<Be::Time>( session->ballpark->mTickInterval ) * 10000;
	while( simulationTime >= session->nextTickTime )
	{
		if( session->pendingCommand != 0 && session->pendingCommandTime == session->nextTickTime )
		{
			if( session->pendingCommand == 1 )
			{
				session->ballpark->GotoPoint(
					session->ball->mId,
					session->pendingTarget[0],
					session->pendingTarget[1],
					session->pendingTarget[2] );
			}
			else if( session->pendingCommand == 2 )
			{
				session->ballpark->Stop( session->ball->mId );
			}
			else if( session->pendingCommand == 3 )
			{
				session->ballpark->Orbit(
					session->ball->mId, session->pendingTargetBallId, session->pendingRange );
			}
			else if( session->pendingCommand == 4 )
			{
				session->ballpark->WarpTo(
					session->ball->mId,
					session->pendingTarget[0],
					session->pendingTarget[1],
					session->pendingTarget[2],
					session->pendingMinRange,
					session->pendingWarpFactor );
			}
			else if( session->pendingCommand == 5 )
			{
				session->ballpark->FollowBall(
					session->ball->mId, session->pendingTargetBallId, session->pendingRange );
			}
			else if( session->pendingCommand == 6 )
			{
				session->ballpark->GotoDirection(
					session->ball->mId,
					session->pendingTarget[0],
					session->pendingTarget[1],
					session->pendingTarget[2] );
			}
			session->pendingCommand = 0;
		}
		// ClientBall samples two ticks behind render time. Evolving the local fixture
		// one tick behind the explicit clock keeps the latest interpolation segment
		// aligned with that native history window without a scheduler or look-ahead.
		session->ballpark->Evolve( session->nextTickTime - tickInterval );
		if( session->ball->mMode == DSTBALL_ORBIT && session->orbitTarget )
		{
			const Vector3d relative = session->ball->mNewPos - session->orbitTarget->mNewPos;
			if( session->hasPreviousOrbitRelative )
			{
				Vector3d normal = session->previousOrbitRelative;
				normal.Cross( relative );
				const double crossLength = normal.Length();
				const double dot = session->previousOrbitRelative * relative;
				session->orbitAccumulatedPhase += std::atan2( crossLength, dot );
			}
			session->previousOrbitRelative = relative;
			session->hasPreviousOrbitRelative = true;
		}
		if( session->orientationPolicy == DESTINY_EMBEDDED_PIN_INITIAL )
		{
			// Preserve the PL-10 null fixture while native-orientation sessions expose
			// Destiny's roll and steering behavior without this compatibility pin.
			session->ballpark->SetBallRotation(
				session->ball->mId,
				session->authoredRotation.x,
				session->authoredRotation.y,
				session->authoredRotation.z,
				session->authoredRotation.w );
			session->ball->mLastRot = session->authoredRotation;
			session->ball->mLastAngVel = Vector3( 0.0f, 0.0f, 0.0f );
			session->ball->mTorque = Vector3( 0.0f, 0.0f, 0.0f );
			session->ball->mRoll = 0.0f;
			session->ball->mRotUpdateTime = 0;
			++session->orientationPinCount;
		}
		++session->directEvolveCount;
		session->nextTickTime += tickInterval;
	}
	session->lastRequestedTime = simulationTime;
	return true;
}

extern "C" bool Destiny_AddEmbeddedFixedTarget(
	DestinyEmbeddedSession* session,
	const DestinyEmbeddedFixedTargetConfig* config,
	char* error,
	size_t errorSize )
{
	if( error && errorSize )
		error[0] = '\0';
	if( !session || !config || session->fixedTarget || session->directEvolveCount != 0 ||
		session->commandCount != 0 || config->ballId == 0 || config->ballId == session->ball->mId ||
		config->ballId == session->ballpark->mEgo || !std::isfinite( config->radius ) || config->radius <= 0.0f ||
		!std::isfinite( config->position[0] ) || !std::isfinite( config->position[1] ) ||
		!std::isfinite( config->position[2] ) )
	{
		SetError( error, errorSize, "Embedded Destiny requires one finite fixed target before advance or commands" );
		return false;
	}
	session->fixedTarget = session->ballpark->AddOldStyleOrientedBall(
		config->ballId, 1.0, config->radius, 0.0f, false, false, false, false, false,
		config->position[0], config->position[1], config->position[2], 0.0, 0.0, 0.0, 1.0f, 0.0f );
	if( !session->fixedTarget )
	{
		SetError( error, errorSize, "Failed to add embedded Destiny fixed target" );
		return false;
	}
	session->fixedTargetRoleId = config->ballId;
	return true;
}

extern "C" bool Destiny_AddEmbeddedCelestial(
	DestinyEmbeddedSession* session,
	const DestinyEmbeddedCelestialConfig* config,
	char* error,
	size_t errorSize )
{
	if( error && errorSize )
		error[0] = '\0';
	bool duplicate = false;
	if( session && config )
	{
		for( size_t i = 0; i < session->celestialCount; ++i )
			if( session->celestialRoleIds[i] == config->ballId )
				duplicate = true;
	}
	if( !session || !config || duplicate || session->directEvolveCount != 0 ||
		session->commandCount != 0 || session->celestialCount >= kMaxCelestialBalls ||
		config->ballId == 0 || config->ballId == session->ball->mId ||
		config->ballId == session->ballpark->mEgo ||
		( session->fixedTarget && config->ballId == session->fixedTarget->mId ) ||
		!std::isfinite( config->radius ) || config->radius <= 0.0f ||
		!std::isfinite( config->position[0] ) || !std::isfinite( config->position[1] ) ||
		!std::isfinite( config->position[2] ) )
	{
		SetError( error, errorSize, "Embedded Destiny requires finite unique celestials before advance or commands" );
		return false;
	}
	Ball* celestial = session->ballpark->AddOldStyleOrientedBall(
		config->ballId, 1.0, config->radius, 0.0f, false, true, false, false, false,
		config->position[0], config->position[1], config->position[2], 0.0, 0.0, 0.0, 1.0f, 0.0f );
	if( !celestial )
	{
		SetError( error, errorSize, "Failed to add embedded Destiny celestial ball" );
		return false;
	}
	session->ballpark->SetBallRigid( config->ballId );
	session->celestials[session->celestialCount] = celestial;
	session->celestialRoleIds[session->celestialCount] = config->ballId;
	++session->celestialCount;
	return true;
}

extern "C" ITriVectorFunction* Destiny_GetEmbeddedCelestialPosition(
	DestinyEmbeddedSession* session,
	int64_t ballId )
{
	return IsCelestialRole( session, ballId ) ? Destiny_GetEmbeddedBallPosition( session, ballId ) : nullptr;
}

extern "C" bool Destiny_GetEmbeddedCelestialState(
	DestinyEmbeddedSession* session,
	int64_t ballId,
	DestinyEmbeddedCelestialState* state )
{
	if( !session || !state || !IsCelestialRole( session, ballId ) )
		return false;
	Ball* celestial = FindEmbeddedBall( session, ballId );
	if( !celestial )
		return false;
	*state = {};
	state->ballId = celestial->mId;
	state->mode = static_cast<int32_t>( celestial->mMode );
	state->isFree = celestial->isFree;
	state->isGlobal = celestial->isGlobal;
	state->isMassive = celestial->isMassive;
	state->isInteractive = celestial->isInteractive;
	state->radius = celestial->mRadius;
	for( size_t axis = 0; axis < 3; ++axis )
	{
		state->position[axis] = ( &celestial->mNewPos.x )[axis];
		state->velocity[axis] = ( &celestial->mNewVel.x )[axis];
	}
	return true;
}

extern "C" bool Destiny_CommandEmbeddedGoto(
	DestinyEmbeddedSession* session,
	Be::Time effectiveTime,
	const double target[3] )
{
	if( !session || !target || effectiveTime != session->nextTickTime ||
		effectiveTime < session->lastRequestedTime ||
		( session->commandCount != 0 && effectiveTime <= session->lastCommandTime ) ||
		!std::isfinite( target[0] ) ||
		!std::isfinite( target[1] ) || !std::isfinite( target[2] ) )
	{
		return false;
	}
	session->pendingCommand = 1;
	session->pendingCommandTime = effectiveTime;
	for( size_t i = 0; i < 3; ++i )
		session->pendingTarget[i] = target[i];
	session->lastCommandTime = effectiveTime;
	++session->commandCount;
	return true;
}

extern "C" bool Destiny_CommandEmbeddedGotoDirection(
	DestinyEmbeddedSession* session,
	Be::Time effectiveTime,
	const double direction[3] )
{
	if( !session || !direction || effectiveTime != session->nextTickTime ||
		effectiveTime < session->lastRequestedTime ||
		( session->commandCount != 0 && effectiveTime <= session->lastCommandTime ) ||
		!std::isfinite( direction[0] ) || !std::isfinite( direction[1] ) ||
		!std::isfinite( direction[2] ) )
	{
		return false;
	}
	const double lengthSquared =
		direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2];
	if( lengthSquared <= 0.0 )
		return false;
	session->pendingCommand = 6;
	session->pendingCommandTime = effectiveTime;
	for( size_t i = 0; i < 3; ++i )
		session->pendingTarget[i] = direction[i];
	session->lastCommandTime = effectiveTime;
	++session->commandCount;
	return true;
}

extern "C" bool Destiny_CommandEmbeddedStop( DestinyEmbeddedSession* session, Be::Time effectiveTime )
{
	if( !session || effectiveTime != session->nextTickTime || effectiveTime < session->lastRequestedTime ||
		( session->commandCount != 0 && effectiveTime <= session->lastCommandTime ) )
		return false;
	session->pendingCommand = 2;
	session->pendingCommandTime = effectiveTime;
	session->lastCommandTime = effectiveTime;
	++session->commandCount;
	return true;
}

extern "C" bool Destiny_CommandEmbeddedOrbit(
	DestinyEmbeddedSession* session,
	Be::Time effectiveTime,
	int64_t targetBallId,
	float surfaceRange )
{
	Ball* target = ResolveCommandTarget( session, targetBallId, true, true );
	if( !session || !target || targetBallId == session->ball->mId || targetBallId == session->ballpark->mEgo ||
		effectiveTime != session->nextTickTime || effectiveTime < session->lastRequestedTime ||
		( session->commandCount != 0 && effectiveTime <= session->lastCommandTime ) ||
		!std::isfinite( surfaceRange ) || surfaceRange < 0.0f )
	{
		return false;
	}
	session->pendingCommand = 3;
	session->pendingCommandTime = effectiveTime;
	session->pendingTargetBallId = targetBallId;
	session->pendingRange = surfaceRange;
	session->orbitTarget = target;
	session->lastCommandTime = effectiveTime;
	++session->commandCount;
	return true;
}

extern "C" bool Destiny_CommandEmbeddedWarp(
	DestinyEmbeddedSession* session,
	Be::Time effectiveTime,
	const double target[3],
	double minimumRange,
	int32_t warpFactor )
{
	if( !session || !target || effectiveTime != session->nextTickTime ||
		effectiveTime < session->lastRequestedTime ||
		( session->commandCount != 0 && effectiveTime <= session->lastCommandTime ) ||
		!std::isfinite( target[0] ) || !std::isfinite( target[1] ) || !std::isfinite( target[2] ) ||
		!std::isfinite( minimumRange ) || minimumRange < 0.0 || warpFactor < 1 )
	{
		return false;
	}
	// The sim silently downgrades sub-100 km warps to GotoPoint
	// (Ballpark::WarpTo); the embedded contract rejects that zone outright,
	// with margin, so a warp command always means a warp. Evaluated against
	// the ball's current position at issue time.
	const double offset[3] = {
		target[0] - session->ball->mNewPos.x,
		target[1] - session->ball->mNewPos.y,
		target[2] - session->ball->mNewPos.z,
	};
	const double distanceSquared =
		offset[0] * offset[0] + offset[1] * offset[1] + offset[2] * offset[2];
	if( distanceSquared < 4e10 ) // (200 km)^2
	{
		return false;
	}
	session->pendingCommand = 4;
	session->pendingCommandTime = effectiveTime;
	for( size_t i = 0; i < 3; ++i )
		session->pendingTarget[i] = target[i];
	session->pendingMinRange = minimumRange;
	session->pendingWarpFactor = warpFactor;
	session->lastCommandTime = effectiveTime;
	++session->commandCount;
	return true;
}

extern "C" bool Destiny_CommandEmbeddedFollow(
	DestinyEmbeddedSession* session,
	Be::Time effectiveTime,
	int64_t targetBallId,
	float surfaceRange )
{
	// The client's Approach and Keep-at-Range both issue FollowBall; the sim
	// only rejects a NaN range (Ballpark::FollowBall), so the embedded
	// contract enforces finite non-negative ranges and a live fixed target
	// itself, mirroring the orbit seam.
	Ball* target = ResolveCommandTarget( session, targetBallId, true, false );
	if( !session || !target ||
		targetBallId == session->ball->mId || targetBallId == session->ballpark->mEgo ||
		effectiveTime != session->nextTickTime || effectiveTime < session->lastRequestedTime ||
		( session->commandCount != 0 && effectiveTime <= session->lastCommandTime ) ||
		!std::isfinite( surfaceRange ) || surfaceRange < 0.0f )
	{
		return false;
	}
	session->pendingCommand = 5;
	session->pendingCommandTime = effectiveTime;
	session->pendingTargetBallId = targetBallId;
	session->pendingRange = surfaceRange;
	session->lastCommandTime = effectiveTime;
	++session->commandCount;
	return true;
}

extern "C" IEveBallpark* Destiny_GetEmbeddedBallpark( DestinyEmbeddedSession* session )
{
	return session ? static_cast<IEveBallpark*>( session->ballpark.p ) : nullptr;
}

extern "C" ITriVectorFunction* Destiny_GetEmbeddedPosition( DestinyEmbeddedSession* session )
{
	return session ? static_cast<ITriVectorFunction*>( session->ball ) : nullptr;
}

extern "C" ITriQuaternionFunction* Destiny_GetEmbeddedRotation( DestinyEmbeddedSession* session )
{
	return session ? static_cast<ITriQuaternionFunction*>( session->ball ) : nullptr;
}

extern "C" ITriVectorFunction* Destiny_GetEmbeddedBallPosition(
	DestinyEmbeddedSession* session,
	int64_t ballId )
{
	Ball* ball = FindEmbeddedBall( session, ballId );
	return ball ? static_cast<ITriVectorFunction*>( static_cast<ClientBall*>( ball ) ) : nullptr;
}

extern "C" ITriQuaternionFunction* Destiny_GetEmbeddedBallRotation(
	DestinyEmbeddedSession* session,
	int64_t ballId )
{
	Ball* ball = FindEmbeddedBall( session, ballId );
	return ball ? static_cast<ITriQuaternionFunction*>( static_cast<ClientBall*>( ball ) ) : nullptr;
}

extern "C" bool Destiny_GetEmbeddedDiagnostics(
	DestinyEmbeddedSession* session,
	DestinyEmbeddedDiagnostics* diagnostics )
{
	if( !session || !diagnostics )
		return false;
	*diagnostics = {};
	diagnostics->directEvolveCount = session->directEvolveCount;
	diagnostics->startCallCount = DestinyEmbeddedGetStartCount();
	diagnostics->onTickCallCount = DestinyEmbeddedGetOnTickCount();
	diagnostics->pythonCallbackCount = DestinyEmbeddedGetPythonCallbackCount();
	diagnostics->lastRequestedTime = session->lastRequestedTime;
	diagnostics->nextTickTime = session->nextTickTime;
	diagnostics->mode = static_cast<int32_t>( session->ball->mMode );
	diagnostics->schedulerRegistered = session->ballpark->DestinyEmbeddedHasRegisteredTicks();
	diagnostics->dynamicalOrientationEnabled = g_useDynamicalOrientation;
	diagnostics->frontierOrbitEnabled = g_useNewOrbit;
	diagnostics->primaryBallId = session->ball->mId;
	diagnostics->egoBallId = session->ballpark->mEgo;
	diagnostics->observerBallId = session->observerRoleId;
	diagnostics->commandCount = session->commandCount;
	diagnostics->orientationPinCount = session->orientationPinCount;
	diagnostics->lastCommandTime = session->lastCommandTime;

	const Vector3d& rawPosition = session->ball->mNewPos;
	const Vector3d& rawVelocity = session->ball->mNewVel;
	const Vector3d rawAcceleration = session->ball->mLastG + session->ball->mLastC;
	Vector3d absolutePosition;
	Vector3d referencePoint;
	session->ball->InterpolatedPosition( &absolutePosition, session->lastRequestedTime );
	session->ballpark->GetReferencePoint( &referencePoint, session->lastRequestedTime );

	const Vector3 position = ( absolutePosition - referencePoint ).AsVector3();
	const Vector3 velocity = session->ball->mLastVel.AsVector3();
	Vector3 acceleration;
	Quaternion rotation;
	session->ball->GetValueDoubleDotAt( &acceleration, session->lastRequestedTime );
	session->ball->GetValueAt( &rotation, session->lastRequestedTime );
	for( size_t i = 0; i < 3; ++i )
	{
		diagnostics->rawPosition[i] = ( &rawPosition.x )[i];
		diagnostics->rawVelocity[i] = ( &rawVelocity.x )[i];
		diagnostics->rawAcceleration[i] = ( &rawAcceleration.x )[i];
		diagnostics->absolutePosition[i] = ( &absolutePosition.x )[i];
		diagnostics->referencePoint[i] = ( &referencePoint.x )[i];
		diagnostics->position[i] = ( &position.x )[i];
		diagnostics->velocity[i] = ( &velocity.x )[i];
		diagnostics->acceleration[i] = ( &acceleration.x )[i];
		diagnostics->angularVelocity[i] = ( &session->ball->mLastAngVel.x )[i];
		diagnostics->gotoPoint[i] = ( &session->ball->mGoto.x )[i];
	}
	diagnostics->rotation[0] = rotation.x;
	diagnostics->rotation[1] = rotation.y;
	diagnostics->rotation[2] = rotation.z;
	diagnostics->rotation[3] = rotation.w;
	diagnostics->followBallId = session->ball->mFollowId;
	diagnostics->followRange = session->ball->mFollowRange;
	diagnostics->orbitAccumulatedPhase = session->orbitAccumulatedPhase;
	diagnostics->warpEffectStamp = 0;
	diagnostics->warpFactor = 0;
	diagnostics->warpMinRange = 0.0;
	diagnostics->isMassive = session->ball->isMassive;
	diagnostics->sensorActive = session->ball->mSensor.active;
	if( session->ball->mMode == DSTBALL_WARP )
	{
		// WarpTo shanghais mFollowId to hold the minimum range as bit-punned
		// double and mOwnerId to hold the warp factor; unpun them here so
		// consumers never see the raw reuse.
		diagnostics->warpEffectStamp = session->ball->mEffectStamp;
		diagnostics->warpFactor = static_cast<int32_t>( session->ball->mOwnerId );
		double minimumRange = 0.0;
		const int64_t punned = session->ball->mFollowId;
		std::memcpy( &minimumRange, &punned, sizeof( minimumRange ) );
		diagnostics->warpMinRange = minimumRange;
		// mLastCollision is repurposed as the total warp length once warp
		// proper begins (RealWarp); zero while still aligning.
		diagnostics->warpTotalDistance =
			session->ball->mEffectStamp >= 0 ? session->ball->mLastCollision : 0.0;
		diagnostics->followBallId = 0;
	}
	if( session->orbitTarget )
	{
		diagnostics->orbitTargetBallId = session->orbitTarget->mId;
		const Vector3d relative = rawPosition - session->orbitTarget->mNewPos;
		const double distance = relative.Length();
		diagnostics->orbitCenterDistance = distance;
		diagnostics->orbitSurfaceDistance = distance - session->ball->mRadius - session->orbitTarget->mRadius;
		Vector3d radial = distance > 0.0 ? relative / distance : Vector3d();
		diagnostics->orbitRadialVelocity = rawVelocity * radial;
		const double speedSquared = rawVelocity.LengthSq();
		diagnostics->orbitTangentialVelocity = std::sqrt( std::max(
			0.0, speedSquared - diagnostics->orbitRadialVelocity * diagnostics->orbitRadialVelocity ) );
		Vector3d normal = relative;
		normal.Cross( rawVelocity );
		if( normal.LengthSq() > 0.0 )
			normal.Normalize();
		for( size_t i = 0; i < 3; ++i )
		{
			diagnostics->orbitTargetPosition[i] = ( &session->orbitTarget->mNewPos.x )[i];
			diagnostics->orbitTargetVelocity[i] = ( &session->orbitTarget->mNewVel.x )[i];
			diagnostics->orbitNormal[i] = ( &normal.x )[i];
		}
	}
	return true;
}

extern "C"
{

	__attribute__( ( visibility( "default" ) ) ) bool Destiny_WriteEmbeddedFullState(
		DestinyEmbeddedSession* session,
		void* buffer,
		size_t bufferSize,
		size_t* bytesWritten )
	{
		if( !session || !buffer || !bytesWritten )
			return false;
		IEveBallpark* eve = Destiny_GetEmbeddedBallpark( session );
		if( !eve )
			return false;
		EmbeddedByteStream stream( buffer, bufferSize );
		Ballpark* ballpark = static_cast<Ballpark*>( eve );
		const size_t written = session->fullStateOrder.empty() ?
			ballpark->DestinyEmbeddedWriteFullState( IBlueStreamPtr( &stream ) ) :
			ballpark->DestinyEmbeddedWriteFullStateOrdered(
				IBlueStreamPtr( &stream ), session->fullStateOrder.data(), session->fullStateOrder.size() );
		*bytesWritten = written;
		return written > 0 && !stream.Overflowed();
	}

	__attribute__( ( visibility( "default" ) ) ) bool Destiny_MeasureEmbeddedFullState(
		DestinyEmbeddedSession* session,
		size_t* bytesRequired )
	{
		if( !session || !bytesRequired )
			return false;
		IEveBallpark* eve = Destiny_GetEmbeddedBallpark( session );
		if( !eve )
			return false;
		EmbeddedMeasureStream stream;
		Ballpark* ballpark = static_cast<Ballpark*>( eve );
		const size_t measured = session->fullStateOrder.empty() ?
			ballpark->DestinyEmbeddedWriteFullState( IBlueStreamPtr( &stream ) ) :
			ballpark->DestinyEmbeddedWriteFullStateOrdered(
				IBlueStreamPtr( &stream ), session->fullStateOrder.data(), session->fullStateOrder.size() );
		*bytesRequired = measured;
		return measured > 0 && measured == static_cast<size_t>( stream.GetSize() );
	}
}
