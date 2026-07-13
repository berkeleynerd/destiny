// Copyright (c) 2026 CCP Games

#pragma once

#include "IEveBallpark.h"

#include <ITriFunction.h>

#include <cstddef>
#include <cstdint>

#if defined( __GNUC__ )
#define DESTINY_EMBEDDED_API __attribute__( ( visibility( "default" ) ) )
#else
#define DESTINY_EMBEDDED_API
#endif

struct DestinyEmbeddedSession;

enum DestinyEmbeddedOrientationPolicy
{
	DESTINY_EMBEDDED_PIN_INITIAL = 0,
	DESTINY_EMBEDDED_NATIVE_ORIENTATION = 1,
};

enum DestinyEmbeddedReferenceFrame
{
	DESTINY_EMBEDDED_PRIMARY_EGO = 0,
	DESTINY_EMBEDDED_FIXED_OBSERVER = 1,
};

enum DestinyEmbeddedOrbitPolicy
{
	DESTINY_EMBEDDED_ORBIT_CHECKOUT_DEFAULT = 0,
	DESTINY_EMBEDDED_ORBIT_FRONTIER_NEW = 1,
};

// Mirrors DSTBALLMODE for embedded consumers that must not include destiny internals.
enum DestinyEmbeddedBallMode
{
	DESTINY_EMBEDDED_BALL_MODE_GOTO = 0,
	DESTINY_EMBEDDED_BALL_MODE_FOLLOW = 1,
	DESTINY_EMBEDDED_BALL_MODE_STOP = 2,
	DESTINY_EMBEDDED_BALL_MODE_WARP = 3,
	DESTINY_EMBEDDED_BALL_MODE_ORBIT = 4,
	DESTINY_EMBEDDED_BALL_MODE_RIGID = 11,
};

struct DestinyEmbeddedRegistration
{
	uint32_t discoveredClassCount;
	uint32_t newlyRegisteredClassCount;
	bool alreadyRegistered;
	bool valid;
};

struct DestinyEmbeddedBallConfig
{
	int64_t ballId;
	int64_t solarSystemId;
	double mass;
	float radius;
	float maximumVelocity;
	float maximumAngularVelocity;
	double position[3];
	double velocity[3];
	float rotation[4];
	float angularVelocity[3];
	float agility;
	float rotationalAgility;
	float speedFraction;
	bool isFree;
	bool isGlobal;
	bool isMassive;
	bool isInteractive;
	bool isSpaceJunk;
};

struct DestinyEmbeddedSessionOptions
{
	DestinyEmbeddedOrientationPolicy orientationPolicy;
	DestinyEmbeddedReferenceFrame referenceFrame;
	DestinyEmbeddedOrbitPolicy orbitPolicy;
	int64_t observerBallId;
	double observerPosition[3];
};

struct DestinyEmbeddedFixedTargetConfig
{
	int64_t ballId;
	float radius;
	double position[3];
};

struct DestinyEmbeddedCelestialConfig
{
	int64_t ballId;
	float radius;
	double position[3];
};

struct DestinyEmbeddedCelestialState
{
	int64_t ballId;
	int32_t mode;
	bool isFree;
	bool isGlobal;
	bool isMassive;
	bool isInteractive;
	float radius;
	double position[3];
	double velocity[3];
};

struct DestinyEmbeddedDiagnostics
{
	uint64_t directEvolveCount;
	uint64_t startCallCount;
	uint64_t onTickCallCount;
	uint64_t pythonCallbackCount;
	Be::Time lastRequestedTime;
	Be::Time nextTickTime;
	int32_t mode;
	bool schedulerRegistered;
	bool dynamicalOrientationEnabled;
	bool frontierOrbitEnabled;
	int64_t primaryBallId;
	int64_t egoBallId;
	uint64_t commandCount;
	uint64_t orientationPinCount;
	Be::Time lastCommandTime;
	double rawPosition[3];
	double rawVelocity[3];
	double rawAcceleration[3];
	double absolutePosition[3];
	double referencePoint[3];
	double position[3];
	double velocity[3];
	double acceleration[3];
	float rotation[4];
	float angularVelocity[3];
	double gotoPoint[3];
	int64_t orbitTargetBallId;
	int64_t followBallId;
	float followRange;
	double orbitTargetPosition[3];
	double orbitTargetVelocity[3];
	double orbitCenterDistance;
	double orbitSurfaceDistance;
	double orbitRadialVelocity;
	double orbitTangentialVelocity;
	double orbitNormal[3];
	double orbitAccumulatedPhase;
	// Warp state (valid while mode is DSTBALL_WARP, zero otherwise). The
	// effect stamp is negative while aligning and holds the warp-start tick
	// once warp proper begins; factor and minimum range echo the command
	// inputs the sim stores in its shanghaied ball members.
	int64_t warpEffectStamp;
	int32_t warpFactor;
	double warpMinRange;
	double warpTotalDistance;
	// Collision/sensor participation of the primary ball; warp suspends both
	// and dropout must restore them.
	bool isMassive;
	bool sensorActive;
};

extern "C"
{
	DESTINY_EMBEDDED_API bool Destiny_RegisterBlueClasses( DestinyEmbeddedRegistration* registration );
	DESTINY_EMBEDDED_API DestinyEmbeddedSession* Destiny_CreateEmbeddedSession(
		const DestinyEmbeddedBallConfig* config,
		char* error,
		size_t errorSize );
	DESTINY_EMBEDDED_API DestinyEmbeddedSession* Destiny_CreateEmbeddedSessionWithOptions(
		const DestinyEmbeddedBallConfig* config,
		const DestinyEmbeddedSessionOptions* options,
		char* error,
		size_t errorSize );
	DESTINY_EMBEDDED_API void Destiny_DestroyEmbeddedSession( DestinyEmbeddedSession* session );
	DESTINY_EMBEDDED_API bool Destiny_AdvanceEmbeddedSession( DestinyEmbeddedSession* session, Be::Time simulationTime );
	DESTINY_EMBEDDED_API bool Destiny_AddEmbeddedFixedTarget(
		DestinyEmbeddedSession* session,
		const DestinyEmbeddedFixedTargetConfig* config,
		char* error,
		size_t errorSize );
	DESTINY_EMBEDDED_API bool Destiny_AddEmbeddedCelestial(
		DestinyEmbeddedSession* session,
		const DestinyEmbeddedCelestialConfig* config,
		char* error,
		size_t errorSize );
	DESTINY_EMBEDDED_API ITriVectorFunction* Destiny_GetEmbeddedCelestialPosition(
		DestinyEmbeddedSession* session,
		int64_t ballId );
	DESTINY_EMBEDDED_API bool Destiny_GetEmbeddedCelestialState(
		DestinyEmbeddedSession* session,
		int64_t ballId,
		DestinyEmbeddedCelestialState* state );
	DESTINY_EMBEDDED_API bool Destiny_CommandEmbeddedGoto(
		DestinyEmbeddedSession* session,
		Be::Time effectiveTime,
		const double target[3] );
	DESTINY_EMBEDDED_API bool Destiny_CommandEmbeddedGotoDirection(
		DestinyEmbeddedSession* session,
		Be::Time effectiveTime,
		const double direction[3] );
	DESTINY_EMBEDDED_API bool Destiny_CommandEmbeddedStop(
		DestinyEmbeddedSession* session,
		Be::Time effectiveTime );
	DESTINY_EMBEDDED_API bool Destiny_CommandEmbeddedOrbit(
		DestinyEmbeddedSession* session,
		Be::Time effectiveTime,
		int64_t targetBallId,
		float surfaceRange );
	DESTINY_EMBEDDED_API bool Destiny_CommandEmbeddedWarp(
		DestinyEmbeddedSession* session,
		Be::Time effectiveTime,
		const double target[3],
		double minimumRange,
		int32_t warpFactor );
	DESTINY_EMBEDDED_API bool Destiny_CommandEmbeddedFollow(
		DestinyEmbeddedSession* session,
		Be::Time effectiveTime,
		int64_t targetBallId,
		float surfaceRange );
	DESTINY_EMBEDDED_API IEveBallpark* Destiny_GetEmbeddedBallpark( DestinyEmbeddedSession* session );
	DESTINY_EMBEDDED_API ITriVectorFunction* Destiny_GetEmbeddedPosition( DestinyEmbeddedSession* session );
	DESTINY_EMBEDDED_API ITriQuaternionFunction* Destiny_GetEmbeddedRotation( DestinyEmbeddedSession* session );
	DESTINY_EMBEDDED_API bool Destiny_GetEmbeddedDiagnostics(
		DestinyEmbeddedSession* session,
		DestinyEmbeddedDiagnostics* diagnostics );
}

#undef DESTINY_EMBEDDED_API
