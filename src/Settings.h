#pragma once
#ifndef DESTINY_SETTINGS_H
#define DESTINY_SETTINGS_H
#include "stdafx.h"


constexpr size_t s_collisionMaxIterationsDefault = 20;
constexpr bool s_useIterativeCollisionDefault = false;

// Only do this many iterations in the search for collision before stopping.
extern size_t s_collisionMaxIterations;
extern bool s_useIterativeCollision;


BLUE_CLASS( SettingsConfiguration ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();
	size_t m_collisionMaxIterations = s_collisionMaxIterationsDefault;
	bool m_useIterativeCollision = s_useIterativeCollisionDefault;
};
TYPEDEF_BLUECLASS( SettingsConfiguration );

BLUE_CLASS( GlobalSettings ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();
	PyObject* ApplyConfiguration( PyObject * args );
	PyObject* GetConfiguration( PyObject * args );
	PyObject* GetDefaultConfiguration( PyObject* args );
	PyObject* ResetConfiguration( PyObject * args );
};

TYPEDEF_BLUECLASS( GlobalSettings );

extern GlobalSettings* s_globalSettings;

#endif
