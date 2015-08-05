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

    def test_write_read_miniball(self):
        stream = blue.MemStream()
        saved_ball, = self.add_balls(1)
        saved_ball.AddMiniBall(1.0, 2.0, 3.0, 4.0)
        self.park.WriteBallsToStream([saved_ball.id], stream)
        self.park.ClearAll()
        self.park.ReadFullStateFromStream(stream)
        self.assertIn(saved_ball.id, self.park.balls)
        loaded_ball = self.park.balls[saved_ball.id]
        self.assertMiniBallEqual(saved_ball.miniBalls[0], loaded_ball.miniBalls[0])

    def test_write_read_minicapsule(self):
        stream = blue.MemStream()
        saved_ball, = self.add_balls(1)
        saved_ball.AddMiniCapsule(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0)
        self.park.WriteBallsToStream([saved_ball.id], stream)
        self.park.ClearAll()
        self.park.ReadFullStateFromStream(stream)
        self.assertIn(saved_ball.id, self.park.balls)
        loaded_ball = self.park.balls[saved_ball.id]
        self.assertMiniCapsuleEqual(saved_ball.miniCapsules[0], loaded_ball.miniCapsules[0])

    def test_write_read_minibox(self):
        stream = blue.MemStream()
        saved_ball, = self.add_balls(1)
        saved_ball.AddMiniBox(1.0, 2.0, 3.0,
                              1.0, 0.0, 0.0,
                              0.0, 1.0, 0.0,
                              0.0, 0.0, 1.0)
        self.park.WriteBallsToStream([saved_ball.id], stream)
        self.park.ClearAll()
        self.park.ReadFullStateFromStream(stream)
        self.assertIn(saved_ball.id, self.park.balls)
        loaded_ball = self.park.balls[saved_ball.id]
        self.assertMiniBoxEqual(saved_ball.miniBoxes[0], loaded_ball.miniBoxes[0])


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
