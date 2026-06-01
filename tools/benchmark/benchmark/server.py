# Copyright © 2023 CCP ehf.

from time import time
from benchmark.destinyinterface import BallInfo, Network, ClientInterests, CharacterInterests
from benchmark.util import get_id_for_interactive_ball
from destiny.net.server import (
    Ticker,
    ParkUpdateBatcher,
    Actions,
    BubbleUpdater,
)

import destiny


class Server(object):
    def __init__(self):
        self._park = destiny.Ballpark()
        self._park.isMaster = True
        self._ball_info = BallInfo()
        self._actions = Actions(self._park)
        self._network = Network()
        self._character_interests = CharacterInterests()
        self._client_interests = ClientInterests()
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
        self._update_batcher.update_bubbles()

    def tick(self):
        tick_start = time()
        self._ticker.tick()
        tick_end = time()
        self._update_batcher.send_batch()
        send_batch_end = time()
        self._park.Evolve()
        evolve_end = time()
        tick_duration = tick_end - tick_start
        send_batch_duration = send_batch_end - tick_end
        evolve_duration = evolve_end - send_batch_end
        return tick_duration, send_batch_duration, evolve_duration

    @property
    def destiny_actions(self):
        return self._actions

    @property
    def park(self):
        return self._park

    def add_ball(
            self,
            src_id=-1,
            mass=1000000.0,
            radius=30.0,
            max_velocity=500.0,
            is_free=True,
            is_global=False,
            is_massive=True,
            is_interactive=False,
            is_space_junk=False,
            x=0.0,
            y=0.0,
            z=0.0,
            vx=0.0,
            vy=0.0,
            vz=0.0,
            agility=2.0,
            speed_fraction=1.0):
        """
        This should no longer be necessary once we
        properly get keyword arguments in for the AddBall method
        with autocomplete figured out.
        https://www.jetbrains.com/help/pycharm/stubs.html
        """
        return self._park.AddBall(
            src_id,
            mass,
            radius,
            max_velocity,
            is_free,
            is_global,
            is_massive,
            is_interactive,
            is_space_junk,
            x,
            y,
            z,
            vx,
            vy,
            vz,
            agility,
            speed_fraction
        )

    def add_interactive(self, **kwargs):
        ball_id = get_id_for_interactive_ball()
        character_id = ball_id
        client_id = ball_id
        self.add_ball(ball_id, is_interactive=True, **kwargs)
        self._client_interests.add_client_interest(ball_id, client_id)
        self._character_interests.add_interest(ball_id, character_id)
        self._character_interests.map_to_client(character_id, client_id)
        self._ball_info.set_characters_for_ball(ball_id, [character_id])

    @property
    def narrowcast_bytes(self):
        return self._network._narrowcasts_bytes

    @property
    def singlecast_bytes(self):
        return self._network.singlecast_bytes
