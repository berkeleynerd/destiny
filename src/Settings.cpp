#include "Settings.h"


extern size_t s_collisionMaxIterations = s_collisionMaxIterationsDefault;
extern bool s_useIterativeCollision = s_useIterativeCollisionDefault;
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

	s_collisionMaxIterations = config->m_collisionMaxIterations;
	s_useIterativeCollision = config->m_useIterativeCollision;
	Py_RETURN_NONE;
}

PyObject* GlobalSettings::GetConfiguration( PyObject* args )
{
	SettingsConfigurationPtr config = new OSettingsConfiguration();
	config->m_collisionMaxIterations = s_collisionMaxIterations;
	config->m_useIterativeCollision = s_useIterativeCollision;
	return BlueWrapObjectForPython(config);
}

PyObject* GlobalSettings::GetDefaultConfiguration( PyObject* args )
{
	SettingsConfigurationPtr config = new OSettingsConfiguration();
	config->m_collisionMaxIterations = s_collisionMaxIterationsDefault;
	config->m_useIterativeCollision = s_useIterativeCollisionDefault;
	return BlueWrapObjectForPython( config );
}

PyObject* GlobalSettings::ResetConfiguration( PyObject* args )
{
	s_collisionMaxIterations = s_collisionMaxIterationsDefault;
	s_useIterativeCollision = s_useIterativeCollisionDefault;
	Py_RETURN_NONE;
}
