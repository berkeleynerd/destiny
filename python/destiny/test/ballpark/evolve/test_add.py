# Copyright © 2000 CCP ehf.

import destiny
import unittest

from destiny.test.net.client.helpers import add_ball_to_park


class EventHandlingBall(destiny.Ball):
    def __init__(self, *args):
        super(EventHandlingBall, self).__init__(*args)
        self.warp_done = False
        self.ball_added_during_evolve = None

    def DoModeChange(self, oldMode, newMode):
        # When warp finishes DoModeChange gets sent out from within Evolve()
        if oldMode == destiny.DSTBALL_WARP and newMode == destiny.DSTBALL_STOP:
            self.warp_done = True
            self.ball_added_during_evolve = add_ball_to_park(self.ballpark)


class TestAdd(unittest.TestCase):
    def test_balls_can_not_be_added_during_evolve(self):
        park = destiny.Ballpark()
        park.isMaster = True
        b = EventHandlingBall(add_ball_to_park(park))
        x, y, z = 100000.0, 200000.0, 300000.0
        park.WarpTo(b.id, x, y, z)
        while not b.warp_done:
            park.Evolve()
        self.assertEqual(None, b.ball_added_during_evolve)
