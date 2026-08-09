# Copyright © 2015 CCP ehf.

import destiny
from destiny.test import helpers
import blue


class TestWriteBallsToStream(helpers.BallparkTestCase):
    def test_write_no_ball(self):
        stream = blue.MemStream()
        self.park.WriteBallsToStream([], stream)
        self.assertEqual(stream.Read(), b"\x01\x00\x00\x00\x00")

    def test_write_ball_then_read_ball(self):
        stream = blue.MemStream()
        saved_ball, = self.add_balls(1)
        self.park.WriteBallsToStream([saved_ball.id], stream)
        reader_park = destiny.Ballpark()
        reader_park.ReadFullStateFromStream(stream)
        self.assertIn(saved_ball.id, reader_park.balls)
        loaded_ball = reader_park.balls[saved_ball.id]
        self.assertBallEqual(saved_ball, loaded_ball)

    def test_read_replaces_existing_same_id_ball_state(self):
        stream = blue.MemStream()
        saved_ball = helpers.add_ball_to_park(
            self.park,
            objectID=42,
            x=100.0,
            y=200.0,
            z=300.0,
            vx=4.0,
            vy=5.0,
            vz=6.0,
            isFree=True,
        )
        self.park.WriteBallsToStream([saved_ball.id], stream)

        reader_park = destiny.Ballpark()
        existing_ball = helpers.add_ball_to_park(
            reader_park,
            objectID=saved_ball.id,
            x=-1.0,
            y=-2.0,
            z=-3.0,
        )
        reader_park.ReadFullStateFromStream(stream)

        self.assertEqual(1, len(reader_park.balls))
        self.assertEqual(existing_ball.id, saved_ball.id)
        self.assertBallEqual(saved_ball, reader_park.balls[saved_ball.id])

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
        self.assertEqual(stream.Read(), b"\x00\x00\x00\x00\x00")

    def test_write_ball_then_read_ball(self):
        stream = blue.MemStream()
        saved_ball, = self.add_balls(1)
        self.park.WriteFullStateToStream(stream)
        reader_park = destiny.Ballpark()
        reader_park.ReadFullStateFromStream(stream)
        self.assertIn(saved_ball.id, reader_park.balls)
        loaded_ball = reader_park.balls[saved_ball.id]
        self.assertBallEqual(saved_ball, loaded_ball)
