# Copyright © 2023 CCP ehf.

import os
import sys

import unittest

import blue
import destiny


CID_BALL = "_destiny.Ball"
CID_PARK = "_destiny.Ballpark"
CID_CLIENTBALL = "_destiny.ClientBall"
CID_MINIBALL = '_destiny.MiniBall'
CID_MINIBOX = '_destiny.MiniBox'
CID_MINICAPSULE = '_destiny.MiniCapsule'

PYTHON_MODULE_PATH = os.path.abspath(os.path.dirname(os.path.dirname(os.path.dirname(__file__))))


def get_live_count(cid):
    live_count = blue.classes.LiveCount()
    return live_count.get(cid, -1)


class TestLiveCount(unittest.TestCase):
    def test_initial_count_is_zero(self):
        self.assertLiveCount(0, CID_BALL)
        self.assertLiveCount(0, CID_PARK)
        self.assertLiveCount(0, CID_CLIENTBALL)
        self.assertLiveCount(0, CID_MINIBALL)
        self.assertLiveCount(0, CID_MINIBOX)
        self.assertLiveCount(0, CID_MINICAPSULE)

    def test_parks_get_counted(self):
        # noinspection PyUnusedLocal
        park = self._create_server_park()
        self.assertLiveCount(1, CID_PARK)
        del park
        self.assertLiveCount(0, CID_PARK)

    def assertLiveCount(self, expected, cid):
        live_count_dict = blue.classes.LiveCount()
        live_count = live_count_dict.get(cid, None)
        self.assertEqual(expected, live_count)

    def _create_server_park(self):
        park = destiny.Ballpark()
        park.isMaster = True
        return park

    def _add_ball(self, park):
        return park.AddBall(
            -1,  # id
            0.0,  # mass
            500.0,  # radius
            0,  # maxVel
            0,  # isFree
            0,  # isGlobal
            0,  # isMassive
            0,  # isInteractive
            0,  # isSpaceJunk
            0, 0, 0,  # x, y, z
            0, 0, 0,  # vx, vy, vz
            0,  # agility
            0  # speedFraction
        )

    def addMiniCapsule(self, ball):
        ball.AddMiniCapsule(0.0, 0.0, 0.0, 10.0, 0.0, 0.0, 1.0)

    def addMiniBall(self, ball):
        ball.AddMiniBall(0.0, 0.0, 0.0, 10.0)

    def addMiniBox(self, ball):
        ball.AddMiniBox(
            -5.0, 0.0, 0.0,
            10.0, 0.0, 0.0,
            0.0, 10.0, 0.0,
            0.0, 0.0, 10.0
        )

    def test_balls_get_counted(self):
        park = self._create_server_park()
        # noinspection PyUnusedLocal
        ball = self._add_ball(park)
        self.assertLiveCount(1, CID_BALL)
        park.RemoveBall(ball.id, 0)
        park.Evolve()
        del ball
        self.assertLiveCount(0, CID_BALL)

    def test_miniballs_get_counted(self):
        park = self._create_server_park()
        ball = self._add_ball(park)
        self.addMiniBall(ball)
        self.assertLiveCount(1, CID_MINIBALL)
        ball_id = ball.id
        park.RemoveBall(ball_id, 0)
        park.Evolve()
        del ball
        self.assertLiveCount(0, CID_MINIBALL)

    def test_miniboxes_get_counted(self):
        park = self._create_server_park()
        ball = self._add_ball(park)
        self.addMiniBox(ball)
        self.assertLiveCount(1, CID_MINIBOX)
        ball_id = ball.id
        park.RemoveBall(ball_id, 0)
        park.Evolve()
        del ball
        self.assertLiveCount(0, CID_MINIBOX)

    def test_minicapsules_get_counted(self):
        park = self._create_server_park()
        ball = self._add_ball(park)
        self.addMiniCapsule(ball)
        self.assertLiveCount(1, CID_MINICAPSULE)
        ball_id = ball.id
        park.RemoveBall(ball_id, 1)
        del ball
        self.assertLiveCount(0, CID_MINICAPSULE)

    def test_clear_all_resets_counts(self):
        park = self._create_server_park()
        ball = self._add_ball(park)
        self.addMiniBall(ball)
        self.addMiniBox(ball)
        self.addMiniCapsule(ball)
        park.ClearAll()
        del ball
        self.assertLiveCount(0, CID_BALL)
        self.assertLiveCount(0, CID_MINIBALL)
        self.assertLiveCount(0, CID_MINIBOX)
        self.assertLiveCount(0, CID_MINICAPSULE)

    def test_clientballs_get_counted(self):
        park = destiny.Ballpark()
        ball = self._add_ball(park)
        self.assertLiveCount(1, CID_CLIENTBALL)
        ball_id = ball.id
        park.RemoveBall(ball_id, 0)
        park.Evolve()
        del ball
        self.assertLiveCount(0, CID_CLIENTBALL)

    def test_clientballs_get_cleared(self):
        park = destiny.Ballpark()
        # noinspection PyUnusedLocal
        ball = self._add_ball(park)
        self.assertLiveCount(1, CID_CLIENTBALL)
        park.ClearAll()
        del ball
        self.assertLiveCount(0, CID_CLIENTBALL)
