import destiny
from destiny.test import helpers

class TestBallpark(helpers.BallparkTestCase):
    def test_can_create_ballpark(self):
        self.assertIsNotNone(self.park)

    def test_add_ball_creates_a_client_ball(self):
        ball = helpers.add_ball_to_park(self.park)
        self.assertEqual(type(ball), type(destiny.ClientBall()))

    def test_add_ball_adds_ball_to_park(self):
        _, = self.add_balls(1)
        self.assertEqual(len(self.park.balls), 1)

    def test_remove_ball_from_park(self):
        ball, = self.add_balls(1)
        self.park.RemoveBall(ball.id)
        self.assertEqual(len(self.park.balls), 0)


class TestRemoveBall(helpers.BallparkTestCase):
    def test_remove_ball_no_delay(self):
        ball, = self.add_balls(1)
        self.park.RemoveBall(ball.id)
        self.assertTrue(ball.isMoribund)

    def test_remove_ball_with_delay(self):
        ball, = self.add_balls(1)
        self.park.RemoveBall(ball.id, 100)
        self.assertTrue(ball.isMoribund)


class TestClearAll(helpers.BallparkTestCase):
    def test_clear_all_removes_ball_from_park(self):
        helpers.add_ball_to_park(self.park, x=1.0, y=2.0, z=3.0)
        self.park.ClearAll()
        self.assertIsNone(self.park.GetBubbleAtCoordinates(1.0, 2.0, 3.0))


class TestAdditionsAndDeletions(helpers.BallparkTestCase):
    def test_addition(self):
        ball, = self.add_balls(1)
        ball.isInteractive = True
        additions_per_player = {}
        deletions_per_player = {}
        additions_per_bubble = {}
        deletions_per_bubble = {}
        user_ships = [ball.id]
        self.park.InitializeBubbles()
        self.park.AdditionsAndDeletions(additions_per_player, deletions_per_player, additions_per_bubble, deletions_per_bubble, user_ships)
        self.assertEqual(additions_per_player, { ball.id: [ball.newBubbleId, ball.id] })
        self.assertEqual(deletions_per_player, { ball.id: [] })
        self.assertEqual(additions_per_bubble, { ball.newBubbleId: [ball.id] })
        self.assertEqual(deletions_per_bubble, { ball.newBubbleId: [] })

    def test_no_change(self):
        ball, = self.add_balls(1)
        ball.isInteractive = True
        additions_per_player = {}
        deletions_per_player = {}
        additions_per_bubble = {}
        deletions_per_bubble = {}
        user_ships = [ball.id]
        self.park.InitializeBubbles()
        self.park.AdditionsAndDeletions(additions_per_player, deletions_per_player, additions_per_bubble, deletions_per_bubble, user_ships)
        self.park.CopyBubbles()
        self.park.AdditionsAndDeletions(additions_per_player, deletions_per_player, additions_per_bubble, deletions_per_bubble, user_ships)
        self.assertEqual(additions_per_player, { ball.id: ball.newBubbleId })
        self.assertEqual(deletions_per_player, { ball.id: ball.newBubbleId })
        self.assertEqual(additions_per_bubble, { ball.newBubbleId: [] })
        self.assertEqual(deletions_per_bubble, { ball.newBubbleId: [] })

    def test_removal(self):
        ball, = self.add_balls(1)
        ball.isInteractive = True
        additions_per_player = {}
        deletions_per_player = {}
        additions_per_bubble = {}
        deletions_per_bubble = {}
        user_ships = [ball.id]

        self.park.InitializeBubbles()
        self.park.AdditionsAndDeletions(additions_per_player, deletions_per_player, additions_per_bubble, deletions_per_bubble, user_ships)
        self.park.RemoveBall(ball.id)
        self.park.CopyBubbles()
        self.park.AdditionsAndDeletions(additions_per_player, deletions_per_player, additions_per_bubble, deletions_per_bubble, user_ships)

        self.assertEqual(additions_per_player, {})
        self.assertEqual(deletions_per_player, {})
        self.assertEqual(additions_per_bubble, {})
        self.assertEqual(deletions_per_bubble, {})


class TestAddMushroom(helpers.BallparkTestCase):
    """
    Mushrooms are balls that expand over time, originating from and not colliding with their owners.
    """
    def test_mushroom_gets_created_correctly(self):
        owner, = self.add_balls(1)
        _range = 10.0
        time = 1.0
        self.park.AddMushroom(owner.id, _range, time)
        mushroom = self.park.balls[0] # Mushrooms should get added with an id of 0
        self.assertEqual(mushroom.mode, destiny.DSTBALL_MUSHROOM)
        self.assertEqual(mushroom.ownerId, owner.id)
        self.assertEqual(mushroom.followRange, _range)
        self.assertEqual(mushroom.gotoX, time)
        self.assertFalse(mushroom.isMoribund)

    def test_mushroom_can_not_be_created_with_invalid_owner(self):
        _range = 10.0
        time = 1.0
        self.park.AddMushroom(1, _range, time)
        self.assertNotIn(0, self.park.balls)

    def test_mushroom_that_is_not_set_free_does_not_expand(self):
        owner, = self.add_balls(1)
        _range = 10.0
        time = 5.0
        self.park.AddMushroom(owner.id, _range, time)
        mushroom = self.park.balls[0]
        self.park.Evolve()
        self.park.Evolve()
        self.assertAlmostEqual(mushroom.radius, 0.0)

    def test_mushroom_expands_correctly(self):
        owner, = self.add_balls(1)
        _range = 10.0
        time = 5.0
        self.park.AddMushroom(owner.id, _range, time)
        mushroom = self.park.balls[0]
        self.park.SetBallFree(mushroom.id, True)
        self.assertEqual(mushroom.radius, 0.0)
        self.park.Evolve()
        self.assertEqual(mushroom.radius, 0.0)
        self.park.Evolve()
        self.assertAlmostEqual(mushroom.radius, 6.687403202056885)
        self.park.Evolve()
        self.assertAlmostEqual(mushroom.radius, 7.952707290649414)
        self.park.Evolve()
        self.assertAlmostEqual(mushroom.radius, 8.801117897033691)
        self.park.Evolve()
        self.assertAlmostEqual(mushroom.radius, 9.457415580749512)
        self.park.Evolve()
        self.assertAlmostEqual(mushroom.radius, 9.457415580749512)

    def test_mushrooms_die_when_fully_expanded(self):
        owner, = self.add_balls(1)
        _range = 10.0
        time = 1.0
        self.park.AddMushroom(owner.id, _range, time)
        mushroom = self.park.balls[0]
        self.park.SetBallFree(mushroom.id, True)
        self.park.Evolve()
        self.park.Evolve()
        self.assertTrue(mushroom.isMoribund)