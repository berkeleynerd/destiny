// Copyright © 2023 CCP ehf.

#include "Settings.h"


extern size_t g_collisionMaxIterations = g_collisionMaxIterationsDefault;
extern bool g_useIterativeCollision = g_useIterativeCollisionDefault;
extern bool g_useDynamicalOrientation = g_useDynamicalOrientationDefault;
extern bool g_disableDynamicalOrientationForMissiles = g_disableDynamicalOrientationForMissilesDefault;
extern bool g_useNewOrbit = g_useNewOrbitDefault;
extern GlobalSettings* s_globalSettings = new OGlobalSettings();

PyObject* GlobalSettings::ApplyConfiguration( PyObject* args )
{
	PyObject* pyConfig;
	if( !PyArg_ParseTuple( args, "O", &pyConfig ) )
	{
		return nullptr;
	}
	SettingsConfigurationPtr config(pyConfig);
	if( !config )
	{
		PyErr_SetString( PyExc_TypeError, "Argument 1 is not a SettingsConfiguration" );
		return nullptr;
	}

	g_collisionMaxIterations = config->m_collisionMaxIterations;
	g_useIterativeCollision = config->m_useIterativeCollision;
	g_useDynamicalOrientation = config->m_useDynamicalOrientation;
	g_disableDynamicalOrientationForMissiles = config->m_disableDynamicalOrientationForMissiles;
	g_useNewOrbit = config->m_useNewOrbit;
	Py_RETURN_NONE;
}

PyObject* GlobalSettings::GetConfiguration( PyObject* args )
{
	SettingsConfigurationPtr config = new OSettingsConfiguration();
	config->m_collisionMaxIterations = g_collisionMaxIterations;
	config->m_useIterativeCollision = g_useIterativeCollision;
	config->m_useDynamicalOrientation = g_useDynamicalOrientation;
	config->m_disableDynamicalOrientationForMissiles = g_disableDynamicalOrientationForMissiles;
	config->m_useNewOrbit = g_useNewOrbit;
	return BlueWrapObjectForPython(config);
}

PyObject* GlobalSettings::GetDefaultConfiguration( PyObject* args )
{
	SettingsConfigurationPtr config = new OSettingsConfiguration();
	config->m_collisionMaxIterations = g_collisionMaxIterationsDefault;
	config->m_useIterativeCollision = g_useIterativeCollisionDefault;
	config->m_useDynamicalOrientation = g_useDynamicalOrientationDefault;
	config->m_disableDynamicalOrientationForMissiles = g_disableDynamicalOrientationForMissilesDefault;
	config->m_useNewOrbit = g_useNewOrbitDefault;
	return BlueWrapObjectForPython( config );
}

PyObject* GlobalSettings::ResetConfiguration( PyObject* args )
{
	g_collisionMaxIterations = g_collisionMaxIterationsDefault;
	g_useIterativeCollision = g_useIterativeCollisionDefault;
	g_useDynamicalOrientation = g_useDynamicalOrientationDefault;
	g_disableDynamicalOrientationForMissiles = g_disableDynamicalOrientationForMissilesDefault;
	g_useNewOrbit = g_useNewOrbitDefault;
	Py_RETURN_NONE;
}
