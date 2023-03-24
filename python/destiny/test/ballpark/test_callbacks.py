import destiny
from destiny.test import helpers


class TestProximitySensors(helpers.BallparkTestCase):
    def test_callback_gets_called_when_period_has_passed(self):
        proximity_ball = helpers.add_ball_event_spy_to_park(self.park, objectID=3)
        range = 10.0
        period = 2.0
        shuffle = False
        onlyInteractives = False
        proximity_ball.AddProximitySensor(range, period, shuffle, onlyInteractives)
        regular_ball = helpers.add_ball_to_park(self.park, objectID=4)
        regular_ball.isFree = True
        self.park.Evolve() # Each tick takes one time unit
        self.assertEqual(proximity_ball.proximity_callback_args, [])
        self.park.Evolve()
        self.assertEqual(proximity_ball.proximity_callback_args, [(regular_ball.id, 1)])

    def test_callback_gets_called(self):
        proximity_ball = helpers.add_ball_event_spy_to_park(self.park, objectID=3)
        range = 10.0
        period = 0.0
        shuffle = False
        onlyInteractives = False
        proximity_ball.AddProximitySensor(range, period, shuffle, onlyInteractives)
        regular_ball = helpers.add_ball_to_park(self.park, objectID=4)
        regular_ball.isFree = True
        self.park.Evolve()
        self.assertEqual(proximity_ball.proximity_callback_args, [(regular_ball.id, 1)])

    def test_ignores_balls_that_are_not_free(self):
        proximity_ball = helpers.add_ball_event_spy_to_park(self.park, objectID=3)
        range = 10.0
        period = 0.0
        shuffle = False
        onlyInteractives = False
        proximity_ball.AddProximitySensor(range, period, shuffle, onlyInteractives)
        regular_ball = helpers.add_ball_to_park(self.park, objectID=4)
        regular_ball.isFree = False
        self.park.Evolve()
        self.assertEqual(proximity_ball.proximity_callback_args, [])

    def test_only_interactives_ignores_non_interactive_balls(self):
        proximity_ball = helpers.add_ball_event_spy_to_park(self.park, objectID=3)
        range = 10.0
        period = 0.0
        shuffle = False
        onlyInteractives = True
        proximity_ball.AddProximitySensor(range, period, shuffle, onlyInteractives)
        regular_ball = helpers.add_ball_to_park(self.park, objectID=4)
        regular_ball.isFree = True
        self.park.Evolve()
        self.assertEqual(proximity_ball.proximity_callback_args, [])

    def test_remove_proximity_sensor(self):
        proximity_ball = helpers.add_ball_event_spy_to_park(self.park, objectID=3)
        range = 10.0
        period = 0.0
        shuffle = False
        onlyInteractives = False
        proximity_ball.AddProximitySensor(range, period, shuffle, onlyInteractives)
        self.park.RemoveProximitySensor(proximity_ball.id)
        regular_ball = helpers.add_ball_to_park(self.park, objectID=4)
        regular_ball.isFree = True
        self.park.Evolve()
        self.assertEqual(proximity_ball.proximity_callback_args, [])

    def test_ball_out_of_range(self):
        proximity_ball = helpers.add_ball_event_spy_to_park(self.park, objectID=3)
        range = 10.0
        period = 0.0
        shuffle = False
        onlyInteractives = False
        proximity_ball.AddProximitySensor(range, period, shuffle, onlyInteractives)
        regular_ball = helpers.add_ball_to_park(self.park, objectID=4, x=(proximity_ball.x + 11.0))
        regular_ball.isFree = True
        self.park.Evolve()
        self.assertEqual(proximity_ball.proximity_callback_args, [])

    def test_ball_on_edge_of_range(self):
        proximity_ball = helpers.add_ball_event_spy_to_park(self.park, objectID=3)
        range = 10.0
        period = 0.0
        shuffle = False
        onlyInteractives = False
        proximity_ball.AddProximitySensor(range, period, shuffle, onlyInteractives)
        regular_ball = helpers.add_ball_to_park(self.park, objectID=4, x=(proximity_ball.x + 10.0))
        regular_ball.isFree = True
        self.park.Evolve()
        self.assertEqual(proximity_ball.proximity_callback_args, [(regular_ball.id, 1)])

    def test_leave_proximity(self):
        proximity_ball = helpers.add_ball_event_spy_to_park(self.park, objectID=3)
        range = 10.0
        period = 0.0
        shuffle = False
        onlyInteractives = False
        proximity_ball.AddProximitySensor(range, period, shuffle, onlyInteractives)
        regular_ball = helpers.add_ball_to_park(self.park, objectID=4)
        regular_ball.isFree = True
        self.park.Evolve()
        regular_ball.x += 11.0
        self.park.Evolve()
        self.assertEqual(proximity_ball.proximity_callback_args, [(regular_ball.id, 1), (regular_ball.id, 0)])


class TestNotificationRange(helpers.BallparkTestCase):
    def test_detects_ball_in_range_when_in_goto_mode(self):
        ball = helpers.add_ball_event_spy_to_park(self.park, objectID=3)
        other = helpers.add_ball_to_park(self.park, objectID=4, x=(ball.x+1.0))
        self.park.GotoPoint(ball.id, other.x, other.y, other.z)
        self.park.SetNotificationRange(ball.id, 10.0)
        self.park.Evolve()
        self.assertEqual(ball.range_callback_args, [1])

    def test_ignores_ball_on_edge_of_range(self):
        ball = helpers.add_ball_event_spy_to_park(self.park, objectID=3, radius=1.0)
        other = helpers.add_ball_to_park(self.park, objectID=4, x=ball.x+11.0)
        self.park.GotoPoint(ball.id, other.x, other.y, other.z)
        self.park.SetNotificationRange(ball.id, 10.0)
        self.park.Evolve()
        self.assertEqual(ball.range_callback_args, [])

    def test_ignores_ball_that_should_be_out_of_range(self):
        ball = helpers.add_ball_event_spy_to_park(self.park, objectID=3)
        other = helpers.add_ball_to_park(self.park, objectID=4, y=ball.y+100.0)
        self.park.GotoPoint(ball.id, other.x, other.y, other.z)
        self.park.SetNotificationRange(ball.id, 10.0)
        self.park.Evolve()
        self.assertEqual(ball.range_callback_args, [])

    def test_ignores_ball_in_range_if_stopped(self):
        ball = helpers.add_ball_event_spy_to_park(self.park, objectID=3)
        helpers.add_ball_to_park(self.park, objectID=4)
        self.park.SetNotificationRange(ball.id, 10.0)
        self.park.Evolve()
        self.assertEqual(ball.range_callback_args, [])

    def test_detects_ball_leaving_range(self):
        ball = helpers.add_ball_event_spy_to_park(self.park, objectID=3)
        self.park.SetNotificationRange(ball.id, 10.0)
        helpers.add_ball_to_park(self.park, objectID=4)
        self.park.GotoPoint(ball.id, ball.x + 1.0, ball.y, ball.z)
        self.park.Evolve()
        ball.x += 100.0
        self.park.Evolve()
        self.assertEqual(ball.range_callback_args, [1, 0])


class TestSetTargetTracking(helpers.BallparkTestCase):
    def test_foo(self):
        ball = helpers.add_ball_event_spy_to_park(self.park, objectID=3)
        other = helpers.add_ball_to_park(self.park, objectID=4)
        self.park.SetTargetTracking(ball.id, other.id, 10.0)
        self.park.Evolve()
        self.assertEqual(ball.target_tracking_callback_args, [1])

    def test_detects_target_on_edge_of_range(self):
        ball = helpers.add_ball_event_spy_to_park(self.park, objectID=3)
        other = helpers.add_ball_to_park(self.park, objectID=4, x=ball.x+10.0)
        self.park.SetTargetTracking(ball.id, other.id, 10.0)
        self.park.Evolve()
        self.assertEqual(ball.target_tracking_callback_args, [])

    def test_ignores_target_that_should_be_out_of_range(self):
        ball = helpers.add_ball_event_spy_to_park(self.park, objectID=3)
        other = helpers.add_ball_to_park(self.park, objectID=4, x=ball.x+11.0)
        self.park.SetTargetTracking(ball.id, other.id, 10.0)
        self.park.Evolve()
        self.assertEqual(ball.target_tracking_callback_args, [])

    def test_detects_target_leaving_range(self):
        ball = helpers.add_ball_event_spy_to_park(self.park, objectID=3)
        other = helpers.add_ball_to_park(self.park, objectID=4)
        self.park.SetTargetTracking(ball.id, other.id, 10.0)
        self.park.Evolve()
        other.x += 100.0
        self.park.Evolve()
        self.assertEqual(ball.target_tracking_callback_args, [1, 0])


class TestSetBallNotInParkCallback(helpers.BallparkTestCase):
    def setUp(self):
        self.called = False
        super(TestSetBallNotInParkCallback, self).setUp()

    def handle_ball_not_in_park_error(self, ball):
        self.called = True

    def test_gets_called_when_addminiball_is_called_on_ball_that_is_not_in_park(self):
        self.park.SetBallNotInParkCallback(self.handle_ball_not_in_park_error)
        b = destiny.Ball()
        b.AddMiniBall(1.0, 1.0, 1.0, 1.0)
        self.assertTrue(self.called)

    def test_does_not_get_called_when_addminiball_is_called_on_ball_that_is_in_park(self):
        self.park.SetBallNotInParkCallback(self.handle_ball_not_in_park_error)
        b, = self.add_balls(1)
        b.AddMiniBall(1.0, 1.0, 1.0, 1.0)
        self.assertFalse(self.called)

    def test_remove_callback(self):
        self.park.SetBallNotInParkCallback(self.handle_ball_not_in_park_error)
        self.park.SetBallNotInParkCallback(None)
        b = destiny.Ball()
        b.AddMiniBall(1.0, 1.0, 1.0, 1.0)
        self.assertFalse(self.called)
