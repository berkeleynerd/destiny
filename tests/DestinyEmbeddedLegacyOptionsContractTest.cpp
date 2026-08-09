// Copyright (c) 2026 CCP Games

// Deliberately does not include DestinyEmbedded.h. This models a binary
// caller compiled against the pre-D06 public options layout.

#if defined( _WIN32 )
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

#include "StdAfx.h"

#include <Blue.h>
#include <IEveBallpark.h>
#include <ITriFunction.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if defined( _WIN32 )
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

const char* g_moduleName = "DestinyEmbeddedLegacyOptionsContractTest";

BLUE_DEFINE_INTERFACE( ITriVectorFunction );
BLUE_DEFINE_INTERFACE( ITriQuaternionFunction );
BLUE_DEFINE_INTERFACE( IEveReferencePoint );
BLUE_DEFINE_INTERFACE( IEveBallpark );

struct LegacyBallConfig
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

struct LegacySessionOptions
{
	int orientationPolicy;
	int referenceFrame;
	int orbitPolicy;
	int64_t observerBallId;
	double observerPosition[3];
};

struct DestinyEmbeddedSession;

extern "C" DestinyEmbeddedSession* Destiny_CreateEmbeddedSessionWithOptions(
	const LegacyBallConfig* config,
	const LegacySessionOptions* options,
	char* error,
	size_t errorSize );
extern "C" void Destiny_DestroyEmbeddedSession( DestinyEmbeddedSession* session );

namespace
{
struct GuardedOptions
{
	LegacySessionOptions* options = nullptr;
	void* allocation = nullptr;
	size_t allocationSize = 0;
};

bool AllocateGuardedOptions( GuardedOptions& guarded )
{
#if defined( _WIN32 )
	SYSTEM_INFO systemInfo = {};
	GetSystemInfo( &systemInfo );
	const size_t pageSize = systemInfo.dwPageSize;
	void* allocation = VirtualAlloc( nullptr, 2 * pageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
	if( !allocation )
		return false;
	DWORD oldProtection = 0;
	if( !VirtualProtect(
		static_cast<char*>( allocation ) + pageSize, pageSize, PAGE_NOACCESS, &oldProtection ) )
	{
		VirtualFree( allocation, 0, MEM_RELEASE );
		return false;
	}
#else
	const long systemPageSize = sysconf( _SC_PAGESIZE );
	if( systemPageSize <= 0 )
		return false;
	const size_t pageSize = static_cast<size_t>( systemPageSize );
	void* allocation = mmap(
		nullptr, 2 * pageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0 );
	if( allocation == MAP_FAILED )
		return false;
	if( mprotect( static_cast<char*>( allocation ) + pageSize, pageSize, PROT_NONE ) != 0 )
	{
		munmap( allocation, 2 * pageSize );
		return false;
	}
#endif
	guarded.options = reinterpret_cast<LegacySessionOptions*>(
		static_cast<char*>( allocation ) + pageSize - sizeof( LegacySessionOptions ) );
	guarded.allocation = allocation;
	guarded.allocationSize = 2 * pageSize;
	std::memset( guarded.options, 0, sizeof( *guarded.options ) );
	return true;
}

void FreeGuardedOptions( GuardedOptions& guarded )
{
#if defined( _WIN32 )
	VirtualFree( guarded.allocation, 0, MEM_RELEASE );
#else
	munmap( guarded.allocation, guarded.allocationSize );
#endif
	guarded = {};
}
}

int main()
{
	static_assert( sizeof( LegacySessionOptions ) == 48, "pre-D06 options ABI changed" );
	if( !Py_IsInitialized() )
		Py_Initialize();
	BlueModuleStartup();
	if( !BeClasses )
	{
		std::fputs( "Blue startup did not initialize BeClasses\n", stderr );
		return 1;
	}
	LegacyBallConfig config = {};
	config.ballId = 1;
	config.solarSystemId = 30000142;
	config.mass = 975000.0;
	config.radius = 35.0f;
	config.maximumVelocity = 312.0f;
	config.maximumAngularVelocity = 1.0f;
	config.rotation[3] = 1.0f;
	config.agility = 2.87f;
	config.rotationalAgility = 1.0f;
	config.speedFraction = 1.0f;
	config.isFree = true;
	config.isMassive = true;
	config.isInteractive = true;
	GuardedOptions guarded = {};
	if( !AllocateGuardedOptions( guarded ) )
	{
		std::fputs( "failed to allocate guarded legacy options\n", stderr );
		return 1;
	}
	char error[512] = {};
	DestinyEmbeddedSession* session = Destiny_CreateEmbeddedSessionWithOptions(
		&config, guarded.options, error, sizeof( error ) );
	FreeGuardedOptions( guarded );
	if( !session )
	{
		std::fprintf( stderr, "legacy options create failed: %s\n", error );
		return 1;
	}
	Destiny_DestroyEmbeddedSession( session );
	std::puts( "legacy-options-size=48 create-destroy=true" );
	return 0;
}
