from destiny.test import helpers

class TestMissile(helpers.BallparkTestCase):
    def test_not_aimed_not_massive(self):
        src = helpers.create_space_ball(self.park)
        dst = helpers.create_space_ball(self.park, x=10.0)
        owner = helpers.create_space_ball(self.park)
        self.park.LaunchMissile(src.id, dst.id, owner.id, False, False)
        missile_coordinates = []
        for i in xrange(10):
            self.park.Evolve()
            missile_coordinates.append((src.x, src.y, src.z))

        expected_missile_coordinates = [
            (0.0, 0.0, -144.18395768140732),
            (0.028743517593736632, 0.0, -276.54578027116486),
            (0.09803783106128847, 0.0, -397.24664580273657),
            (0.18655641597057487, 0.0, -507.24096664400145),
            (0.28592059677481174, 0.0, -607.4056283618579),
            (0.39159797696709886, 0.0, -698.5457962466971),
            (0.5007884360362507, 0.0, -781.4007136373806),
            (0.6116371320541193, 0.0, -856.6490812109938),
            (0.7228658129569013, 0.0, -924.9140088245537),
            (0.8335741171028117, 0.0, -986.7675660216928)
        ]
        self.assertListOfPointsAlmostEqual(missile_coordinates, expected_missile_coordinates, places=2)

    def test_aimed_not_massive(self):
        src = helpers.create_space_ball(self.park)
        dst = helpers.create_space_ball(self.park, x=10.0)
        owner = helpers.create_space_ball(self.park)
        self.park.LaunchMissile(src.id, dst.id, owner.id, True, False)
        missile_coordinates = []
        for i in xrange(10):
            self.park.Evolve()
            missile_coordinates.append((src.x, src.y, src.z))

        expected_missile_coordinates = [
            (1.373887883884875, 0.0, 0.0),
            (3.454421657712534, 0.0, 0.0),
            (6.183713317356512, 0.0, 0.0),
            (9.508617008994243, 0.0, 0.0),
            (13.380340555020881, 0.0, 0.0),
            (16.923226839336774, 0.0, 0.0),
            (19.35668806611188, 0.0, 0.0),
            (20.771607665396463, 0.0, 0.0),
            (21.251423951591427, 0.0, 0.0),
            (20.872740022821617, 0.0, 0.0)
        ]
        self.assertListOfPointsAlmostEqual(missile_coordinates, expected_missile_coordinates)

    def test_not_aimed_massive(self):
        src = helpers.create_space_ball(self.park)
        dst = helpers.create_space_ball(self.park, x=10.0)
        owner = helpers.create_space_ball(self.park)
        self.park.LaunchMissile(src.id, dst.id, owner.id, False, True)
        missile_coordinates = []
        for i in xrange(10):
            self.park.Evolve()
            missile_coordinates.append((src.x, src.y, src.z))

        expected_missile_coordinates = [
            (0.0, 0.0, -144.18395768140732),
            (0.028743517593736632, 0.0, -276.54578027116486),
            (0.09803783106128847, 0.0, -397.24664580273657),
            (0.18655641597057487, 0.0, -507.24096664400145),
            (0.28592059677481174, 0.0, -607.4056283618579),
            (0.39159797696709886, 0.0, -698.5457962466971),
            (0.5007884360362507, 0.0, -781.4007136373806),
            (0.6116371320541193, 0.0, -856.6490812109938),
            (0.7228658129569013, 0.0, -924.9140088245537),
            (0.8335741171028117, 0.0, -986.7675660216928)
        ]
        self.assertListOfPointsAlmostEqual(missile_coordinates, expected_missile_coordinates, places=2)

    def test_aimed_massive(self):
        src = helpers.create_space_ball(self.park)
        dst = helpers.create_space_ball(self.park, x=10.0)
        owner = helpers.create_space_ball(self.park)
        self.park.LaunchMissile(src.id, dst.id, owner.id, True, True)
        missile_coordinates = []
        for i in xrange(10):
            self.park.Evolve()
            missile_coordinates.append((src.x, src.y, src.z))

        expected_missile_coordinates = [
            (1.373887883884875, 0.0, 0.0),
            (3.454421657712534, 0.0, 0.0),
            (6.183713317356512, 0.0, 0.0),
            (9.508617008994243, 0.0, 0.0),
            (13.380340555020881, 0.0, 0.0),
            (16.923226839336774, 0.0, 0.0),
            (19.35668806611188, 0.0, 0.0),
            (20.771607665396463, 0.0, 0.0),
            (21.251423951591427, 0.0, 0.0),
            (20.872740022821617, 0.0, 0.0)
        ]
        self.assertListOfPointsAlmostEqual(missile_coordinates, expected_missile_coordinates)
