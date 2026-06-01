# Copyright © 2015 CCP ehf.

import destiny
from destiny.test import helpers

class TestFollow(helpers.BallparkTestCase):
    def test_follow_ball(self):
        src, dst = self.add_balls(2)
        self.park.FollowBall(src.id, dst.id)
        self.assertEqual(src.followId, dst.id)
        self.assertEqual(src.followRange, 1.0)

    def test_follow_ball_with_range(self):
        src, dst = self.add_balls(2)
        self.park.FollowBall(src.id, dst.id, 42.0)
        self.assertEqual(src.followId, dst.id)
        self.assertEqual(src.followRange, 42.0)

    def test_ball_can_not_follow_self(self):
        src = helpers.add_ball_to_park(self.park, objectID=1)
        self.park.FollowBall(src.id, src.id)
        self.assertNotEqual(src.followId, src.id)

    def test_ball_can_not_follow_moribund_ball(self):
        src, dst = self.add_balls(2)
        self.park.RemoveBall(dst.id)
        self.park.FollowBall(src.id, dst.id)
        self.assertNotEqual(src.followId, dst.id)

    def test_removing_ball_stops_followers_from_following_that_ball(self):
        src, dst = self.add_balls(2)
        self.park.FollowBall(src.id, dst.id)
        self.park.RemoveBall(dst.id)
        self.assertNotEqual(src.followId, dst.id)


class TestMissileFollow(helpers.BallparkTestCase):
    def test_sets_ids_correctly(self):
        src, dst, owner = self.add_balls(3)
        self.park.MissileFollow(src.id, dst.id, owner.id)
        self.assertEqual(src.followId, dst.id)
        self.assertEqual(src.ownerId, owner.id)

    def test_sets_follow_range_as_negative_sum_of_src_and_dst_radii(self):
        src, dst, owner = self.add_balls(3)
        src.radius = 1.0
        dst.radius = 2.0
        self.park.MissileFollow(src.id, dst.id, owner.id)
        self.assertEqual(src.followRange, -3.0)

    def test_can_not_follow_moribund_ball(self):
        src, dst, owner = self.add_balls(3)
        self.park.RemoveBall(dst.id)
        self.park.MissileFollow(src.id, dst.id, owner.id)
        self.assertNotEqual(src.followId, dst.id)

    def test_removing_ball_stops_followers_from_following_that_ball(self):
        src, dst, owner = self.add_balls(3)
        self.park.FollowBall(src.id, dst.id)
        self.park.RemoveBall(dst.id)
        self.assertNotEqual(src.followId, dst.id)

    def test_sets_missile_mode_on_src_ball(self):
        src, dst, owner = self.add_balls(3)
        self.park.MissileFollow(src.id, dst.id, owner.id)
        self.assertEqual(src.mode, destiny.DSTBALL_MISSILE)


class TestWarpTo(helpers.BallparkTestCase):
    def test_can_warp(self):
        src, = self.add_balls(1)
        x,y,z = 1, 2, 3
        self.park.WarpTo(src.id, x, y, z)
        self.assertEqual(src.mode, destiny.DSTBALL_WARP)
        self.assertEqual(src.gotoX, x)
        self.assertEqual(src.gotoY, y)
        self.assertEqual(src.gotoZ, z)

    def test_minrange_and_warpfactor_are_stored_in_strange_places(self):
        src, = self.add_balls(1)
        x,y,z = 1, 2, 3
        min_range = 42.0
        warp_factor = 10000
        self.park.WarpTo(src.id, x, y, z, min_range, warp_factor)
        min_range_interpreted_as_int = helpers.reinterpret_double_as_int(min_range)
        self.assertEqual(src.followId, min_range_interpreted_as_int)
        self.assertEqual(src.ownerId, warp_factor)

    def test_if_warp_location_is_too_close_we_goto_it_instead(self):
        src = helpers.add_ball_to_park(self.park, objectID=1, x=0, y=0, z=0)
        x,y,z = 1, 2, 3
        self.park.WarpTo(src.id, x, y, z)
        self.assertEqual(src.mode, destiny.DSTBALL_GOTO)
        self.assertEqual(src.gotoX, x)
        self.assertEqual(src.gotoY, y)
        self.assertEqual(src.gotoZ, z)


class GotoPoint(helpers.BallparkTestCase):
    def test_can_goto(self):
        src = helpers.add_ball_to_park(self.park, objectID=1, x=0, y=0, z=0)
        x,y,z = 1, 2, 3
        self.park.GotoPoint(src.id, x, y, z)
        self.assertEqual(src.mode, destiny.DSTBALL_GOTO)
        self.assertEqual(src.gotoX, x)
        self.assertEqual(src.gotoY, y)
        self.assertEqual(src.gotoZ, z)
        self.assertEqual(src.speedFraction, 1.0)


class GotoDirection(helpers.BallparkTestCase):
    def test_go_right(self):
        src = helpers.add_ball_to_park(self.park, objectID=1, x=0, y=0, z=0)
        x,y,z = 1, 0, 0
        self.park.GotoDirection(src.id, x, y, z)
        self.assertEqual(src.mode, destiny.DSTBALL_GOTO)
        self.assertEqual(src.gotoX, 1e+17)
        self.assertEqual(src.gotoY, 0)
        self.assertEqual(src.gotoZ, 0)
        self.assertEqual(src.speedFraction, 1.0)

    def test_go_left(self):
        src = helpers.add_ball_to_park(self.park, objectID=1, x=0, y=0, z=0)
        x,y,z = -1, 0, 0
        self.park.GotoDirection(src.id, x, y, z)
        self.assertEqual(src.mode, destiny.DSTBALL_GOTO)
        self.assertEqual(src.gotoX, -1e+17)
        self.assertEqual(src.gotoY, 0)
        self.assertEqual(src.gotoZ, 0)
        self.assertEqual(src.speedFraction, 1.0)

    def test_go_up(self):
        src = helpers.add_ball_to_park(self.park, objectID=1, x=0, y=0, z=0)
        x,y,z = 0, 1, 0
        self.park.GotoDirection(src.id, x, y, z)
        self.assertEqual(src.mode, destiny.DSTBALL_GOTO)
        self.assertEqual(src.gotoX, 0)
        self.assertEqual(src.gotoY, 1e+17)
        self.assertEqual(src.gotoZ, 0)
        self.assertEqual(src.speedFraction, 1.0)

    def test_go_down(self):
        src = helpers.add_ball_to_park(self.park, objectID=1, x=0, y=0, z=0)
        x,y,z = 0, -1, 0
        self.park.GotoDirection(src.id, x, y, z)
        self.assertEqual(src.mode, destiny.DSTBALL_GOTO)
        self.assertEqual(src.gotoX, 0)
        self.assertEqual(src.gotoY, -1e+17)
        self.assertEqual(src.gotoZ, 0)
        self.assertEqual(src.speedFraction, 1.0)

    def test_go_forward(self):
        src = helpers.add_ball_to_park(self.park, objectID=1, x=0, y=0, z=0)
        x,y,z = 0, 0, 1
        self.park.GotoDirection(src.id, x, y, z)
        self.assertEqual(src.mode, destiny.DSTBALL_GOTO)
        self.assertEqual(src.gotoX, 0)
        self.assertEqual(src.gotoY, 0)
        self.assertEqual(src.gotoZ, 1e+17)
        self.assertEqual(src.speedFraction, 1.0)

    def test_go_backward(self):
        src = helpers.add_ball_to_park(self.park, objectID=1, x=0, y=0, z=0)
        x,y,z = 0, 0, -1
        self.park.GotoDirection(src.id, x, y, z)
        self.assertEqual(src.mode, destiny.DSTBALL_GOTO)
        self.assertEqual(src.gotoX, 0)
        self.assertEqual(src.gotoY, 0)
        self.assertEqual(src.gotoZ, -1e+17)
        self.assertEqual(src.speedFraction, 1.0)

    def test_three_dimensional_movement(self):
        ball = helpers.add_ball_to_park(self.park, objectID=1, x=0, y=0, z=0)
        self.park.GotoDirection(ball.id, 1, 1, 1)
        self.assertAlmostEqual(ball.gotoX, 5.7735026918962584e+16)
        self.assertAlmostEqual(ball.gotoY, 5.7735026918962584e+16)
        self.assertAlmostEqual(ball.gotoZ, 5.7735026918962584e+16)

    def test_direction_vector_gets_normalized(self):
        first = helpers.add_ball_to_park(self.park, objectID=1, x=0, y=0, z=0)
        second = helpers.add_ball_to_park(self.park, objectID=2, x=0, y=0, z=0)
        self.park.GotoDirection(first.id, 1, 1, 1)
        self.park.GotoDirection(second.id, 5, 5, 5)
        self.assertAlmostEqual(first.gotoX / 1e16, second.gotoX / 1e16)
        self.assertAlmostEqual(first.gotoY / 1e16, second.gotoY / 1e16)
        self.assertAlmostEqual(first.gotoZ / 1e16, second.gotoZ / 1e16)


class TestOrbit(helpers.BallparkTestCase):
    def test_can_orbit(self):
        first, second = self.add_balls(2)
        self.park.Orbit(first.id, second.id)
        self.assertEqual(first.followId, second.id)
        self.assertEqual(first.followRange, 1.0)
        self.assertEqual(first.mode, destiny.DSTBALL_ORBIT)
        self.assertIn(first.id, self.park.GetFollowers(second.id))

    def test_can_not_orbit_self(self):
        ball, = self.add_balls(1)
        self.park.Orbit(ball.id, ball.id)
        self.assertNotEqual(ball.followId, ball.id)
        self.assertNotEqual(ball.followRange, 1.0)
        self.assertNotEqual(ball.mode, destiny.DSTBALL_ORBIT)
        self.assertNotIn(ball.id, self.park.GetFollowers(ball.id))

    def test_orbit_at_range(self):
        first, second = self.add_balls(2)
        self.park.Orbit(first.id, second.id, 42.0)
        self.assertEqual(first.followRange, 42.0)

    def test_can_not_orbit_at_nan_range(self):
        first, second = self.add_balls(2)
        self.park.Orbit(first.id, second.id, float('nan'))
        self.assertNotEqual(first.followId, second.id)
        self.assertNotEqual(first.followRange, 1.0)
        self.assertNotEqual(first.mode, destiny.DSTBALL_ORBIT)
        self.assertNotIn(first.id, self.park.GetFollowers(second.id))

    def test_can_not_orbit_cloaked_ball(self):
        first, second = self.add_balls(2)
        self.park.CloakBall(second.id, destiny.DSTNORMALCLOAK)
        self.park.Orbit(first.id, second.id)
        self.assertNotEqual(first.followId, second.id)
        self.assertNotEqual(first.followRange, 1.0)
        self.assertNotEqual(first.mode, destiny.DSTBALL_ORBIT)
        self.assertNotIn(first.id, self.park.GetFollowers(second.id))

    def test_can_not_orbit_ball_in_different_bubble(self):
        first, second = self.add_balls(2)
        self.park.InitializeBubbles(second.id)
        self.park.Orbit(first.id, second.id)
        self.assertNotEqual(first.followId, second.id)
        self.assertNotEqual(first.followRange, 1.0)
        self.assertNotEqual(first.followRange, 1.0)
        self.assertNotEqual(first.mode, destiny.DSTBALL_ORBIT)
        self.assertNotIn(first.id, self.park.GetFollowers(second.id))

class TestStop(helpers.BallparkTestCase):
    def test_stop_stopped_ball(self):
        ball, = self.add_balls(1)
        self.assertEqual(ball.mode, destiny.DSTBALL_STOP)

    def test_stop_following_ball(self):
        first, second = self.add_balls(2)
        self.park.FollowBall(first.id, second.id)
        self.assertEqual(first.effectStamp, 0)
        self.park.Stop(first.id)
        self.assertEqual(first.mode, destiny.DSTBALL_STOP)
        self.assertNotIn(first.id, self.park.GetFollowers(second.id))
        self.assertEqual(first.followId, 0)
        self.assertEqual(first.followRange, 0)

    def test_stop_orbiting_ball(self):
        first, second = self.add_balls(2)
        self.park.Orbit(first.id, second.id)
        self.assertEqual(first.effectStamp, 0)
        self.park.Stop(first.id)
        self.assertEqual(first.mode, destiny.DSTBALL_STOP)
        self.assertNotIn(first.id, self.park.GetFollowers(second.id))
        self.assertEqual(first.followId, 0)
        self.assertEqual(first.followRange, 0)

    def test_stop_missile(self):
        src, dst, owner = self.add_balls(3)
        self.park.MissileFollow(src.id, dst.id, owner.id)
        self.park.Stop(src.id)
        self.assertEqual(src.mode, destiny.DSTBALL_STOP)
        self.assertNotIn(src.id, self.park.GetFollowers(dst.id))
        self.assertEqual(src.followId, 0)
        self.assertEqual(src.followRange, 0)

    def test_stop_ball_in_formation(self):
        src, dst = self.add_balls(2)
        self.park.LoadFormations(helpers.FORMATIONS)
        self.park.SetBallFormation(dst.id, 1)
        self.park.FormationFollow(src.id, dst.id, 1)
        self.park.Stop(src.id)
        self.assertEqual(src.mode, destiny.DSTBALL_STOP)
        self.assertNotIn(src.id, self.park.GetFollowers(dst.id))
        self.assertEqual(src.followId, 0)
        self.assertEqual(src.followRange, 0)


class TestLaunchMissile(helpers.BallparkTestCase):
    def test_src_gets_launched_at_dst(self):
        src, dst, owner = self.add_balls(3)
        aimed_launch = False
        massive = False
        self.park.LaunchMissile(src.id, dst.id, owner.id, aimed_launch, massive)
        self.assertEqual(src.mode, destiny.DSTBALL_MISSILE)
        self.assertIn(src.id, self.park.GetFollowers(dst.id))

    def test_owner_of_missile_is_correct(self):
        src, dst, owner = self.add_balls(3)
        aimed_launch = False
        massive = False
        self.park.LaunchMissile(src.id, dst.id, owner.id, aimed_launch, massive)
        self.assertEqual(src.ownerId, owner.id)

    def test_massive_missile(self):
        src, dst, owner = self.add_balls(3)
        aimed_launch = False
        massive = True
        self.park.LaunchMissile(src.id, dst.id, owner.id, aimed_launch, massive)
        self.assertEqual(src.isMassive, True)

    def test_direction_of_non_aimed_missiles(self):
        src, dst, owner = self.add_balls(3)
        dst.x += 10000.0
        aimed_launch = False
        massive = False
        self.park.LaunchMissile(src.id, dst.id, owner.id, aimed_launch, massive)
        self.assertEqual(src.vx, 0.0)
        self.assertEqual(src.vy, 0.0)
        self.assertAlmostEqual(src.vz, -150.0, places=4)

    def test_additional_velocity_of_missile_is_the_same_as_the_max_velocity_of_its_owner_if_above_limit(self):
        src, dst, owner = self.add_balls(3)
        dst.x += 10000.0
        aimed_launch = False
        massive = False
        self.park.SetMaxSpeed(owner.id, 1234.0)
        self.park.LaunchMissile(src.id, dst.id, owner.id, aimed_launch, massive)
        self.assertEqual(src.vx, 0.0)
        self.assertEqual(src.vy, 0.0)
        self.assertAlmostEqual(src.vz, -1234.0, places=3)

    def test_initial_velocity_of_owner_has_impact_of_non_aimed_missile_direction_and_speed(self):
        src, dst, owner = self.add_balls(3)
        dst.x += 10000.0
        aimed_launch = False
        massive = False
        owner.vx = 100.0
        self.park.LaunchMissile(src.id, dst.id, owner.id, aimed_launch, massive)
        self.assertEqual((src.vx, src.vy, src.vz), (250.0, 0.0, 0.0))

    def test_direction_of_aimed_missiles(self):
        src, dst, owner = self.add_balls(3)
        dst.x += 10000.0
        aimed_launch = True
        massive = False
        self.park.LaunchMissile(src.id, dst.id, owner.id, aimed_launch, massive)
        self.assertEqual((src.vx, src.vy, src.vz), (1.0, 0.0, 0.0))


class TestFormationFollow(helpers.BallparkTestCase):
    def test_can_follow_in_formation(self):
        src, dst = self.add_balls(2)
        self.park.LoadFormations(helpers.FORMATIONS)
        self.park.SetBallFormation(dst.id, 1)
        self.park.FormationFollow(src.id, dst.id, 1)
        dst_followers = self.park.GetFollowers(dst.id)
        self.assertIn(src.id, dst_followers)
        self.assertEqual(src.mode, destiny.DSTBALL_FORMATION)


class TestEntityWarpIn(helpers.BallparkTestCase):
    def test_warps_immediately(self):
        src, = self.add_balls(1)
        x, y, z = (0.0, 0.0, 0.0)
        warpFactor = 1
        self.park.EntityWarpIn(src.id, x, y, z, warpFactor)
        self.assertEqual(src.mode, destiny.DSTBALL_WARP)

    def test_velocity_is_approximately_one_au(self):
        src, = self.add_balls(1)
        src.x, src.y, src.z = (0.0, 0.0, 0.0)
        x, y, z = (helpers.TEN_BILLION, 0.0, 0.0)
        warpFactor = 1
        self.park.EntityWarpIn(src.id, x, y, z, warpFactor)
        self.assertAlmostEqual(src.vx, helpers.AU)

    def test_ball_in_goto_mode_does_not_warp(self):
        src, = self.add_balls(1)
        x, y, z = (0.0, 0.0, 0.0)
        warpFactor = 1
        src.x, src.y, src.z = (1.0, 0.0, 0.0)
        self.park.GotoPoint(src.id, x, y, z)
        self.park.EntityWarpIn(src.id, x, y, z, warpFactor)
        self.assertNotEqual(src.mode, destiny.DSTBALL_WARP)
