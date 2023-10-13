import destiny
from destiny.test import helpers
from destiny._util import enable_new_orbit, reset_settings_to_default
import math


class TestOldOrbit(helpers.BallparkTestCase):
    def test_orbit_ball(self):
        self.assertEqual(self.park.time, 0)
        self.maxDiff = None
        helpers.create_space_ball.next_object_id = 1 # Ball IDs are used as seeds for randomness.
        orbitee = helpers.create_space_ball(self.park, x=0.0, y=0.0, z=0.0)
        orbiter = helpers.create_space_ball(self.park, x=5.0)
        self.park.Orbit(orbiter.id, orbitee.id, 1.0)

        orbiter_coordinates = []

        for i in range(10):
            self.park.Evolve()
            orbiter_coordinates.append((orbiter.x, orbiter.y, orbiter.z))

        expected_orbiter_coordinates = [
            (5.0, 0.0, 0.3946594280372685),
            (4.968965087530276, -0.014060509698578017, 1.533749375235321),
            (4.7948197558341885, -0.09342506603769896, 3.3353413772468783),
            (4.303462228867957, -0.3182911817228117, 5.664861650309143),
            (3.341010762290748, -0.7601346870799338, 8.325917825178466),
            (1.826888038965309, -1.4570571575293851, 11.103680716692217),
            (-0.25072581197636695, -2.415534207332392, 13.808614450861453),
            (-2.860769892110699, -3.6221094985835713, 16.289471089613752),
            (-5.948838546575331, -5.052297043579673, 18.42935795458875),
            (-9.447930869309866, -6.675507070626672, 20.138672333159825)
        ]
        self.assertListOfPointsAlmostEqual(orbiter_coordinates, expected_orbiter_coordinates)


class TestNewOrbit(helpers.BallparkTestCase):
    def setUp(self):
        enable_new_orbit()
        super(TestNewOrbit, self).setUp()
        helpers.create_space_ball.next_object_id = 1 # Ball IDs are used as seeds for randomness.
        self.orbitee = helpers.create_space_ball(self.park, x=0.0, y=0.0, z=0.0)
        self.orbiter = helpers.create_space_ball(self.park, x=10.0)

    def tearDown(self):
        reset_settings_to_default()
        super(TestNewOrbit, self).tearDown()
        del self.orbitee
        del self.orbiter

    def test_orbit_stopped_ball_starting_right_distance(self):
        #Test a few different orbits, tight orbits do not work properly
        for orbit in [100.0, 500.0, 1000.0, 2000.0]:
            expected_distance = orbit+self.orbitee.radius+self.orbiter.radius
            self.park.SetBallPosition(self.orbiter.id, expected_distance, 0.0, 0.0)
            self.park.SetBallVelocity(self.orbiter.id, 0.0, 0.0, 1.0)
            self.park.Orbit(self.orbiter.id, self.orbitee.id, orbit)

            for i in range(100):
                self.park.Evolve()
                #Assert that the distance is truly the radii plus orbit
                tovector = [self.orbiter.x - self.orbitee.x,
                            self.orbiter.y - self.orbitee.y,
                            self.orbiter.z - self.orbitee.z]
                distance = math.sqrt(sum(x**2 for x in tovector))

                #Allow for a significant deviation
                self.assertAlmostEquals(distance, expected_distance, delta=20)

                #Make sure we are rotating, the velocity should be nearly perpendicular to
                #the direction to the orbitee
                velocity = [self.orbiter.vx, self.orbiter.vy, self.orbiter.vz]
                speed = math.sqrt(sum(x**2 for x in velocity))
                self.assertTrue(abs(sum([velocity[i]*tovector[i] for i in range(3)])) < 0.2*speed*distance)

    def test_orbit_stopped_ball_starting_outside(self):
        #Use a single orbit and make sure we head inwards until orbit is reached
        orbit = 1000.0
        expected_distance = orbit+self.orbitee.radius+self.orbiter.radius

        self.park.SetBallPosition(self.orbiter.id, 1.5*expected_distance, 0.0, 0.0)
        self.park.SetBallVelocity(self.orbiter.id, -1.0, 0.0, 1.0)
        self.park.Orbit(self.orbiter.id, self.orbitee.id, orbit)

        previous_distance = self.orbiter.x
        iorbit = 2000
        for i in range(iorbit):
            self.park.Evolve()
            # The actual distance
            tovector = [self.orbiter.x - self.orbitee.x,
                        self.orbiter.y - self.orbitee.y,
                        self.orbiter.z - self.orbitee.z]
            distance = math.sqrt(sum(x ** 2 for x in tovector))

            if distance > expected_distance + 10:
                #Make sure we are closing in
                self.assertTrue(distance < previous_distance)
                previous_distance = distance

                #Assert that we are not going straight in
                #Angle between velocity and center should be more than 35 degrees
                velocity = [self.orbiter.vx, self.orbiter.vy, self.orbiter.vz]
                speed = math.sqrt(sum(x**2 for x in velocity))
                self.assertTrue(abs(sum([velocity[i]*tovector[i] for i in range(3)])) < 0.8*speed*distance)

            else:
                #Make sure we keep that distance
                if (iorbit > i):
                    iorbit = i
                # Allow for a significant deviation
                self.assertAlmostEquals(distance, expected_distance, delta=10)

        self.assertTrue(iorbit < 2000)

    def test_orbit_stopped_ball_starting_inside(self):
        #Use a single orbit and make sure we head outwards until orbit is reached
        orbit = 1000.0
        expected_distance = orbit+self.orbitee.radius+self.orbiter.radius

        self.park.SetBallPosition(self.orbiter.id, 0.5*expected_distance, 0.0, 0.0)
        self.park.SetBallVelocity(self.orbiter.id, 0.0, 0.0, 1.0)
        self.park.Orbit(self.orbiter.id, self.orbitee.id, orbit)

        previous_distance = self.orbiter.x
        iorbit = 2000
        for i in range(iorbit):
            self.park.Evolve()
            # The actual distance
            tovector = [self.orbiter.x - self.orbitee.x,
                        self.orbiter.y - self.orbitee.y,
                        self.orbiter.z - self.orbitee.z]
            distance = math.sqrt(sum(x ** 2 for x in tovector))

            if distance < expected_distance - 20:
                #Make sure we are closing in
                self.assertTrue(distance > previous_distance)
                previous_distance = distance

                #Assert that we are not going straight in, but rather at an angle
                velocity = [self.orbiter.vx, self.orbiter.vy, self.orbiter.vz]
                speed = math.sqrt(sum(x**2 for x in velocity))
                self.assertTrue(abs(sum([velocity[i]*tovector[i] for i in range(3)])) < 0.85*speed*distance)

            else:
                #Make sure we keep that distance
                if (iorbit > i):
                    iorbit = i
                # Allow for a significant deviation
                self.assertAlmostEquals(distance, expected_distance, delta=20)

        self.assertTrue(iorbit < 2000)

    def test_orbit_moving_ball_starting_right_distance(self):
        #Just a single orbit
        orbit = 2000.0
        expected_distance = orbit+self.orbitee.radius+self.orbiter.radius

        #Make it possible for the orbiter to orbit
        self.orbiter.maxVelocity = 200
        self.orbitee.maxVelocity = 50
        self.park.GotoDirection(self.orbitee.id, 1.0, 1.0, 1.0)

        self.park.SetBallPosition(self.orbiter.id, expected_distance, 0.0, 0.0)
        self.park.SetBallVelocity(self.orbiter.id, 0.0, 0.0, 1.0)
        self.park.Orbit(self.orbiter.id, self.orbitee.id, orbit)

        for i in range(200):
            self.park.Evolve()
            # Assert that the distance is truly the radii plus orbit
            tovector = [self.orbiter.x - self.orbitee.x,
                        self.orbiter.y - self.orbitee.y,
                        self.orbiter.z - self.orbitee.z]
            distance = math.sqrt(sum(x ** 2 for x in tovector))

            # Allow for an even more significant deviation
            self.assertAlmostEquals(distance, expected_distance, delta=700)

            # Assert that we are moving almost in a circle around the orbitee
            velocity = [self.orbiter.vx-self.orbitee.vx,
                        self.orbiter.vy-self.orbitee.vy,
                        self.orbiter.vz-self.orbitee.vz]
            speed = math.sqrt(sum(x ** 2 for x in velocity))
            self.assertTrue(abs(sum([velocity[i] * tovector[i] for i in range(3)])) < 0.4 * speed * distance)