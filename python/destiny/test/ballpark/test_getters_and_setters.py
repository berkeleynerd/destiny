import destiny
from destiny.test import helpers

class TestSetters(helpers.BallparkTestCase):
    def test_can_set_ball_mass(self):
        ball, = self.add_balls(1)
        self.park.SetBallMass(ball.id, 3.14)
        self.assertEqual(ball.mass, 3.14)

    def test_setting_ball_mass_to_a_negative_value_is_ineffective(self):
        ball, = self.add_balls(1)
        self.park.SetBallMass(ball.id, -1.0)
        self.assertEqual(ball.mass, 0.0)

    def test_can_set_ball_radius(self):
        ball, = self.add_balls(1)
        self.park.SetBallRadius(ball.id, 3.14)
        self.assertAlmostEquals(ball.radius, 3.14, places=6)

    def test_setting_ball_radius_to_negative_value_is_ineffective(self):
        ball, = self.add_balls(1)
        self.park.SetBallRadius(ball.id, -1.0)
        self.assertEquals(ball.radius, 0.0)

    def test_can_set_max_speed(self):
        ball, = self.add_balls(1)
        self.park.SetMaxSpeed(ball.id, 3.14)
        self.assertAlmostEquals(ball.maxVelocity, 3.14, places=6)

    def test_setting_max_speed_to_negative_value_is_ineffective(self):
        ball, = self.add_balls(1)
        self.park.SetMaxSpeed(ball.id, -1.0)
        self.assertEquals(ball.maxVelocity, 0.0)

    def test_can_set_ball_position(self):
        ball, = self.add_balls(1)
        x, y, z = 1.0, 2.0, 3.0
        self.park.SetBallPosition(ball.id, x, y, z)
        self.assertAlmostEquals(ball.x, x)
        self.assertAlmostEquals(ball.y, y)
        self.assertAlmostEquals(ball.z, z)

    def test_can_set_ball_velocity(self):
        ball, = self.add_balls(1)
        x, y, z = 1.0, 2.0, 3.0
        self.park.SetBallVelocity(ball.id, x, y, z)
        self.assertAlmostEquals(ball.vx, x)
        self.assertAlmostEquals(ball.vy, y)
        self.assertAlmostEquals(ball.vz, z)

    def test_can_set_speed_fraction(self):
        ball, = self.add_balls(1)
        self.park.SetSpeedFraction(ball.id, 0.2)
        self.assertAlmostEquals(ball.speedFraction, 0.2)

    def test_setting_speed_fraction_clamps_upper_bound_at_one(self):
        ball, = self.add_balls(1)
        self.park.SetSpeedFraction(ball.id, 2.0)
        self.assertAlmostEquals(ball.speedFraction, 1.0)

    def test_setting_speed_fraction_clamps_lowe_bound_at_zero(self):
        ball, = self.add_balls(1)
        self.park.SetSpeedFraction(ball.id, -2.0)
        self.assertAlmostEquals(ball.speedFraction, 0.0)

    def test_can_set_ball_free(self):
        ball, = self.add_balls(1)
        self.park.SetBallFree(ball.id, True)
        self.assertEqual(ball.isFree, True)

    def test_can_set_ball_massive(self):
        ball, = self.add_balls(1)
        self.park.SetBallMassive(ball.id, True)
        self.assertEqual(ball.isMassive, True)

    def test_can_set_ball_global(self):
        ball, = self.add_balls(1)
        self.park.SetBallGlobal(ball.id, True)
        self.assertEqual(ball.isGlobal, True)

    def test_can_set_ball_agility(self):
        ball, = self.add_balls(1)
        self.park.SetBallAgility(ball.id, 3.14)
        self.assertAlmostEquals(ball.Agility, 3.14, places=6)

    def test_setting_agility_to_negative_value_is_ineffective(self):
        ball, = self.add_balls(1)
        self.park.SetBallAgility(ball.id, -1.0)
        self.assertAlmostEquals(ball.Agility, 1.0)

    def test_can_set_ball_interactive(self):
        ball, = self.add_balls(1)
        self.park.SetBallInteractive(ball.id, True)
        self.assertTrue(ball.isInteractive)

    def test_setting_ball_velocity_updates_yaw_pitch_and_roll(self):
        ball, = self.add_balls(1)
        x, y, z = 1.0, 2.0, 3.0
        self.park.SetBallVelocity(ball.id, x, y, z)
        self.assertAlmostEquals(ball.yaw, 0.3217505543966422)
        self.assertAlmostEquals(ball.pitch, -0.5639426413606289)
        self.assertAlmostEquals(ball.roll, 0.0)

    def test_set_boid_true(self):
        ball, = self.add_balls(1)
        self.park.SetBoid(ball.id, True)
        self.assertEqual(ball.mode, destiny.DSTBALL_BOID)

    def test_set_boid_false(self):
        ball, = self.add_balls(1)
        self.park.SetBoid(ball.id, True)
        self.park.SetBoid(ball.id, False)
        self.assertEqual(ball.mode, destiny.DSTBALL_STOP)

    def test_set_ball_troll_no_delay(self):
        ball, = self.add_balls(1)
        self.park.SetBallTroll(ball.id, 0)
        self.assertEqual(ball.mode, destiny.DSTBALL_TROLL)

    def test_set_ball_troll_with_delay(self):
        ball, = self.add_balls(1)
        delay = 3
        self.park.SetBallTroll(ball.id, delay)
        self.assertEqual(ball.mode, destiny.DSTBALL_TROLL)
        self.assertEqual(ball.effectStamp, delay)

    def test_set_ball_rigid(self):
        ball, = self.add_balls(1)
        self.park.SetBallRigid(ball.id)
        self.assertEqual(ball.mode, destiny.DSTBALL_RIGID)

    def test_can_set_ball_harmonic_not_forcefield(self):
        ball, = self.add_balls(1)
        harmonic = 1
        corportationID = 2
        allianceID = 3
        force_field = False
        self.park.SetBallHarmonic(ball.id, harmonic, corportationID, allianceID, force_field)
        self.assertEqual(ball.harmonic, harmonic)
        self.assertEqual(ball.corporationID, corportationID)
        self.assertEqual(ball.allianceID, allianceID)
        self.assertEquals(ball.mode, destiny.DSTBALL_STOP)

    def test_can_set_ball_harmonic_make_forcefield(self):
        ball, = self.add_balls(1)
        harmonic = 1
        corportationID = 2
        allianceID = 3
        force_field = True
        self.park.SetBallHarmonic(ball.id, harmonic, corportationID, allianceID, force_field)
        self.assertEqual(ball.harmonic, harmonic)
        self.assertEqual(ball.corporationID, corportationID)
        self.assertEqual(ball.allianceID, allianceID)
        self.assertEquals(ball.mode, destiny.DSTBALL_FIELD)

    def test_can_set_ball_harmonic_remove_forcefield(self):
        ball, = self.add_balls(1)
        harmonic = 1
        corportationID = 2
        allianceID = 3
        self.park.SetBallHarmonic(ball.id, harmonic, corportationID, allianceID, True)
        self.park.SetBallHarmonic(ball.id, harmonic, corportationID, allianceID, False)
        self.assertEquals(ball.mode, destiny.DSTBALL_STOP)


class TestGetAccuracy(helpers.BallparkTestCase):
    def test_accuracy_is_zero_when_distance_is_zero(self):
        src, dst = self.add_balls(2)
        fallOff = 1.0
        optimalRange = 1.0
        trackingSpeed = 1.0
        signatureRadius = 1.0
        optimalSignatureRadius = 1.0
        accuracy, returned_optimal_range, returned_tracking_speed = self.park.GetAccuracy(
            src.id,
            dst.id,
            optimalRange,
            fallOff,
            trackingSpeed,
            signatureRadius,
            optimalSignatureRadius)
        self.assertEqual(accuracy, 0.0)

    def test_accuracy_is_zero_when_ball_targets_itself(self):
        src, = self.add_balls(1)
        accuracy, returned_optimal_range, returned_tracking_speed = self.park.GetAccuracy(src.id,src.id,1.0,1.0,1.0,1.0,1.0)
        self.assertEqual(accuracy, 0)

    def test_full_accuracy(self):
        src, dst = self.add_balls(2)
        dst.x += 1.0
        fallOff = 1.0
        optimalRange = 1.0
        trackingSpeed = 1.0
        signatureRadius = 1.0
        optimalSignatureRadius = 1.0
        accuracy, returned_optimal_range, returned_tracking_speed = self.park.GetAccuracy(
            src.id,
            dst.id,
            optimalRange,
            fallOff,
            trackingSpeed,
            signatureRadius,
            optimalSignatureRadius)
        self.assertEqual(accuracy, 1.0)

    def test_no_falloff(self):
        src, dst = self.add_balls(2)
        dst.x += 2.0
        fallOff = 1.0
        optimalRange = 1.0
        trackingSpeed = 1.0
        signatureRadius = 1.0
        optimalSignatureRadius = 1.0
        accuracy, returned_optimal_range, returned_tracking_speed = self.park.GetAccuracy(
            src.id,
            dst.id,
            optimalRange,
            fallOff,
            trackingSpeed,
            signatureRadius,
            optimalSignatureRadius)
        self.assertEqual(accuracy, 0.5)

    def test_some_falloff(self):
        src, dst = self.add_balls(2)
        dst.x += 2.0
        fallOff = 0.95
        optimalRange = 1.0
        trackingSpeed = 1.0
        signatureRadius = 1.0
        optimalSignatureRadius = 1.0
        accuracy, returned_optimal_range, returned_tracking_speed = self.park.GetAccuracy(
            src.id,
            dst.id,
            optimalRange,
            fallOff,
            trackingSpeed,
            signatureRadius,
            optimalSignatureRadius)
        self.assertAlmostEqual(accuracy, 0.46392604883716854)

    def test_combination_parameters(self):
        src, dst = self.add_balls(2)
        dst.x += 2.0
        fallOff = 0.95
        optimalRange = 0.5
        trackingSpeed = 0.6
        signatureRadius = 0.2
        optimalSignatureRadius = 0.7
        accuracy, returned_optimal_range, returned_tracking_speed = self.park.GetAccuracy(
            src.id,
            dst.id,
            optimalRange,
            fallOff,
            trackingSpeed,
            signatureRadius,
            optimalSignatureRadius)
        self.assertAlmostEqual(accuracy, 0.17762729659881724)

    def test_returns_accuracy_and_tracking_speed_that_we_specify(self):
        src, dst = self.add_balls(2)
        fallOff = 1.0
        optimalRange = 42.0
        trackingSpeed = 3.14
        signatureRadius = 1.0
        optimalSignatureRadius = 1.0
        accuracy, returned_optimal_range, returned_tracking_speed = self.park.GetAccuracy(
            src.id,
            dst.id,
            optimalRange,
            fallOff,
            trackingSpeed,
            signatureRadius,
            optimalSignatureRadius)
        self.assertEqual(returned_optimal_range, optimalRange)
        self.assertEqual(returned_tracking_speed, trackingSpeed)


class TestGetSurfaceDist(helpers.BallparkTestCase):
    def test_surface_distance_between_two_balls_in_the_same_place_with_no_radius_is_zero(self):
        ball, other = self.add_balls(2)
        result = self.park.GetSurfaceDist(ball.id, other.id)
        self.assertEqual(result, 0.0)

    def test_surface_distance_between_two_balls_with_no_radius_equals_the_distance_between_the_balls(self):
        ball, other = self.add_balls(2)
        other.x += 100.0
        result = self.park.GetSurfaceDist(ball.id, other.id)
        self.assertEqual(result, 100.0)

    def test_surface_distance_between_two_balls_equals_the_distance_between_the_balls_minus_their_combined_radii(self):
        ball, other = self.add_balls(2)
        ball.radius = 10.0
        other.radius = 5.0
        other.x += 100.0
        result = self.park.GetSurfaceDist(ball.id, other.id)
        self.assertEqual(result, 85.0)

    def test_euclidean_distance(self):
        ball, other = self.add_balls(2)
        other.x += 1.0
        other.y += 2.0
        other.z += 3.0
        result = self.park.GetSurfaceDist(ball.id, other.id)
        self.assertAlmostEqual(result, 3.7416573867739413)


class TestGetCenterDist(helpers.BallparkTestCase):
    def test_center_distance_between_two_balls_in_the_same_place(self):
        ball, other = self.add_balls(2)
        result = self.park.GetCenterDist(ball.id, other.id)
        self.assertEqual(result, 0.0)

    def test_center_distance_between_two_balls(self):
        ball, other = self.add_balls(2)
        other.x += 100.0
        result = self.park.GetCenterDist(ball.id, other.id)
        self.assertEqual(result, 100.0)


class TestGetRemoteFollowers(helpers.BallparkTestCase):
    def test_returns_empty_list_when_there_are_none(self):
        followers = self.park.GetRemoteFollowers()
        self.assertListEqual(followers, [])

    def test_ignores_ball_that_is_not_free(self):
        src, dst = self.add_balls(2)
        self.park.SetBallFree(src.id, False)
        self.park.FollowBall(src.id, dst.id)
        dst.x = 0
        self.park.InitializeBubbles()
        followers = self.park.GetRemoteFollowers()
        self.assertListEqual(followers, [])

    def test_finds_following_ball(self):
        src, dst = self.add_balls(2)
        self.park.SetBallFree(src.id, True)
        self.park.FollowBall(src.id, dst.id)
        dst.x = 0
        self.park.InitializeBubbles()
        followers = self.park.GetRemoteFollowers()
        self.assertListEqual(followers, [src.id])

    def test_ignores_balls_that_are_not_following(self):
        src, dst = self.add_balls(2)
        self.park.SetBallFree(src.id, True)
        dst.x = 0
        self.park.InitializeBubbles()
        followers = self.park.GetRemoteFollowers()
        self.assertListEqual(followers, [])

    def test_ignores_balls_that_are_in_the_same_bubble(self):
        src, dst = self.add_balls(2)
        self.park.SetBallFree(src.id, True)
        self.park.FollowBall(src.id, dst.id)
        self.park.InitializeBubbles()
        followers = self.park.GetRemoteFollowers()
        self.assertListEqual(followers, [])


class TestGetFollowers(helpers.BallparkTestCase):
    def test_returns_empty_list_when_there_are_no_followers(self):
        ball, = self.add_balls(1)
        followers = self.park.GetFollowers(ball.id)
        self.assertListEqual(followers, [])

    def test_returns_list_of_followers(self):
        src, dst = self.add_balls(2)
        self.park.FollowBall(src.id, dst.id)
        followers = self.park.GetFollowers(dst.id)
        self.assertListEqual(followers, [src.id])


class TestGetCurrentEgoPos(helpers.BallparkTestCase):
    def test_returns_default_initial_ego_value(self):
        ego_pos = self.park.GetCurrentEgoPos()
        self.assertEqual(ego_pos, (0.0, 0.0, 0.0))


class TestSetBallFormation(helpers.BallparkTestCase):
    def test_valid_formation_gets_set(self):
        ball, = self.add_balls(1)
        self.park.LoadFormations(helpers.FORMATIONS)
        self.park.SetBallFormation(ball.id, 0)
        self.assertEqual(ball.formationID, 0)

    def test_formation_out_of_range_does_not_get_set(self):
        ball, = self.add_balls(1)
        self.park.LoadFormations(helpers.FORMATIONS)
        self.park.SetBallFormation(ball.id, len(helpers.FORMATIONS))
        self.assertEqual(ball.formationID, 255)

    def test_followers_whom_we_no_longer_have_room_for_get_dropped(self):
        src, dst = self.add_balls(2)
        self.park.LoadFormations(helpers.FORMATIONS)
        self.park.SetBallFormation(dst.id, 1)
        self.park.FormationFollow(src.id, dst.id, 1)
        self.park.SetBallFormation(dst.id, 2)
        self.assertEqual(src.mode, destiny.DSTBALL_STOP)
