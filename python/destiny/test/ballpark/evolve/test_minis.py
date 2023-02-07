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
            (250.0, 20.40492196989744, 0.0),
            (250.0, 21.67193142963499, 0.0),
            (250.0, 22.056913993009132, 0.0),
            (250.0, 21.632124770171952, 0.0),
            (250.0, 20.463899776331143, 0.0)
        ]

        self.assertEqual(coords, expected_coords)

class TestMiniBox(helpers.BallparkTestCase):
    def test_add(self):
        parent, = self.add_balls(1)
        parent.x = 100
        parent.y = 200
        parent.z = 300
        parent.AddMiniBox(
            0.0, 0.0, 0.0,
            100.0, 0.0, 0.0,
            0.0, 100.0, 0.0,
            0.0, 0.0, 100.0
        )
        mini_box = parent.miniBoxes[0]

        self.assertEqual(mini_box.c0, 0.0)
        self.assertEqual(mini_box.c1, 0.0)
        self.assertEqual(mini_box.c2, 0.0)

        self.assertEqual(mini_box.x0, 100.0)
        self.assertEqual(mini_box.x1, 0.0)
        self.assertEqual(mini_box.x2, 0.0)

        self.assertEqual(mini_box.y0, 0.0)
        self.assertEqual(mini_box.y1, 100.0)
        self.assertEqual(mini_box.y2, 0.0)

        self.assertEqual(mini_box.z0, 0.0)
        self.assertEqual(mini_box.z1, 0.0)
        self.assertEqual(mini_box.z2, 100.0)

    def test_add_with_rotation(self):
        parent, = self.add_balls(1)
        parent.x = 100
        parent.y = 200
        parent.z = 300

        parent.AddMiniBox(
            0.0, 0.0, 0.0,
            1.0, 1.0, 1.0,
            -1.0, 0.0, 1.0,
            1.0, -2.0, 1.0
        )
        mini_box = parent.miniBoxes[0]

        self.assertEqual(mini_box.c0, 0.0)
        self.assertEqual(mini_box.c1, 0.0)
        self.assertEqual(mini_box.c2, 0.0)

        self.assertEqual(mini_box.x0, 1.0)
        self.assertEqual(mini_box.x1, 1.0)
        self.assertEqual(mini_box.x2, 1.0)

        self.assertEqual(mini_box.y0, -1.0)
        self.assertEqual(mini_box.y1, 0.0)
        self.assertEqual(mini_box.y2, 1.0)

        self.assertEqual(mini_box.z0, 1.0)
        self.assertEqual(mini_box.z1, -2.0)
        self.assertEqual(mini_box.z2, 1.0)

    def test_collide(self):
        parent, = self.add_balls(1)
        parent.x = 0
        parent.y = 0
        parent.z = 0
        parent.AddMiniBox(100.0, 0.0, 0.0,
                          10.0, 0.0, 0.0,
                          0.0, 10.0, 0.0,
                          0.0, 0.0, 10.0)
        src = helpers.create_space_ball(self.park, x=105, y=30, z=5)
        src.radius = 10.0

        self.park.GotoPoint(src.id, 105, 5, 5)
        coords = self.evolve_ball_and_get_coordinates(src, 10)

        expected_coords = [
            (105.0, 29.60534057196273, 5.0),
            (105.0, 28.46477715834572, 5.0),
            (105.0, 26.639413762737924, 5.0),
            (105.0, 24.18534878569255, 5.0),
            (105.0, 21.15408508070178, 5.0),
            (105.0, 20.41896408511828, 5.0),
            (105.0, 21.71251305959967, 5.0),
            (105.0, 22.121861036713348, 5.0),
            (105.0, 21.71944122765141, 5.0),
            (105.0, 20.571753158732008, 5.0)
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