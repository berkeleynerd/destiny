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
	double position[3];
	double velocity[3];
	float rotation[4];
};

extern "C"
{
	DESTINY_EMBEDDED_API bool Destiny_RegisterBlueClasses( DestinyEmbeddedRegistration* registration );
	DESTINY_EMBEDDED_API DestinyEmbeddedSession* Destiny_CreateEmbeddedSession(
		const DestinyEmbeddedBallConfig* config,
		char* error,
		size_t errorSize );
	DESTINY_EMBEDDED_API void Destiny_DestroyEmbeddedSession( DestinyEmbeddedSession* session );
	DESTINY_EMBEDDED_API bool Destiny_AdvanceEmbeddedSession( DestinyEmbeddedSession* session, Be::Time simulationTime );
	DESTINY_EMBEDDED_API IEveBallpark* Destiny_GetEmbeddedBallpark( DestinyEmbeddedSession* session );
	DESTINY_EMBEDDED_API ITriVectorFunction* Destiny_GetEmbeddedPosition( DestinyEmbeddedSession* session );
	DESTINY_EMBEDDED_API ITriQuaternionFunction* Destiny_GetEmbeddedRotation( DestinyEmbeddedSession* session );
	DESTINY_EMBEDDED_API bool Destiny_GetEmbeddedDiagnostics(
		DestinyEmbeddedSession* session,
		DestinyEmbeddedDiagnostics* diagnostics );
}

#undef DESTINY_EMBEDDED_API
