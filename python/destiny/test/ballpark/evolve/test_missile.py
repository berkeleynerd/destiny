from destiny.test import helpers


class TestMissile(helpers.BallparkTestCase):
    def setUp(self):
        super(TestMissile, self).setUp()

        self.dst = helpers.create_space_ball(self.park, x=1e6)
        self.owner = helpers.create_space_ball(self.park)

        self.collider = helpers.create_space_ball(self.park, x=2e3)
        self.collider.radius = 1.9e3
        self.missile = helpers.create_space_ball(self.park)

        # Set missile like properties
        self.missile.maxVelocity = 300
        self.missile.Agility = 0.01
        try:
            self.missile.maxAngularVelocity = 2.0
            self.missile.RotationalAgility = 0.5
        except AttributeError:
            pass
        self.missile.mass = 1e4

    def tearDown(self):
        super(TestMissile, self).tearDown()
        del self.dst
        del self.owner
        del self.collider
        del self.missile

    def test_not_aimed_not_massive_from_stationary(self):
        self.park.LaunchMissile(self.missile.id, self.dst.id, self.owner.id, False, False)

        # Non-aimed missiles launched from stationary objects should go backwards
        # at a high speed.  It uses 180-degree rotation, causing minor inaccuracies
        self.assertAlmostEquals(self.missile.vx, 0.0)
        self.assertAlmostEquals(self.missile.vy, 0.0)
        self.assertAlmostEquals(self.missile.vz, -150)

        for i in range(20):
            self.park.Evolve()

        # After launch, they aim for their destination, ignoring the intruder
        self.assertAlmostEquals(self.missile.vx, self.missile.maxVelocity * self.missile.speedFraction, places=2)
        self.assertAlmostEquals(self.missile.vy, 0.0, delta=0.5)
        self.assertAlmostEquals(self.missile.vz, 0.0, delta=0.5)

        # The intruder should not have moved
        self.assertAlmostEquals(self.collider.x, 2e3)
        self.assertAlmostEquals(self.collider.y, 0.0)
        self.assertAlmostEquals(self.collider.z, 0.0)

    def test_not_aimed_not_massive_from_moving(self):
        self.owner.vz = 10
        self.park.LaunchMissile(self.missile.id, self.dst.id, self.owner.id, False, False)
        self.owner.vz = 0

        # Non-aimed missiles launched with velocity should go in the
        # direction of the velocity with 150 on top of the velocity
        self.assertAlmostEquals(self.missile.vx, 0.0)
        self.assertAlmostEquals(self.missile.vy, 0.0)
        self.assertAlmostEquals(self.missile.vz, 160)

        for i in range(20):
            self.park.Evolve()

        # After launch, they aim for their destination
        self.assertAlmostEquals(self.missile.vx, self.missile.maxVelocity * self.missile.speedFraction, places=4)
        self.assertAlmostEquals(self.missile.vy, 0.0, places=0)
        self.assertAlmostEquals(self.missile.vz, 0.0, places=0)

        # The intruder should not have moved
        self.assertAlmostEquals(self.collider.x, 2e3)
        self.assertAlmostEquals(self.collider.y, 0.0)
        self.assertAlmostEquals(self.collider.z, 0.0)

    def test_aimed_not_massive(self):
        self.park.LaunchMissile(self.missile.id, self.dst.id, self.owner.id, True, False)

        # Aimed missiles should go straight in the direction of the target
        self.assertAlmostEquals(self.missile.vx, 1.0)
        self.assertAlmostEquals(self.missile.vy, 0.0)
        self.assertAlmostEquals(self.missile.vz, 0.0)

        for i in range(20):
            self.park.Evolve()

        # After launch, they should just keep going in that direction
        self.assertAlmostEquals(self.missile.vx, self.missile.maxVelocity * self.missile.speedFraction, places=4)
        self.assertAlmostEquals(self.missile.vy, 0.0, places=4)
        self.assertAlmostEquals(self.missile.vz, 0.0, places=4)

        # The intruder should not have moved
        self.assertAlmostEquals(self.collider.x, 2e3)
        self.assertAlmostEquals(self.collider.y, 0.0)
        self.assertAlmostEquals(self.collider.z, 0.0)

        # The missile should have moved beyond the collider
        self.assertGreater(self.missile.x, self.collider.x)

    def test_aimed_massive(self):
        self.park.LaunchMissile(self.missile.id, self.dst.id, self.owner.id, True, True)

        for i in range(20):
            self.park.Evolve()

        # After launch, they should just keep going in that direction
        self.assertAlmostEquals(self.missile.vx, self.missile.maxVelocity * self.missile.speedFraction, places=4)
        self.assertAlmostEquals(self.missile.vy, 0.0, places=4)
        self.assertAlmostEquals(self.missile.vz, 0.0, places=4)

        # The intruder should not have moved
        self.assertAlmostEquals(self.collider.x, 2e3)
        self.assertAlmostEquals(self.collider.y, 0.0)
        self.assertAlmostEquals(self.collider.z, 0.0)
