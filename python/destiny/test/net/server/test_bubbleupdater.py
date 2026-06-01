# Copyright © 2023 CCP ehf.

import unittest

import destiny
from destiny.test.net.server.helpers import add_ball_to_park
from destiny.net.server import BubbleUpdater

BALL_ID_1 = 1001
BALL_ID_2 = 1002


class TestBubbleUpdater(unittest.TestCase):
    def setUp(self):
        self._park = destiny.Ballpark()
        self._park.isMaster = True
        self._bubble_updater = BubbleUpdater(self._park)

    def tearDown(self):
        del self._park
        del self._bubble_updater

    def test_additions_and_deletions(self):
        add_ball_to_park(self._park, ball_id=BALL_ID_1)
        add_ball_to_park(self._park, ball_id=BALL_ID_2, x=500000000.0)
        self._bubble_updater.additions_and_deletions()
        self.assertEqual({BALL_ID_1: [BALL_ID_1], BALL_ID_2: [BALL_ID_2]}, self._bubble_updater.additions_per_player)
        self.assertEqual({BALL_ID_1: [], BALL_ID_2: []}, self._bubble_updater.deletions_per_player)
        self.assertEqual({0: [BALL_ID_1], 1: [BALL_ID_2]}, self._bubble_updater.additions_per_bubble)
        self.assertEqual({0: [], 1: []}, self._bubble_updater.deletions_per_bubble)

    def test_additions_and_deletions_updates_bubbles_dict(self):
        add_ball_to_park(self._park, ball_id=BALL_ID_1)
        add_ball_to_park(self._park, ball_id=BALL_ID_2, x=500000000.0)
        self._bubble_updater.additions_and_deletions()
        self.assertEqual({0: {BALL_ID_1: 0}, 1: {BALL_ID_2: 0}}, self._park.bubbles)

    def test_additions_and_deletions_handles_removal(self):
        add_ball_to_park(self._park, ball_id=BALL_ID_1)
        add_ball_to_park(self._park, ball_id=BALL_ID_2)
        self._bubble_updater.additions_and_deletions()
        self._park.Evolve()
        self._park.RemoveBall(BALL_ID_1)
        self._bubble_updater.additions_and_deletions()
        self.assertEqual(
            {BALL_ID_2: 0},
            self._bubble_updater.additions_per_player
        )
        self.assertEqual(
            {BALL_ID_2: 0},
            self._bubble_updater.deletions_per_player
        )
        self.assertEqual(
            {0: []},
            self._bubble_updater.additions_per_bubble
        )
        self.assertEqual(
            {0: [BALL_ID_1]},
            self._bubble_updater.deletions_per_bubble
        )

    def test_additions_and_deletions_multiple_calls_same_tick(self):
        add_ball_to_park(self._park, ball_id=BALL_ID_1)
        add_ball_to_park(self._park, ball_id=BALL_ID_2, x=500000000.0)
        for _ in range(3):
            self._bubble_updater.additions_and_deletions()
        self.assertEqual(
            {BALL_ID_1: [BALL_ID_1], BALL_ID_2: [BALL_ID_2]},
            self._bubble_updater.additions_per_player
        )
        self.assertEqual(
            {BALL_ID_1: [], BALL_ID_2: []},
            self._bubble_updater.deletions_per_player
        )
        self.assertEqual(
            {0: [BALL_ID_1], 1: [BALL_ID_2]},
            self._bubble_updater.additions_per_bubble
        )
        self.assertEqual(
            {0: [], 1: []},
            self._bubble_updater.deletions_per_bubble
        )
