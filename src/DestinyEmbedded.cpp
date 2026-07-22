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

namespace
{
constexpr Be::Time kTicksPerSecond = 10000000;
std::atomic<uint64_t> s_startCallCount = 0;
std::atomic<uint64_t> s_onTickCallCount = 0;
std::atomic<uint64_t> s_pythonCallbackCount = 0;
std::atomic<bool> s_sessionActive = false;

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
		options.referenceFrame > DESTINY_EMBEDDED_FIXED_OBSERVER )
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
	IRoot* ballRoot = nullptr;
	Be::Time lastRequestedTime = 0;
	Be::Time nextTickTime = kTicksPerSecond;
	uint64_t directEvolveCount = 0;
	uint64_t commandCount = 0;
	uint64_t orientationPinCount = 0;
	bool previousDynamicalOrientation = false;
	DestinyEmbeddedOrientationPolicy orientationPolicy = DESTINY_EMBEDDED_PIN_INITIAL;
	DestinyEmbeddedReferenceFrame referenceFrame = DESTINY_EMBEDDED_PRIMARY_EGO;
	Be::Time lastCommandTime = 0;
	Be::Time pendingCommandTime = 0;
	int pendingCommand = 0;
	double pendingTarget[3] = {};
	Quaternion authoredRotation = Quaternion( 0.0f, 0.0f, 0.0f, 1.0f );

	~DestinyEmbeddedSession()
	{
		ballpark.Unlock();
		if( ballRoot )
			ballRoot->Unlock();
		g_useDynamicalOrientation = previousDynamicalOrientation;
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
	session->orientationPolicy = options->orientationPolicy;
	session->referenceFrame = options->referenceFrame;
	g_useDynamicalOrientation = true;
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
			else
			{
				session->ballpark->Stop( session->ball->mId );
			}
			session->pendingCommand = 0;
		}
		// ClientBall samples two ticks behind render time. Evolving the local fixture
		// one tick behind the explicit clock keeps the latest interpolation segment
		// aligned with that native history window without a scheduler or look-ahead.
		session->ballpark->Evolve( session->nextTickTime - tickInterval );
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
	return true;
}
