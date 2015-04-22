import destiny
from destiny.test import helpers
import blue

class TestWriteBallsToStream(helpers.BallparkTestCase):
    def test_write_no_ball(self):
        stream = blue.MemStream()
        self.park.WriteBallsToStream([], stream)
        self.assertEqual(stream.Read(), "\x01\x00\x00\x00\x00")

    def test_write_ball_then_read_ball(self):
        stream = blue.MemStream()
        saved_ball, = self.add_balls(1)
        self.park.WriteBallsToStream([saved_ball.id], stream)
        self.park.ClearAll()
        self.park.ReadFullStateFromStream(stream)
        self.assertIn(saved_ball.id, self.park.balls)
        loaded_ball = self.park.balls[saved_ball.id]
        self.assertBallEqual(saved_ball, loaded_ball)


class TestWriteFullStateToStream(helpers.BallparkTestCase):
    def test_write_no_ball(self):
        stream = blue.MemStream()
        self.park.WriteFullStateToStream(stream)
        self.assertEqual(stream.Read(), "\x00\x00\x00\x00\x00")

    def test_write_ball_then_read_ball(self):
        stream = blue.MemStream()
        saved_ball, = self.add_balls(1)
        self.park.WriteFullStateToStream(stream)
        self.park.ClearAll()
        self.park.ReadFullStateFromStream(stream)
        self.assertIn(saved_ball.id, self.park.balls)
        loaded_ball = self.park.balls[saved_ball.id]
        self.assertBallEqual(saved_ball, loaded_ball)
