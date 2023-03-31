import mock
import unittest

import blue
import destiny
from destiny.net.client import Ticker
from destiny.test.net.client.helpers import add_ball_to_park, TestClientTicker, TestErrorHandler

BALL_ID_1 = 1
BALL_ID_2 = 2
BALL_ID_3 = 3

TICKS_ELAPSED = 10


class TestTicker(unittest.TestCase):
    def setUp(self):
        self._error_handler = TestErrorHandler()
        self._client_ticker = TestClientTicker()
        self._ticker = Ticker(self._error_handler, self._client_ticker)
        self._ballpark = destiny.Ballpark()
        self._ticker.set_ballpark(self._ballpark)
        self._do_setstate()
        self._tick(TICKS_ELAPSED)

    def tearDown(self):
        del self._ballpark
        del self._ticker
        del self._client_ticker
        del self._error_handler

    def _get_park_with_state(self):
        bp = destiny.Ballpark()
        ball_1 = add_ball_to_park(bp, ball_id=1, x=0, y=0, z=0)
        ball_2 = add_ball_to_park(bp, ball_id=2, x=1000, y=0, z=0)
        ball_3 = add_ball_to_park(bp, ball_id=3, x=-1000, y=0, z=0)
        bp.Orbit(ball_2.id, ball_1.id)
        bp.GotoPoint(ball_3.id, 0, 0, 0)
        bp.GotoDirection(ball_1.id, 0.0, 1.0, 0.0)
        for _ in xrange(TICKS_ELAPSED):
            bp.Evolve()
        return bp

    def _do_setstate(self, current_time=0):
        park_with_state = self._get_park_with_state()
        s = blue.MemStream()
        park_with_state.WriteFullStateToStream(s)
        self._ticker.update([[current_time, ("SetState", (s.Read(), BALL_ID_1))]], False)

    def test_setstate(self):
        self.assertEqual(self._ballpark.currentTime, TICKS_ELAPSED + 1)
        ball_two = self._ballpark.balls[BALL_ID_2]
        self.assertEqual(destiny.DSTBALL_ORBIT, ball_two.mode)

    def test_stopball(self):
        self._ticker.update([[TICKS_ELAPSED + 1, ("Stop", (BALL_ID_2,))]], False)
        self._ticker.do_pre_tick()
        self._ticker.do_post_tick(TICKS_ELAPSED)
        ball_two = self._ballpark.balls[BALL_ID_2]
        self.assertEqual(destiny.DSTBALL_STOP, ball_two.mode)

    def test_future_state_gets_applied_at_correct_time(self):
        self._ticker.update([[TICKS_ELAPSED + 2, ("Stop", (BALL_ID_2,))]], False)
        ball_two = self._ballpark.balls[BALL_ID_2]
        self._tick(TICKS_ELAPSED + 1)
        self.assertEqual(destiny.DSTBALL_ORBIT, ball_two.mode)
        self._tick(TICKS_ELAPSED + 2)
        self.assertEqual(destiny.DSTBALL_STOP, ball_two.mode)

    def test_late_arrival(self):
        ball_two = self._ballpark.balls[BALL_ID_2]
        self._tick(TICKS_ELAPSED + 1)
        self._tick(TICKS_ELAPSED + 2)
        self.assertEqual(destiny.DSTBALL_ORBIT, ball_two.mode)
        self._ticker.update([[TICKS_ELAPSED + 2, ("Stop", (BALL_ID_2,))]], False)
        self._tick(TICKS_ELAPSED + 3)
        self.assertEqual(destiny.DSTBALL_STOP, ball_two.mode)

    def _tick(self, time_stamp):
        self._ticker.do_pre_tick()
        self._ballpark._parent_Evolve()
        self._ticker.do_post_tick(time_stamp)

    def test_synchronize_to_simulation_time_can_not_rewind_before_first_tick(self):
        success = self._ticker.synchronize_to_simulation_time(TICKS_ELAPSED - 1)
        self.assertFalse(success)

    def test_synchronize_to_simulation_time_can_rewind_to_last_tick(self):
        self._tick(TICKS_ELAPSED + 1)
        self._ballpark.RemoveBall(BALL_ID_1)
        self.assertNotIn(BALL_ID_1, self._ballpark.balls)
        self.assertEqual(TICKS_ELAPSED + 2, self._ballpark.currentTime)
        success = self._ticker.synchronize_to_simulation_time(TICKS_ELAPSED + 1)
        self.assertTrue(success)
        self.assertEqual(TICKS_ELAPSED + 1, self._ballpark.currentTime)
        self.assertIn(BALL_ID_1, self._ballpark.balls)

    def test_synchronize_to_simulation_time_can_rewind_two_ticks(self):
        self._tick(TICKS_ELAPSED + 1)
        self._ballpark.RemoveBall(BALL_ID_1)
        self.assertNotIn(BALL_ID_1, self._ballpark.balls)
        self._tick(TICKS_ELAPSED + 2)
        self.assertEqual(TICKS_ELAPSED + 3, self._ballpark.currentTime)
        success = self._ticker.synchronize_to_simulation_time(TICKS_ELAPSED + 1)
        self.assertTrue(success)
        self.assertEqual(TICKS_ELAPSED + 1, self._ballpark.currentTime)

    def test_no_desync_reported_for_successfuly_handled_update(self):
        self._ticker.update([[TICKS_ELAPSED + 1, ("Stop", (BALL_ID_2,))]], False)
        self._ticker.do_pre_tick()
        self.assertFalse(self._error_handler.recoverable_desync_occurred)
        self.assertFalse(self._error_handler.fatal_desync_occurred)

    def test_fatal_desync_reported_when_update_has_multiple_timestamps(self):
        self._ticker.update([[TICKS_ELAPSED + 1, ("Stop", (BALL_ID_2,))], [TICKS_ELAPSED + 2, ("Stop", (BALL_ID_2,))]], False)
        self.assertTrue(self._error_handler.fatal_desync_occurred)

    def test_recoverable_desync_reported_when_fail_to_sync_to_timestamp(self):
        self._ticker.update([[TICKS_ELAPSED + 1, ("Stop", (BALL_ID_2,))]], False)
        self._ticker.synchronize_to_simulation_time = mock.Mock(return_value=False)
        self._ticker.do_pre_tick()
        self.assertTrue(self._error_handler.recoverable_desync_occurred)
