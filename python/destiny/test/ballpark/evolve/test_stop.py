from destiny.test import helpers

class TestStop(helpers.BallparkTestCase):
    def test_stopped_ball_is_stopped(self):
        stopped_ball = helpers.create_space_ball(self.park)

        for i in xrange(10):
            self.park.Evolve()
            self.assertEqual(stopped_ball.x, 0.0)
            self.assertEqual(stopped_ball.y, 0.0)
            self.assertEqual(stopped_ball.z, 0.0)

    def test_stop_ball_in_goto_mode(self):
        stopped_ball = helpers.create_space_ball(self.park)
        stopped_velocity_list = []

        stopped_ball.vx = 10.0
        stopped_ball.vy = 20.0
        stopped_ball.vz = 30.0

        self.park.Stop(stopped_ball.id)

        for i in xrange(10):
            self.park.Evolve()
            stopped_velocity_list.append((stopped_ball.vx, stopped_ball.vy, stopped_ball.vz))

        expected_location_list = [
            (9.180806045144438, 15.959158171933325, 27.542418135433316),
            (8.428719963856068, 12.73473647783931, 25.286159891568197),
            (7.738244319699939, 10.161783686386327, 23.21473295909981),
            (7.104332022910581, 8.108675658000553, 21.312996068731735),
            (6.522349438265067, 6.470381869546817, 19.567048314795194),
            (5.9880425151368355, 5.163092384445365, 17.9641275454105),
            (5.497505692155016, 4.119930400983397, 16.49251707646504),
            (5.047153349175273, 3.2875310463325356, 15.141460047525813),
            (4.633693597887935, 2.6233113981781195, 13.901080793663798),
            (4.254104219483663, 2.0932920768880083, 12.762312658450982)
        ]

        self.assertListOfPointsAlmostEqual(stopped_velocity_list, expected_location_list)
