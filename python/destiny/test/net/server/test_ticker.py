import mock

import destiny
from destiny.test.net.server.helpers import add_ball_to_park, DestinyTestCase

from destiny.net.server import Ticker, Actions, ParkUpdateBatcher, BubbleUpdater
from destiny.test.net.server.helpers import TestClientInterests, TestCharacterInterests, TestNetwork, TestBallInfo


BALL_ID_1 = 1001
BALL_ID_2 = 1002
BALL_ID_3 = 1003

CHAR_1 = 90000128
CHAR_2 = 90000256
CHAR_3 = 90000512

CLIENT_1 = 13579
CLIENT_2 = 24680
CLIENT_3 = 80808


class TestTicker(DestinyTestCase):
    def setUp(self):
        self._park = destiny.Ballpark()
        self._park.isMaster = True
        self._ball_info = TestBallInfo()
        self._actions = Actions(self._park)
        self._network = TestNetwork()
        self._character_interests = TestCharacterInterests()
        self._client_interests = TestClientInterests()
        self._bubble_updater = BubbleUpdater(self._park)
        self._update_batcher = ParkUpdateBatcher(
            self._park,
            self._network,
            self._character_interests,
            self._client_interests,
            self._bubble_updater
        )
        self._ticker = Ticker(
            self._park,
            self._ball_info,
            self._actions,
            self._update_batcher
        )

    def tearDown(self):
        del self._ticker
        del self._update_batcher
        del self._bubble_updater
        del self._actions
        del self._park

    def test_applies_action(self):
        ball = add_ball_to_park(self._park, ball_id=BALL_ID_1)
        self._actions.go_to_point(BALL_ID_1, 10000.0, 0.0, 0.0)
        self._ticker.tick()
        self.assertEqual(destiny.DSTBALL_GOTO, ball.mode)

    def test_cloak_ball_cloaks_ball(self):
        ball = add_ball_to_park(self._park, ball_id=BALL_ID_1)
        self._actions.cloak_ball(BALL_ID_1, destiny.DSTNORMALCLOAK, 1000.0)
        self._ticker.tick()
        self.assertEqual(destiny.DSTNORMALCLOAK, ball.isCloaked)

    def test_cloak_ball_emits_signal(self):
        callback = mock.Mock()
        self._ticker.on_ball_cloaking.connect(callback)
        add_ball_to_park(self._park, ball_id=BALL_ID_1)
        self._actions.cloak_ball(BALL_ID_1, destiny.DSTNORMALCLOAK, 1000.0)
        self._ticker.tick()
        callback.assert_called_once_with(BALL_ID_1)

    def assertUpdateNarrowcast(self, client_ids, action, timestamp=None):
        if timestamp is None:
            timestamp = self._ticker.stamp_for_system - 1
        for entry in self._network.narrowcasts:
            clients, method_name, actions, wait_for_bubble, casts_in_batch = entry
            if method_name != "DoDestinyUpdate":
                continue
            if set(clients) != set(client_ids):
                continue
            if (timestamp, action) in actions:
                return
        self.assertTrue(False, "Could not find specified action in narrowcasts")

    def assertUpdateSinglecast(self, client_id, action, timestamp=None):
        if timestamp is None:
            timestamp = self._ticker.stamp_for_system - 1
        for entry in self._network.singlecasts:
            entry_client_id, method_name, actions, wait_for_bubble, casts_in_batch = entry
            if method_name != "DoDestinyUpdate":
                continue
            if entry_client_id != client_id:
                continue
            if (timestamp, action) in actions:
                return
        self.assertTrue(False, "Could not find specified action in singlecasts")

    def _set_up_ball(self, ball_id, char_id, client_id):
        ball = add_ball_to_park(self._park, ball_id=ball_id)
        self._client_interests.add_client_interest(ball_id, client_id)
        self._character_interests.add_interest(ball_id, char_id)
        self._character_interests.map_to_client(char_id, client_id)
        self._ball_info.set_characters_for_ball(ball_id, [char_id])
        return ball

    def _set_up_single_ball(self):
        return self._set_up_ball(BALL_ID_1, CHAR_1, CLIENT_1)

    def _set_up_two_balls_in_same_bubble(self):
        ball_one = self._set_up_ball(BALL_ID_1, CHAR_1, CLIENT_1)
        ball_two = self._set_up_ball(BALL_ID_2, CHAR_2, CLIENT_2)
        return ball_one, ball_two

    def _set_up_three_balls_in_same_bubble(self):
        ball_one, ball_two = self._set_up_two_balls_in_same_bubble()
        ball_three = self._set_up_ball(BALL_ID_3, CHAR_3, CLIENT_3)
        return ball_one, ball_two, ball_three

    def test_cloak_ball_event_narrowcast_to_clients_in_bubble(self):
        self._set_up_two_balls_in_same_bubble()
        self._perform_tick()
        uncloak_range_meters = 1000.0
        self._actions.cloak_ball(BALL_ID_1, destiny.DSTNORMALCLOAK, uncloak_range_meters)
        self._perform_tick()
        self.assertUpdateNarrowcast(
            [CLIENT_1, CLIENT_2],
            ('CloakBall', (BALL_ID_1, destiny.DSTNORMALCLOAK, uncloak_range_meters))
        )

    def test_cloaking_and_uncloaking_in_same_tick_does_not_uncloak_ball(self):
        """
        This ends up happening because all actions except cloak ball are applied
        together before updating the bubbles. Since only the CloakBall action
        gets applied after the balls are updated, this results in an unintuitive
        result when cloaking, then uncloaking a ball in the same tick.
        """
        self._set_up_single_ball()
        ball = add_ball_to_park(self._park, ball_id=BALL_ID_1)
        self._actions.cloak_ball(BALL_ID_1, destiny.DSTNORMALCLOAK, 1000.0)
        self._actions.uncloak_ball(BALL_ID_1)
        self._ticker.tick()
        self.assertTrue(ball.isCloaked)

    def test_uncloak_ball_uncloaks_ball(self):
        self._set_up_single_ball()
        ball = add_ball_to_park(self._park, ball_id=BALL_ID_1)
        self._actions.cloak_ball(BALL_ID_1, destiny.DSTNORMALCLOAK, 1000.0)
        self._ticker.tick()
        self._actions.uncloak_ball(BALL_ID_1)
        self._ticker.tick()
        self.assertFalse(ball.isCloaked)

    def test_uncloak_does_not_error_when_ball_missing(self):
        self._actions.uncloak_ball(BALL_ID_1)
        self._ticker.tick()

    def test_uncloak_ball_emits_signal(self):
        self._set_up_single_ball()
        callback = mock.Mock()
        self._ticker.on_ball_uncloaking.connect(callback)
        add_ball_to_park(self._park, ball_id=BALL_ID_1)
        self._actions.cloak_ball(BALL_ID_1, destiny.DSTNORMALCLOAK, 1000.0)
        self._ticker.tick()
        self._actions.uncloak_ball(BALL_ID_1)
        self._ticker.tick()
        callback.assert_called_once_with(BALL_ID_1)

    def _perform_tick(self, skip_evolve=False):
        """
        :param skip_evolve: If we want to compare ball state of the sent out balls
        with the state in our ballpark, then we want to skip
        the evolve step.
        :type skip_evolve: bool
        """
        self._ticker.tick()
        if not skip_evolve:
            self._park.Evolve()
        self._update_batcher.send_batch()

    def test_uncloak_event_sent_to_uncloaking_ball(self):
        self._set_up_single_ball()
        add_ball_to_park(self._park, ball_id=BALL_ID_1)
        self._actions.cloak_ball(BALL_ID_1, destiny.DSTNORMALCLOAK, 1000.0)
        self._perform_tick()
        self._actions.uncloak_ball(BALL_ID_1)
        self._network.clear()
        self._perform_tick()
        self.assertUpdateSinglecast(CLIENT_1, ('UncloakBall', (BALL_ID_1,)))

    def _countMatchingAddBallsToParkActions(self, actions, ball_id):
        count = 0
        for timestamp, action in actions:
            action_name, action_parameters = action
            if action_name == "AddBallsToPark":
                ball_data = action_parameters[0]
                park = self.read_balls_from_stream_into_park(ball_data)
                self.assertIn(ball_id, park.balls, "Ball %s not in AddBallsToPark update stream" % ball_id)
                loaded_ball = park.balls[ball_id]
                self.assertBallsEqual(self._park.balls[ball_id], loaded_ball, ignore_bubble_id=True)
                count += 1
        return count

    def assertAddBallsToParkSinglecastExistsForBall(self, ball_id, client_id):
        found = 0
        for entry in self._network.singlecasts:
            entry_client_id, method_name, actions, wait_for_bubble, casts_in_batch = entry
            if method_name != "DoDestinyUpdate":
                continue
            if entry_client_id != client_id:
                continue

            found += self._countMatchingAddBallsToParkActions(actions, ball_id)
        self.assertEqual(1, found)

    def assertAddBallsToParkNarrowcastExistsForBall(self, ball_id, client_ids):
        found = 0
        for entry in self._network.narrowcasts:
            entry_client_id, method_name, actions, wait_for_bubble, casts_in_batch = entry
            if method_name != "DoDestinyUpdate":
                continue
            if set(entry_client_id) != set(client_ids):
                continue
            found += self._countMatchingAddBallsToParkActions(actions, ball_id)
        self.assertEqual(1, found)

    def test_add_ball_event_sent_to_balls_in_same_bubble_as_uncloaking_ball(self):
        self._set_up_two_balls_in_same_bubble()
        self._actions.cloak_ball(BALL_ID_1, destiny.DSTNORMALCLOAK, 1000.0)
        self._perform_tick()
        self._actions.uncloak_ball(BALL_ID_1)
        self._network.clear()
        self._perform_tick(skip_evolve=True)
        self.assertAddBallsToParkSinglecastExistsForBall(BALL_ID_1, CLIENT_2)

    def test_set_ball_position_emits_signal(self):
        self._set_up_single_ball()
        callback = mock.Mock()
        self._ticker.on_set_ball_position.connect(callback)
        is_local_ball = False
        self._actions.set_ball_position(
            BALL_ID_1,
            1234.5, 678.9, 987654321.0,
            is_local_ball=is_local_ball
        )
        self._perform_tick()
        callback.assert_called_once_with(BALL_ID_1, is_local_ball)

    def test_set_ball_position_sets_ball_position(self):
        ball = self._set_up_single_ball()
        x, y, z = 1234.5, 678.9, 987654321.0
        self._actions.set_ball_position(
            BALL_ID_1,
            1234.5, 678.9, 987654321.0,
            is_local_ball=False
        )
        self._perform_tick()
        self.assertEqual(
            (x, y, z),
            (ball.x, ball.y, ball.z)
        )

    def test_set_ball_position_event_sent_to_bubble(self):
        self._set_up_two_balls_in_same_bubble()
        self._perform_tick()
        x, y, z = 1.5, 2.9, 3.0
        self._actions.set_ball_position(
            BALL_ID_1,
            1.5, 2.9, 3.0,
            is_local_ball=False
        )
        self._perform_tick()
        self.assertUpdateNarrowcast(
            [CLIENT_1, CLIENT_2],
            ('SetBallPosition', (BALL_ID_1, x, y, z))
        )

    def test_set_ball_position_event_not_sent_to_bubble_for_local_balls(self):
        self._set_up_two_balls_in_same_bubble()
        self._perform_tick()
        self._actions.set_ball_position(
            BALL_ID_1,
            1.5, 2.9, 3.0,
            is_local_ball=True
        )
        self._perform_tick()
        self.assertEqual([], self._network.narrowcasts)

    def test_add_balls_to_park(self):
        self._set_up_two_balls_in_same_bubble()
        self._actions.set_ball_position(BALL_ID_2, 1000000000.0, 0.0, 0.0)
        self._perform_tick()
        self._network.clear()
        self._actions.add_ball_to_client_parks(BALL_ID_2)
        self._perform_tick(skip_evolve=True)
        self.assertAddBallsToParkNarrowcastExistsForBall(BALL_ID_2, [CLIENT_1])

    def test_ball_not_global_sends_remove_ball_update_to_other_bubbles(self):
        self._set_up_three_balls_in_same_bubble()
        self._actions.set_ball_position(BALL_ID_2, 1000000000.0, 0.0, 0.0)
        self._actions.set_ball_position(BALL_ID_3, 1000000000.0, 0.0, 1000.0)
        self._actions.make_ball_global(BALL_ID_2)
        self._perform_tick()
        self._network.clear()
        self._actions.make_ball_local(BALL_ID_2)
        self._perform_tick()
        # Ball 3 should not be included in the narrowcast as it is in the same bubble.
        self.assertUpdateNarrowcast([CLIENT_1], ('RemoveBall', (BALL_ID_2,)))

    def test_handle_remove_global_ball_sends_remove_ball_event_to_each_bubble_containing_interactives(self):
        self._set_up_three_balls_in_same_bubble()
        self._actions.set_ball_position(BALL_ID_2, 1000000000.0, 0.0, 0.0)
        self._actions.set_ball_position(BALL_ID_3, 1000000000.0, 0.0, 1000.0)
        self._actions.make_ball_global(BALL_ID_2)
        self._perform_tick()
        self._network.clear()
        self._actions.remove_ball(BALL_ID_2)
        self._perform_tick()
        self.assertUpdateNarrowcast([CLIENT_1], ('RemoveBall', (BALL_ID_2,)))
        self.assertUpdateNarrowcast([CLIENT_3], ('RemoveBall', (BALL_ID_2,)))

    def test_cloaked_ball_updates_only_get_sent_to_cloaked_balls_client(self):
        self._set_up_two_balls_in_same_bubble()
        uncloak_range_meters = 1000.0
        self._actions.cloak_ball(BALL_ID_1, destiny.DSTNORMALCLOAK, uncloak_range_meters)
        self._perform_tick()
        self._network.clear()
        x, y, z = 1.5, 2.9, 3.0
        self._actions.go_to_point(
            BALL_ID_1,
            x, y, z
        )
        self._perform_tick()
        self.assertEqual([], self._network.narrowcasts)
        self.assertUpdateSinglecast(CLIENT_1, ("GotoPoint", (BALL_ID_1, x, y, z)))

    def test_no_updates_are_sent_out_for_bubbles_not_containing_interactive_balls(self):
        self._set_up_two_balls_in_same_bubble()
        self._actions.set_ball_interactive(BALL_ID_1, False)
        self._actions.set_ball_interactive(BALL_ID_2, False)
        self._perform_tick()
        self._network.clear()
        self._actions.go_to_point(BALL_ID_1, 1000.0, 2000.0, 3000.0)
        self._actions.set_speed_fraction(BALL_ID_2, 0.5)
        self._perform_tick()
        self.assertEqual([], self._network.narrowcasts)
        self.assertEqual([], self._network.singlecasts)
