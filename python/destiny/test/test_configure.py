# Copyright © 2023 CCP ehf.

import destiny
import unittest


class TestConfigure(unittest.TestCase):
    def tearDown(self):
        destiny.settings.Reset()

    def test_initial_settings_configuration_values_match_default_values(self):
        config = destiny.SettingsConfiguration()
        default_config = destiny.settings.GetDefault()
        self.assertEqual(default_config.collisionMaxIterations, config.collisionMaxIterations)
        self.assertEqual(default_config.useIterativeCollision, config.useIterativeCollision)

    def test_configure(self):
        config = destiny.settings.GetDefault()
        config.collisionMaxIterations = 123
        destiny.settings.Apply(config)
        applied_config = destiny.settings.Get()
        self.assertEqual(123, applied_config.collisionMaxIterations)

    def test_reset(self):
        config = destiny.settings.GetDefault()
        config.collisionMaxIterations = 123
        destiny.settings.Apply(config)
        destiny.settings.Reset()
        applied_config = destiny.settings.Get()
        default_config = destiny.settings.GetDefault()
        self.assertEqual(default_config.collisionMaxIterations, applied_config.collisionMaxIterations)

    def test_enable_dynamical_orientation(self):
        config = destiny.settings.GetDefault()
        self.assertEqual(False, config.useDynamicalOrientation)
        config.useDynamicalOrientation = True
        destiny.settings.Apply(config)
        applied_config = destiny.settings.Get()
        self.assertEqual(True, applied_config.useDynamicalOrientation)

    def test_disable_dynamical_orientation_for_missiles(self):
        config = destiny.settings.GetDefault()
        self.assertEqual(False, config.disableDynamicalOrientationForMissiles)
        config.disableDynamicalOrientationForMissiles = True
        destiny.settings.Apply(config)
        applied_config = destiny.settings.Get()
        self.assertEqual(True, applied_config.disableDynamicalOrientationForMissiles)

    def test_enable_new_orbit(self):
        config = destiny.settings.GetDefault()
        self.assertEqual(False, config.useNewOrbit)
        config.useNewOrbit = True
        destiny.settings.Apply(config)
        applied_config = destiny.settings.Get()
        self.assertEqual(True, applied_config.useNewOrbit)
