from destiny.test import helpers


class TestMiniCapsule(helpers.BallparkTestCase):
    def test_add(self):
        parent, = self.add_balls(1)
        parent.AddMiniCapsule(-1, -2, -3, 1, 2, 3, 10)
        mini_capsule = parent.miniCapsules[0]
        self.assertEqual(mini_capsule.ax, -1)
        self.assertEqual(mini_capsule.ay, -2)
        self.assertEqual(mini_capsule.az, -3)
        self.assertEqual(mini_capsule.bx, 1)
        self.assertEqual(mini_capsule.by, 2)
        self.assertEqual(mini_capsule.bz, 3)

    def test_collide(self):
        parent, = self.add_balls(1)
        parent.x = 100
        parent.y = 0
        parent.z = 0
        parent.AddMiniCapsule(100, 0, 0, 200, 0, 0, 10)
        src = helpers.create_space_ball(self.park, x=250, y=30, z=0)
        src.radius = 10.0

        self.park.GotoPoint(src.id, 250, 0, 0)
        coords = self.evolve_ball_and_get_coordinates(src, 10)

        expected_coords = [
            (250.0, 29.60534057196273, 0.0),
            (250.0, 28.46477715834572, 0.0),
            (250.0, 26.639413762737924, 0.0),
            (250.0, 24.18534878569255, 0.0),
            (250.0, 21.15408508070178, 0.0),
            (250.0, 20.546773727349397, 0.0),
            (250.0, 22.081882169977366, 0.0),
            (250.0, 22.713001209633244, 0.0),
            (250.0, 22.514185111733862, 0.0),
            (250.0, 21.553421661039966, 0.0)
        ]

        self.assertEqual(coords, expected_coords)


class TestMiniBall(helpers.BallparkTestCase):
    def test_add(self):
        parent, = self.add_balls(1)
        parent.AddMiniBall(1, 2, 3, 10)
        mini_ball = parent.miniBalls[0]
        self.assertEqual(mini_ball.x, 1)
        self.assertEqual(mini_ball.y, 2)
        self.assertEqual(mini_ball.z, 3)

    def test_collide(self):
        parent, = self.add_balls(1)
        parent.x = 100
        parent.y = 0
        parent.z = 0
        parent.AddMiniBall(100, 0, 0, 10)
        src = helpers.create_space_ball(self.park, x=200, y=30, z=0)
        src.radius = 10.0

        self.park.GotoPoint(src.id, 200, 0, 0)
        coords = self.evolve_ball_and_get_coordinates(src, 10)

        expected_coords = [
            (200.0, 29.60534057196273, 0.0),
            (200.0, 28.46477715834572, 0.0),
            (200.0, 26.639413762737924, 0.0),
            (200.0, 24.18534878569255, 0.0),
            (200.0, 21.15408508070178, 0.0),
            (200.0, 20.41896408511828, 0.0),
            (200.0, 21.71251305959967, 0.0),
            (200.0, 22.121861036713348, 0.0),
            (200.0, 21.71944122765141, 0.0),
            (200.0, 20.571753158732008, 0.0)
        ]

        self.assertEqual(coords, expected_coords)