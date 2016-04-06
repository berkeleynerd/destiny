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