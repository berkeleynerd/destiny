#include "StdAfx.h"
#include "Settings.h"

BLUE_DEFINE( GlobalSettings );

const Be::ClassInfo* GlobalSettings::ExposeToBlue()
{
	EXPOSURE_BEGIN( GlobalSettings, "Use this to configure the destiny module" )
		MAP_INTERFACE( IRoot )
		MAP_INTERFACE( GlobalSettings )
		MAP_METHOD_AS_METHOD( "Apply", ApplyConfiguration, "Pass in an instance of destiny.SettingsConfiguration to configure global settings." );
		MAP_METHOD_AS_METHOD( "Get", GetConfiguration, "Get the current global settings configuration." );
		MAP_METHOD_AS_METHOD( "GetDefault", GetDefaultConfiguration, "Get the default global settings configuration." );
		MAP_METHOD_AS_METHOD( "Reset", ResetConfiguration, "Reset the current global settings configuration to the default values." );
	EXPOSURE_END()
}

BLUE_DEFINE( SettingsConfiguration );

const Be::ClassInfo* SettingsConfiguration::ExposeToBlue()
{
	EXPOSURE_BEGIN( SettingsConfiguration, "Pass an instance of this to destiny.settings.Configure() to apply a settings configuration" )
		MAP_INTERFACE( IRoot )
		MAP_INTERFACE( SettingsConfiguration )
		MAP_ATTRIBUTE( "collisionMaxIterations", m_collisionMaxIterations, "The maximum number of collision iterations (assuming iterative collision is enabled). Recommended value >= 10, at 5 agile ships are known to fly through thin boxes.", Be::READWRITE )
		MAP_ATTRIBUTE( "useIterativeCollision", m_useIterativeCollision, "Boolean flag to enable/disable iterative collision", Be::READWRITE )
	EXPOSURE_END()
}

BLUE_REGISTER_GLOBAL_AS_MODULE_OBJECT( "settings", s_globalSettings );
