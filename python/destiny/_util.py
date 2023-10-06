import destiny


def is_dynamical_orientation_enabled():
    config = destiny.settings.Get()
    return config.useDynamicalOrientation


def enable_dynamical_orientation():
    config = destiny.settings.Get()
    config.useDynamicalOrientation = True
    destiny.settings.Apply(config)


def disable_dynamical_orientation():
    config = destiny.settings.Get()
    config.useDynamicalOrientation = False
    destiny.settings.Apply(config)


def reset_settings_to_default():
    config = destiny.settings.GetDefault()
    destiny.settings.Apply(config)


def enable_new_orbit():
    config = destiny.settings.Get()
    config.useNewOrbit = True
    destiny.settings.Apply(config)
