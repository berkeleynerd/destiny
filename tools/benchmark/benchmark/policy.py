# Copyright © 2023 CCP ehf.

import abc
from time import time
from benchmark.output import print_ball_state
from benchmark.util import get_random_direction, get_random_vector

import destiny


AU = .1495978707e12  # 1 Astronomical Unit in meters
MAU = .1495978707e9  # 1/1000th of an AU in meters.


class Policy(object):
    __metaclass__ = abc.ABCMeta

    def __init__(self, server, tick_interval=1):
        self._server = server
        self._tick_interval = tick_interval
        self._last_apply = None

    def apply(self):
        apply_start = time()
        if self._last_apply is None or self._last_apply + self._tick_interval >= self._server.park.currentTime:
            self._apply()
            self._last_apply = self._server.park.currentTime
        apply_end = time()
        return apply_end - apply_start

    @abc.abstractmethod
    def _apply(self):
        pass


class AddInteractive(Policy):
    def _apply(self):
        self._server.add_interactive()


class ChangeDirectionAtRandom(Policy):
    def _apply(self):
        for ball_id in self._server.park.balls:
            x, y, z = get_random_direction()
            self._server.destiny_actions.go_to_direction(ball_id, x, y, z)


class GoToDirection(Policy):
    def __init__(self, server, tick_interval=1, x=0.0, y=0.0, z=0.0):
        super(GoToDirection, self).__init__(server, tick_interval=tick_interval)
        self.x = x
        self.y = y
        self.z = z

    def _apply(self):
        for ball_id in self._server.park.balls:
            self._server.destiny_actions.go_to_direction(ball_id, self.x, self.y, self.z)


class OutputBallState(Policy):
    def __init__(self, server, tick_interval=1, filter_ids=None):
        super(OutputBallState, self).__init__(server, tick_interval=tick_interval)
        self._filter_ids = filter_ids

    def _apply(self):
        for ball_id in self._server.park.balls:
            if self._filter_ids and ball_id not in self._filter_ids:
                continue
            ball = self._server.park.balls[ball_id]
            print_ball_state(ball)


class EntityWarpSymmetrical(Policy):
    def __init__(self, server, tick_interval=1, x=0.0, y=0.0, z=0.0, warp_speed_mau=2, warp_distance_km=500):
        super(EntityWarpSymmetrical, self).__init__(server, tick_interval=tick_interval)
        self._x = x
        self._y = y
        self._z = z
        self._warp_speed_mau = warp_speed_mau
        self._warp_distance_km = warp_distance_km

    def _apply(self):
        warp_distance_m = self._warp_distance_km * 1000.0
        dx, dy, dz = get_random_vector(warp_distance_m)
        for ball_id in self._server.park.balls:
            ball = self._server.park.balls[ball_id]
            if ball.mode == destiny.DSTBALL_WARP:
                continue  # Let's finish our current warp first, shall we?
            self._server.destiny_actions.entity_warp_in(
                ball_id,
                self._x + dx, self._y + dy, self._z + dz,
                self._warp_speed_mau
            )


class EntityWarpAsymmetrical(Policy):
    def __init__(self, server, tick_interval=1, x=0.0, y=0.0, z=0.0, warp_speed_mau=2, warp_distance_km=500):
        super(EntityWarpAsymmetrical, self).__init__(server, tick_interval=tick_interval)
        self._x = x
        self._y = y
        self._z = z
        self._warp_speed_mau = warp_speed_mau
        self._warp_distance_km = warp_distance_km

    def _apply(self):
        warp_distance_m = self._warp_distance_km * 1000.0
        for ball_id in self._server.park.balls:
            dx, dy, dz = get_random_vector(warp_distance_m)
            ball = self._server.park.balls[ball_id]
            if ball.mode == destiny.DSTBALL_WARP:
                continue  # Let's finish our current warp first, shall we?
            self._server.destiny_actions.entity_warp_in(
                ball_id,
                self._x + dx, self._y + dy, self._z + dz,
                self._warp_speed_mau
            )
