import destiny
from destiny.test import helpers


class TestWarp(helpers.BallparkTestCase):
    def setUp(self):
        self.maxDiff = None
        super(TestWarp, self).setUp()

    def test_warpto(self):
        locations = []
        ball = helpers.create_space_ball(self.park)

        ball.vx = 1.0
        ball.vy = 2.0
        ball.vz = 3.0

        x, y, z = 100000.0, 200000.0, 300000.0
        self.park.WarpTo(ball.id, x, y, z)

        for i in range(10):
            self.park.Evolve()
            self.assertEqual(ball.mode, destiny.DSTBALL_WARP)
            locations.append((ball.x, ball.y, ball.z))

        expected_locations = [
            (1.0639340706602571, 2.1278681413205143, 3.1918022119807685),
            (2.248703156860341, 4.497406313720682, 6.746109470581021),
            (3.544408527173739, 7.088817054347478, 10.633225581521215),
            (4.941962348268478, 9.883924696536956, 14.825887044805434),
            (6.433021256625409, 12.866042513250818, 19.299063769876224),
            (8.00992537202119, 16.01985074404238, 24.029776116063566),
            (9.665642306989863, 19.331284613979726, 28.99692692096958),
            (11.393715762995505, 22.78743152599101, 34.18114728898651),
            (13.188218337575332, 26.376436675150664, 39.56465501272599),
            (15.043708197493105, 30.08741639498621, 45.13112459247932)
        ]

        self.assertListOfPointsAlmostEqual(locations, expected_locations)
