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
        helpers.create_space_ball.next_object_id = 2
        src = helpers.create_space_ball(self.park, x=200, y=0, z=-30)
        src.radius = 10.0

        self.park.GotoPoint(src.id, 200, 0, 0)
        coords = self.evolve_ball_and_get_coordinates(src, 20)

        # Make sure there is no significant overlap, and we move only in
        # the z-direction
        for coord in coords:
            self.assertAlmostEquals(coord[0], 200)
            self.assertAlmostEquals(coord[1], 0.0)
            self.assertTrue(1.1*coord[2] < -20, "Significant overlap: %f, %f"%(coord[2], -20))

    def test_collide_with_miniballs(self):
        parent, = self.add_balls(1)
        parent.x = 100
        parent.y = 0
        parent.z = 0
        parent.AddMiniBall(100, 0, 0, 10)
        helpers.create_space_ball.next_object_id = 2
        src = helpers.create_space_ball(self.park, x=200, y=0, z=-30)
        src.radius = 10.0
        #Add several balls, only one of which should be hit
        src.AddMiniBall(4, 0, 4, 3)
        src.AddMiniBall(0, 0, 5, 2)
        src.AddMiniBall(-4, 0, 4, 3)

        self.park.GotoPoint(src.id, 200, 0, 0)
        coords = self.evolve_ball_and_get_coordinates(src, 20)

        # Make sure there is no significant overlap, and we move only in
        # the z-direction.  The overlap is roughly estimated
        for coord in coords:
            self.assertAlmostEquals(coord[0], 200)
            self.assertAlmostEquals(coord[1], 0.0)
            self.assertTrue(1.01*coord[2] < -17, "Significant overlap: %f, %f"%(coord[2], -17))
