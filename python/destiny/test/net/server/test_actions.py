import unittest

import destiny

from destiny.test.net.server.helpers import add_ball_to_park, DestinyTestCase
from destiny.net.server import Actions

BALL_ID_1 = 1000000000001
BALL_ID_2 = 1000000000002
BALL_ID_3 = 1000000000003
INITIAL_TIMESTAMP = 0


CORP_ID_1 = 98000001
ALLIANCE_ID_1 = 99000001


class TestActions(DestinyTestCase):
    def setUp(self):
        self._park = destiny.Ballpark()
        self._park.isMaster = True
        self._actions = Actions(self._park)

    def tearDown(self):
        self._park.ClearAll()
        del self._park
        del self._actions

    def assertHistoryEmpty(self):
        history = self._actions.flush_history()
        self.assertListEqual(
            [],
            history
        )

    def assertSingleActionHistory(self, ball_id, expected_action):
        history = self._actions.flush_history()
        self.assertListEqual(
            [(ball_id, INITIAL_TIMESTAMP, expected_action)],
            history
        )

    def assertMultiActionHistory(self, ball_action_list):
        history = self._actions.flush_history()
        self.assertListEqual(ball_action_list, history)

    def assertBallAddedToClientParks(self, ball, timestamp):
        history = self._actions.flush_history()
        for entry in history:
            ball_id, entry_timestamp, action = entry
            if ball_id != ball.id:
                continue
            if entry_timestamp != timestamp:
                continue
            actionName, parameters = action
            if actionName != "AddBallsToPark":
                continue
            self.assertEqual(1, len(parameters))
            bytes = parameters[0]
            park = self.read_balls_from_stream_into_park(bytes)
            loaded_ball = park.balls[ball.id]
            if self.ballsAreEqual(ball, loaded_ball, ignore_bubble_id=True):
                return
        self.assertFalse(True, "Ball not added to park (or was added at the wrong time).")

    def assertActionInHistory(self, ball_id, action, timestamp=INITIAL_TIMESTAMP):
        history = self._actions.flush_history()
        for entry in history:
            entry_ball_id, entry_timestamp, entry_action = entry
            if entry_ball_id != ball_id:
                continue
            if entry_action != action:
                continue
            if entry_timestamp != timestamp:
                continue
            return
        self.assertFalse(
            "Entry for action %s not found in history for ball %s at time %s" % (action, ball_id, timestamp)
        )

    def test_go_to_point(self):
        x, y, z = 100, 200, 300
        self._actions.go_to_point(BALL_ID_1, x, y, z)
        expected_action = ('GotoPoint', (BALL_ID_1, 100, 200, 300))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_duplicate_subsequent_actions_are_ignored(self):
        x, y, z = 100, 200, 300
        self._actions.go_to_point(BALL_ID_1, x, y, z)
        self._actions.go_to_point(BALL_ID_1, x, y, z)
        expected_action = ('GotoPoint', (BALL_ID_1, 100, 200, 300))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_duplicate_subsequent_actions_with_different_timestamps_are_not_ignored(self):
        x, y, z = 100, 200, 300
        self._actions.go_to_point(BALL_ID_1, x, y, z)
        self._actions.set_stamp_for_system(INITIAL_TIMESTAMP + 1)
        self._actions.go_to_point(BALL_ID_1, x, y, z)
        expected_action = ('GotoPoint', (BALL_ID_1, 100, 200, 300))
        self.assertMultiActionHistory(
            [
                (BALL_ID_1, INITIAL_TIMESTAMP, expected_action),
                (BALL_ID_1, INITIAL_TIMESTAMP + 1, expected_action)
            ]
        )

    def test_duplicate_non_subsequent_actions_are_not_ignored(self):
        x, y, z = 100, 200, 300
        self._actions.go_to_point(BALL_ID_1, x, y, z)
        self._actions.stop(BALL_ID_1)
        self._actions.go_to_point(BALL_ID_1, x, y, z)
        goto_action = ('GotoPoint', (BALL_ID_1, x, y, z))
        stop_action = ('Stop', (BALL_ID_1,))
        self.assertMultiActionHistory(
            [
                (BALL_ID_1, INITIAL_TIMESTAMP, goto_action),
                (BALL_ID_1, INITIAL_TIMESTAMP, stop_action),
                (BALL_ID_1, INITIAL_TIMESTAMP, goto_action)
            ]
        )

    def test_go_to_direction(self):
        x, y, z = 100, 200, 300
        self._actions.go_to_direction(BALL_ID_1, x, y, z)
        expected_action = ('GotoDirection', (BALL_ID_1, x, y, z))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_orbit(self):
        range_meters = 123
        self._actions.orbit(BALL_ID_1, BALL_ID_2, range_meters)
        expected_action = ("Orbit", (BALL_ID_1, BALL_ID_2, range_meters))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_set_ball_troll(self):
        tick_delay = 42
        self._actions.set_ball_troll(BALL_ID_1, tick_delay)
        expected_action = ("SetBallTroll", (BALL_ID_1, tick_delay))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_set_ball_velocity(self):
        vx, vy, vz = (333.3, 666.6, 999.9)
        self._actions.set_ball_velocity(BALL_ID_1, vx, vy, vz)
        expected_action = ("SetBallVelocity", (BALL_ID_1, vx, vy, vz))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_set_ball_massive(self):
        self._actions.set_ball_massive(BALL_ID_1, False)
        expected_action = ("SetBallMassive", (BALL_ID_1, False))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_stop(self):
        self._actions.stop(BALL_ID_1)
        expected_action = ("Stop", (BALL_ID_1,))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_set_ball_mass(self):
        mass_kg = 72.6
        self._actions.set_ball_mass(BALL_ID_1, mass_kg)
        expected_action = ("SetBallMass", (BALL_ID_1, mass_kg))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_set_ball_agility(self):
        agility = 3.141592
        self._actions.set_ball_agility(BALL_ID_1, agility)
        expected_action = ("SetBallAgility", (BALL_ID_1, agility))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_add_ball_to_client_parks(self):
        ball = add_ball_to_park(self._park)
        self._actions.add_ball_to_client_parks(ball.id)
        self.assertBallAddedToClientParks(ball, INITIAL_TIMESTAMP)

    def test_make_ball_local(self):
        ball = add_ball_to_park(self._park)
        self._actions.make_ball_local(ball.id)
        expected_action = ("BallNotGlobal", (ball.newBubbleId,))
        self.assertSingleActionHistory(ball.id, expected_action)

    def test_make_ball_global_makes_ball_global(self):
        ball = add_ball_to_park(self._park, is_global=False)
        self._actions.make_ball_global(ball.id)
        self.assertTrue(ball.isGlobal)

    def test_make_ball_global_adds_ball_to_client_parks(self):
        ball = add_ball_to_park(self._park, is_global=False)
        self._actions.make_ball_global(ball.id)
        self.assertBallAddedToClientParks(ball, INITIAL_TIMESTAMP)

    def test_make_ball_global_sends_out_signal_with_src_id(self):
        callback_parameters = []

        def callback(src_id):
            callback_parameters.append(src_id)

        self._actions.on_ball_made_global.connect(callback)
        ball = add_ball_to_park(self._park, is_global=False)
        self._actions.make_ball_global(ball.id)
        self.assertListEqual([ball.id], callback_parameters)

    def test_remove_ball_removes_ball_right_away(self):
        ball = add_ball_to_park(self._park, is_global=False)
        self._actions.remove_ball(ball.id)
        self.assertNotIn(ball.id, self._park.balls)

    def test_remove_ball_does_not_note_removal_of_non_global_ball(self):
        ball = add_ball_to_park(self._park, is_global=False)
        self._actions.remove_ball(ball.id)
        self.assertHistoryEmpty()

    def test_remove_ball_notifies_removal_of_global_ball(self):
        ball = add_ball_to_park(self._park, is_global=True)
        self._actions.remove_ball(ball.id)
        expected_action = ("RemoveGlobalBall", (ball.id,))
        self.assertSingleActionHistory(ball.id, expected_action)

    def test_set_speed_fraction(self):
        speed_fraction = 0.5
        self._actions.set_speed_fraction(BALL_ID_1, speed_fraction)
        expected_action = ("SetSpeedFraction", (BALL_ID_1, speed_fraction))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_warp_to(self):
        x, y, z = 123.0, 456.0, 789.0
        minimum_range = 1000.0
        warp_speed = 20
        self._actions.warp_to(BALL_ID_1, x, y, z, minimum_range, warp_speed)
        expected_action = ("WarpTo", (BALL_ID_1, x, y, z, minimum_range, warp_speed))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_entity_warp_in(self):
        x, y, z = 123.0, 456.0, 789.0
        warp_speed = 20
        self._actions.entity_warp_in(BALL_ID_1, x, y, z, warp_speed)
        expected_action = ("EntityWarpIn", (BALL_ID_1, x, y, z, warp_speed))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_set_ball_position_local_ball(self):
        x, y, z = 123.0, 456.0, 789.0
        self._actions.set_ball_position(BALL_ID_1, x, y, z)
        expected_action = ("SetBallPosition", (BALL_ID_1, x, y, z), False)
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_set_ball_position_not_local_ball(self):
        x, y, z = 123.0, 456.0, 789.0
        self._actions.set_ball_position(BALL_ID_1, x, y, z, is_local_ball=True)
        expected_action = ("SetBallPosition", (BALL_ID_1, x, y, z), True)
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_set_ball_harmonic_on_forcefield_with_all_negative_one_values_makes_field_non_massive(self):
        harmonic_value = -1
        corporation_id = -1
        alliance_id = -1
        self._actions.set_ball_harmonic(BALL_ID_1, harmonic_value, corporation_id, alliance_id, is_forcefield=True)
        expected_action = ("SetBallMassive", (BALL_ID_1, False))
        self.assertActionInHistory(BALL_ID_1, expected_action)

    def test_set_ball_harmonic_on_forcefield_with_harmonic_value_makes_field_massive(self):
        harmonic_value = 0
        corporation_id = -1
        alliance_id = -1
        self._actions.set_ball_harmonic(BALL_ID_1, harmonic_value, corporation_id, alliance_id, is_forcefield=True)
        expected_action = ("SetBallMassive", (BALL_ID_1, True))
        self.assertActionInHistory(BALL_ID_1, expected_action)

    def test_set_ball_harmonic_on_forcefield_with_corporation_id_makes_field_massive(self):
        harmonic_value = -1
        corporation_id = CORP_ID_1
        alliance_id = -1
        self._actions.set_ball_harmonic(BALL_ID_1, harmonic_value, corporation_id, alliance_id, is_forcefield=True)
        expected_action = ("SetBallMassive", (BALL_ID_1, True))
        self.assertActionInHistory(BALL_ID_1, expected_action)

    def test_set_ball_harmonic_on_forcefield_with_alliance_id_makes_field_massive(self):
        harmonic_value = -1
        corporation_id = -1
        alliance_id = ALLIANCE_ID_1
        self._actions.set_ball_harmonic(BALL_ID_1, harmonic_value, corporation_id, alliance_id, is_forcefield=True)
        expected_action = ("SetBallMassive", (BALL_ID_1, True))
        self.assertActionInHistory(BALL_ID_1, expected_action)

    def test_set_ball_harmonic_on_forcefield_sets_harmonic_values(self):
        harmonic_value = 12345
        corporation_id = CORP_ID_1
        alliance_id = ALLIANCE_ID_1
        self._actions.set_ball_harmonic(BALL_ID_1, harmonic_value, corporation_id, alliance_id, is_forcefield=True)
        expected_action = ("SetBallHarmonic", (BALL_ID_1, harmonic_value, corporation_id, alliance_id, True))
        self.assertActionInHistory(BALL_ID_1, expected_action)

    def test_set_ball_harmonic_on_non_forcefield_sets_harmonic_values(self):
        harmonic_value = 12345
        corporation_id = CORP_ID_1
        alliance_id = ALLIANCE_ID_1
        self._actions.set_ball_harmonic(BALL_ID_1, harmonic_value, corporation_id, alliance_id, is_forcefield=False)
        expected_action = ("SetBallHarmonic", (BALL_ID_1, harmonic_value, corporation_id, alliance_id, False))
        self.assertActionInHistory(BALL_ID_1, expected_action)

    def test_set_ball_radius(self):
        radius_meters = 42.0
        self._actions.set_ball_radius(BALL_ID_1, radius_meters)
        expected_action = ("SetBallRadius", (BALL_ID_1, radius_meters))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_set_ball_free(self):
        self._actions.set_ball_free(BALL_ID_1)
        expected_action = ("SetBallFree", (BALL_ID_1, True))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_set_ball_not_free(self):
        self._actions.set_ball_free(BALL_ID_1, False)
        expected_action = ("SetBallFree", (BALL_ID_1, False))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_set_ball_interactive(self):
        self._actions.set_ball_interactive(BALL_ID_1, True)
        expected_action = ("SetBallInteractive", (BALL_ID_1, True))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_set_ball_not_interactive(self):
        self._actions.set_ball_interactive(BALL_ID_1, False)
        expected_action = ("SetBallInteractive", (BALL_ID_1, False))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_follow_ball(self):
        range_meters = 1000.0
        self._actions.follow_ball(BALL_ID_1, BALL_ID_2, range_meters)
        expected_action = ("FollowBall", (BALL_ID_1, BALL_ID_2, range_meters))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_set_max_speed(self):
        max_meters_per_second = 500.0
        self._actions.set_max_speed(BALL_ID_1, max_meters_per_second)
        expected_action = ("SetMaxSpeed", (BALL_ID_1, max_meters_per_second))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_cloak_nonexistent_ball_should_fail(self):
        uncloak_range_meters = 1000.0
        success = self._actions.cloak_ball(BALL_ID_1, destiny.DSTNORMALCLOAK, uncloak_range_meters)
        self.assertFalse(success)
        self.assertHistoryEmpty()

    def test_cloak_existing_ball(self):
        ball = add_ball_to_park(self._park)
        uncloak_range_meters = 1000.0
        cloak_mode = destiny.DSTNORMALCLOAK
        success = self._actions.cloak_ball(ball.id, cloak_mode, uncloak_range_meters)
        self.assertTrue(success)
        expected_action = ("CloakBall", (ball.id, cloak_mode, uncloak_range_meters), ball.oldBubbleId)
        self.assertSingleActionHistory(ball.id, expected_action)

    def test_uncloak_ball(self):
        self._actions.uncloak_ball(BALL_ID_1)
        expected_action = ("UncloakBall", (BALL_ID_1,))
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_launch_missile(self):
        src_id = BALL_ID_1
        dst_id = BALL_ID_2
        owner_id = BALL_ID_3
        is_aimed_launch = True
        is_missile_massive = False
        self._actions.launch_missile(src_id, dst_id, owner_id, is_aimed_launch, is_missile_massive)
        expected_action = ("LaunchMissile", (src_id, dst_id, owner_id, is_aimed_launch, is_missile_massive))
        self.assertSingleActionHistory(src_id, expected_action)

    def test_clean_up_followers(self):
        follower = add_ball_to_park(self._park, ball_id=1)
        followee = add_ball_to_park(self._park, ball_id=2)
        self._park.FollowBall(follower.id, followee.id, 1000)
        self._park.SetBallPosition(follower.id, 500000000, 0, 0)
        remote = self._park.GetRemoteFollowers()
        self.assertEqual(1, len(remote))
        self._actions.clean_up_followers()
        expected_action = ("Stop", (follower.id,))
        self.assertSingleActionHistory(follower.id, expected_action)

    def test_undo_pending_cloak(self):
        ball = add_ball_to_park(self._park)
        self._actions.cloak_ball(ball.id, destiny.DSTNORMALCLOAK, 1000.0)
        self._actions.undo_pending_cloak(ball.id)
        self.assertHistoryEmpty()

    def test_undo_pending_cloak_ignores_other_balls(self):
        ball = add_ball_to_park(self._park)
        uncloak_range_meters = 1000.0
        cloak_mode = destiny.DSTNORMALCLOAK
        self._actions.cloak_ball(ball.id, cloak_mode, uncloak_range_meters)
        self._actions.undo_pending_cloak(ball.id + 1)
        expected_action = ("CloakBall", (ball.id, cloak_mode, uncloak_range_meters), ball.oldBubbleId)
        self.assertSingleActionHistory(ball.id, expected_action)

    def test_undo_pending_cloak_ignores_other_actions(self):
        radius_meters = 42.0
        self._actions.set_ball_radius(BALL_ID_1, radius_meters)
        expected_action = ("SetBallRadius", (BALL_ID_1, radius_meters))
        self._actions.undo_pending_cloak(BALL_ID_1)
        self.assertSingleActionHistory(BALL_ID_1, expected_action)

    def test_has_pending_cloak_returns_false_when_no_pending_cloak(self):
        has_pending_cloak = self._actions.has_pending_cloak(BALL_ID_1)
        self.assertFalse(has_pending_cloak)

    def test_has_pending_cloak_returns_true_when_pending_cloak(self):
        ball = add_ball_to_park(self._park)
        self._actions.cloak_ball(ball.id, destiny.DSTNORMALCLOAK, 1000.0)
        has_pending_cloak = self._actions.has_pending_cloak(ball.id)
        self.assertTrue(has_pending_cloak)

    def test_has_pending_uncloak_returns_false_when_no_pending_uncloak(self):
        has_pending_uncloak = self._actions.has_pending_uncloak(BALL_ID_1)
        self.assertFalse(has_pending_uncloak)

    def test_has_pending_uncloak_returns_true_when_pending_uncloak(self):
        self._actions.uncloak_ball(BALL_ID_1)
        has_pending_uncloak = self._actions.has_pending_uncloak(BALL_ID_1)
        self.assertTrue(has_pending_uncloak)

    def test_get_pending_cloak_mode_returns_none_when_ball_does_not_exist(self):
        cloak_mode = self._actions.get_pending_cloak_mode(BALL_ID_1)
        self.assertIsNone(cloak_mode)

    def test_get_pending_cloak_mode_returns_existing_mode_when_no_cloak_pending(self):
        ball = add_ball_to_park(self._park)
        cloak_mode = self._actions.get_pending_cloak_mode(ball.id)
        self.assertEqual(0, cloak_mode)

    def test_get_pending_cloak_mode_returns_pending_cloak_mode_when_no_cloak_pending(self):
        ball = add_ball_to_park(self._park)
        self._actions.cloak_ball(ball.id, destiny.DSTNORMALCLOAK, 1000.0)
        cloak_mode = self._actions.get_pending_cloak_mode(ball.id)
        self.assertEqual(destiny.DSTNORMALCLOAK, cloak_mode)

    def test_has_pending_set_free_returns_false_when_freedom_not_pending(self):
        freedom_pending = self._actions.has_pending_set_free(BALL_ID_1)
        self.assertFalse(freedom_pending)

    def test_has_pending_set_free_returns_false_when_freedom_not_pending_and_ball_already_free(self):
        ball = add_ball_to_park(self._park)
        self._park.SetBallFree(ball.id, True)
        freedom_pending = self._actions.has_pending_set_free(ball.id)
        self.assertFalse(freedom_pending)

    def test_has_pending_set_free_returns_true_when_freedom_pending(self):
        self._actions.set_ball_free(BALL_ID_1, True)
        freedom_pending = self._actions.has_pending_set_free(BALL_ID_1)
        self.assertTrue(freedom_pending)

    def test_has_pending_set_not_free_returns_false_when_freedom_removal_not_pending(self):
        freedom_removal_pending = self._actions.has_pending_set_not_free(BALL_ID_1)
        self.assertFalse(freedom_removal_pending)

    def test_has_pending_set_not_free_returns_false_when_freedom_removal_not_pending_and_ball_already_not_free(self):
        ball = add_ball_to_park(self._park)
        self._park.SetBallFree(ball.id, False)
        freedom_removal_pending = self._actions.has_pending_set_not_free(ball.id)
        self.assertFalse(freedom_removal_pending)

    def test_has_pending_set_not_free_returns_true_when_freedom_removal_pending(self):
        self._actions.set_ball_free(BALL_ID_1, False)
        freedom_removal_pending = self._actions.has_pending_set_not_free(BALL_ID_1)
        self.assertTrue(freedom_removal_pending)

    def test_get_pending_interactive_state_returns_none_when_ball_does_not_exist(self):
        state = self._actions.get_pending_interactive_state(BALL_ID_1)
        self.assertIsNone(state)

    def test_get_pending_interactive_state_returns_current_state_when_ball_exists(self):
        ball = add_ball_to_park(self._park, is_interactive=True)
        state = self._actions.get_pending_interactive_state(ball.id)
        self.assertEqual(True, state)

    def test_get_pending_interactive_state_returns_pending_state_when_available(self):
        ball = add_ball_to_park(self._park, is_interactive=True)
        self._actions.set_ball_interactive(ball.id, False)
        state = self._actions.get_pending_interactive_state(ball.id)
        self.assertEqual(False, state)

    def test_get_pending_speed_fraction_returns_none_when_ball_does_not_exist(self):
        speed_fraction = self._actions.get_pending_speed_fraction(BALL_ID_1)
        self.assertIsNone(speed_fraction)

    def test_get_pending_speed_fraction_returns_current_state_when_ball_exists(self):
        ball = add_ball_to_park(self._park, speed_fraction=0.5)
        speed_fraction = self._actions.get_pending_speed_fraction(ball.id)
        self.assertEqual(0.5, speed_fraction)

    def test_get_pending_speed_fraction_returns_pending_state_when_available(self):
        ball = add_ball_to_park(self._park, speed_fraction=0.5)
        self._actions.set_speed_fraction(ball.id, 1.0)
        speed_fraction = self._actions.get_pending_speed_fraction(ball.id)
        self.assertEqual(1.0, speed_fraction)
