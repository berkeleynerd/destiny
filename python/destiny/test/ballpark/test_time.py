from destiny.test import helpers


class TestTime(helpers.BallparkTestCase):
    def test_ballpark_time_initialises_to_zero(self):
        old_time = self.park.time
        self.assertEqual(old_time, 0)

    def test_adjust_times_adds_to_ballpark_time(self):
        self.park.AdjustTimes(2)
        self.assertEqual(self.park.time, 2)
        self.park.AdjustTimes(3)
        self.assertEqual(self.park.time, 5)


class TestPauseAndStart(helpers.BallparkTestCase):
    def test_initial_state_is_paused(self):
        self.assertFalse(self.park.isRunning)

    def test_can_start(self):
        self.park.Start()
        self.assertTrue(self.park.isRunning)

    def test_can_pause(self):
        self.park.Start()
        self.park.Pause()
        self.assertFalse(self.park.isRunning)
