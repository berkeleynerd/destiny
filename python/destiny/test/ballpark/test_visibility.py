import destiny
from destiny.test import helpers

import math


class TestCheckVisibility(helpers.BallparkTestCase):
    def test_returns_zero_when_there_is_no_occlusion(self):
        src, dst = self.add_balls(2)
        dst.x += 100.0
        id = self.park.CheckVisibility(src.id, dst.id)
        self.assertEqual(id, 0)

    def test_returns_the_id_of_the_occluding_ball_when_there_is_an_occlusion(self):
        src, dst, occluder = self.add_balls(3)
        occluder.radius = 25.0
        occluder.x += 50.0
        occluder.isMassive = True
        dst.x += 100.0
        id = self.park.CheckVisibility(src.id, dst.id)
        self.assertEqual(id, occluder.id)

    def test_ball_that_is_not_massive_is_not_an_occlusion(self):
        src, dst, occluder = self.add_balls(3)
        occluder.radius = 25.0
        occluder.x += 50.0
        occluder.isMassive = False
        dst.x += 100.0
        id = self.park.CheckVisibility(src.id, dst.id)
        self.assertNotEqual(id, occluder.id)

    def test_ball_that_is_cloaked_is_not_an_occlusion(self):
        src, dst, occluder = self.add_balls(3)
        occluder.radius = 25.0
        occluder.x += 50.0
        occluder.isMassive = True
        occluder.isCloaked = True
        dst.x += 100.0
        id = self.park.CheckVisibility(src.id, dst.id)
        self.assertNotEqual(id, occluder.id)


class TestCloaking(helpers.BallparkTestCase):
    def test_cloaked_balls_are_cloaked(self):
        ball, = self.add_balls(1)
        self.park.CloakBall(ball.id, destiny.DSTNORMALCLOAK)
        self.assertTrue(ball.isCloaked)

    def test_cloaked_balls_are_not_massive(self):
        ball, = self.add_balls(1)
        ball.isMassive = True
        self.park.CloakBall(ball.id, destiny.DSTNORMALCLOAK)
        self.assertFalse(ball.isMassive)

    def test_uncloaked_balls_are_not_cloaked(self):
        ball, = self.add_balls(1)
        ball.isCloaked = True
        self.park.UncloakBall(ball.id)
        self.assertFalse(ball.isCloaked)

    def test_uncloaking_makes_a_ball_massive(self):
        ball, = self.add_balls(1)
        ball.isMassive = False
        self.park.UncloakBall(ball.id)
        self.assertTrue(ball.isMassive)

    def test_uncloaking_a_ball_in_warp_does_not_make_it_massive(self):
        ball, = self.add_balls(1)
        ball.isMassive = False
        ball.x, ball.y, ball.z = 0, 0, 0

        # Need to be aligned with the warp vector and over 75% max speed in order to start the warp.
        ball.maxVelocity = 1.0
        ball.vx = 1.0
        self.park.SetBallFree(ball.id, True)
        self.park.WarpTo(ball.id, helpers.TEN_BILLION, 0, 0)
        self.park.Evolve()

        self.park.UncloakBall(ball.id)
        self.assertFalse(ball.isMassive)


class TestScanCone(helpers.BallparkTestCase):
    def test_scan_cone_finds_nothing_when_there_is_nothing_to_be_found(self):
        ball, = self.add_balls(1)
        angle = 45.0
        _range = 100.0
        x, y, z = (1, 0, 0)
        result = self.park.ScanCone(ball.id, angle, _range, x, y, z)
        self.assertEqual(result, [])

    def test_scan_cone_finds_ball_when_it_is_in_the_cone(self):
        ball, other = self.add_balls(2)
        other.x += 50.0
        angle = math.pi / 2.0
        _range = 100.0
        x, y, z = (1.0, 0.0, 0.0)
        result = self.park.ScanCone(ball.id, angle, _range, x, y, z)
        self.assertEqual(result, [other.id])

    def test_scan_cone_excludes_balls_not_in_cone(self):
        ball, left, up, down, forward, back = self.add_balls(6)
        left.x -= 50.0
        up.y += 50.0
        down.y -= 50.0
        forward.z += 50.0
        back.z -= 50.0
        angle = math.pi / 2.0
        _range = 100.0
        x, y, z = (1.0, 0.0, 0.0)
        result = self.park.ScanCone(ball.id, angle, _range, x, y, z)
        self.assertEqual(result, [])
