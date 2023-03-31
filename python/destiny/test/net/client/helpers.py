from destiny.net.client import ClientTickerInterface, TickErrorHandlerInterface


def add_ball_to_park(
        park,
        ball_id=-1,
        mass=1,
        radius=1.0,
        max_velocity=1.0,
        is_free=True,
        is_global=False,
        is_massive=True,
        is_interactive=True,
        is_space_junk=False,
        x=0,
        y=0,
        z=0,
        vx=0,
        vy=0,
        vz=0,
        agility=1.0,
        speed_fraction=1.0
):
    return park.AddBall(
        ball_id,
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
        speed_fraction,
    )


class TestErrorHandler(TickErrorHandlerInterface):
    def __init__(self):
        self._fatal_desync_occured = False
        self._recoverable_desync_occurred = False

    def on_fatal_desync(self):
        self._fatal_desync_occured = True

    def on_recoverable_desync(self):
        self._recoverable_desync_occurred = True

    @property
    def fatal_desync_occurred(self):
        return self._fatal_desync_occured

    @property
    def recoverable_desync_occurred(self):
        return self._recoverable_desync_occurred


class TestClientTicker(ClientTickerInterface):
    def on_set_state(self):
        pass

    def on_ballpark_local_action(self, func_name, args):
        pass

    def should_log_actions(self):
        return True

    def get_ball_destruction_delay(self, ball, is_terminal=False):
        return 0

    def get_ball_destruction_delays(self, ball_ids, is_release=False):
        return {
            ball_id: 0
            for ball_id in ball_ids
        }

    def clean_up_after_ball_removal(self, ball_id, ball, is_terminal=False):
        pass

    def clean_up_after_multiple_ball_removal(self, ball_ids, is_release=False):
        pass
