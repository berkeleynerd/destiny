// Copyright (c) 2026 CCP Games

#include <DestinyEmbedded.h>

extern "C"
{
int Py_IsInitialized();
void Py_Initialize();
void BlueModuleStartup();
}

#define DESTINY_DEFINE_HOST_INTERFACE( _interface ) \
	const Be::IID& Get##_interface##IID() \
	{ \
		static Be::IID s_iid( #_interface ); \
		return s_iid; \
	} \
	template<> const Be::IID& BlueInterfaceIID<_interface>() \
	{ \
		return Get##_interface##IID(); \
	}

DESTINY_DEFINE_HOST_INTERFACE( ITriVectorFunction );
DESTINY_DEFINE_HOST_INTERFACE( ITriQuaternionFunction );
DESTINY_DEFINE_HOST_INTERFACE( IEveReferencePoint );
DESTINY_DEFINE_HOST_INTERFACE( IEveBallpark );

namespace
{
void OnWarpEvent( int, int64_t, int64_t, void* )
{
}
}

int main()
{
	if( !Py_IsInitialized() )
		Py_Initialize();
	BlueModuleStartup();

	DestinyEmbeddedBallConfig config = {};
	config.ballId = 1;
	config.solarSystemId = 30005286;
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

	char error[256] = {};
	DestinyEmbeddedSession* session = Destiny_CreateEmbeddedSession(
		&config, error, sizeof( error ) );
	if( !session || !Destiny_SetEmbeddedWarpEventCallback( session, OnWarpEvent, nullptr ) )
		return 1;
	Destiny_DestroyEmbeddedSession( session );
	return 0;
}
