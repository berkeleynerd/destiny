from destiny.test import helpers


class TestOrbit(helpers.BallparkTestCase):
    def test_orbit_ball(self):
        self.assertEqual(self.park.time, 0)
        self.maxDiff = None
        helpers.create_space_ball.next_object_id = 1 # Ball IDs are used as seeds for randomness.
        orbitee = helpers.create_space_ball(self.park, x=0.0, y=0.0, z=0.0)
        orbiter = helpers.create_space_ball(self.park, x=5.0)
        self.park.Orbit(orbiter.id, orbitee.id, 1.0)

        orbiter_coordinates = []

        for i in xrange(10):
            self.park.Evolve()
            orbiter_coordinates.append((orbiter.x, orbiter.y, orbiter.z))

        expected_orbiter_coordinates = [
            (5.0, 0.0, 0.3946594280372685),
            (4.968957804601756, -0.01116601720900443, 1.5338416437585325),
            (4.794705661394493, -0.08192431523297017, 3.336008882793782),
            (4.30344070630156, -0.29692292248124114, 5.666293489681137),
            (3.3438872325031914, -0.7390037716697525, 8.326003454582425),
            (1.8414994725824108, -1.460196769206803, 11.097124788926102),
            (-0.20744339756495483, -2.4801243593602917, 13.787939034212652),
            (-2.763448831428362, -3.7967376969998266, 16.246418089514048),
            (-5.7641853212738505, -5.395160687462917, 18.355958426789925),
            (-9.135537997594978, -7.252676323500039, 20.02800422013239)
        ]
        self.assertListOfPointsAlmostEqual(orbiter_coordinates, expected_orbiter_coordinates)