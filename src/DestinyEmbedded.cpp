// Copyright (c) 2026 CCP Games

#include "StdAfx.h"

#include "DestinyEmbedded.h"

#include "Ball.h"
#include "Ballpark.h"
#include "DstConstants.h"
#include "Settings.h"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <new>

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

void SetError( char* error, size_t errorSize, const char* message )
{
	if( error && errorSize )
		std::snprintf( error, errorSize, "%s", message );
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
}

struct DestinyEmbeddedSession
{
	BluePtr<Ballpark> ballpark;
	ClientBall* ball = nullptr;
	Ball* fixedTarget = nullptr;
	Ball* orbitTarget = nullptr;
	Ball* celestials[kMaxCelestialBalls] = {};
	size_t celestialCount = 0;
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
	session->authoredRotation = session->ball->mNewRot;
	session->ball->mLastRot = session->ball->mNewRot;
	session->ball->mLastSpeed = Quaternion( 0.0f, 0.0f, 0.0f, 0.0f );
	session->ball->mLastAngVel = session->ball->mNewAngVel;
	session->ball->mLastPos = session->ball->mNewPos;
	session->ball->mLastVel = session->ball->mNewVel;
	session->ball->mDeltaPos = Vector3d( 0.0, 0.0, 0.0 );
	session->ball->mLastG = Vector3d( 0.0, 0.0, 0.0 );
	session->ball->mLastC = Vector3d( 0.0, 0.0, 0.0 );
	session->ball->mTorque = Vector3( 0.0f, 0.0f, 0.0f );
	session->ball->mRoll = 0.0f;
	session->ball->mOldTime = -static_cast<Be::Time>( session->ballpark->mTickInterval ) * 10000;
	session->ball->mNewTime = 0;
	session->ball->mRotUpdateTime = 0;
	session->ball->mPosUpdateTime = 0;
	return session;
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
			if( session->celestials[i]->mId == config->ballId )
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
	++session->celestialCount;
	return true;
}

extern "C" ITriVectorFunction* Destiny_GetEmbeddedCelestialPosition(
	DestinyEmbeddedSession* session,
	int64_t ballId )
{
	if( !session )
		return nullptr;
	for( size_t i = 0; i < session->celestialCount; ++i )
	{
		if( session->celestials[i]->mId == ballId )
			return static_cast<ITriVectorFunction*>( static_cast<ClientBall*>( session->celestials[i] ) );
	}
	return nullptr;
}

extern "C" bool Destiny_GetEmbeddedCelestialState(
	DestinyEmbeddedSession* session,
	int64_t ballId,
	DestinyEmbeddedCelestialState* state )
{
	if( !session || !state )
		return false;
	for( size_t i = 0; i < session->celestialCount; ++i )
	{
		Ball* celestial = session->celestials[i];
		if( celestial->mId != ballId )
			continue;
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
	return false;
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
	Ball* target = nullptr;
	if( session )
	{
		if( session->fixedTarget && targetBallId == session->fixedTarget->mId )
			target = session->fixedTarget;
		for( size_t i = 0; !target && i < session->celestialCount; ++i )
			if( targetBallId == session->celestials[i]->mId )
				target = session->celestials[i];
	}
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
	if( !session || !session->fixedTarget || targetBallId != session->fixedTarget->mId ||
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
	const size_t written = static_cast<Ballpark*>( eve )
		->DestinyEmbeddedWriteFullState( IBlueStreamPtr( &stream ) );
	*bytesWritten = written;
	return written > 0 && !stream.Overflowed();
}
}
