from destiny.test import helpers
from destiny.test.helpers import MIN_BOX_SIZE
import math


class TestInitialize_bubbles(helpers.BallparkTestCase):
    def test_can_initialize_bubbles(self):
        ball, = self.add_balls(1)
        self.assertEqual(ball.newBubbleId, -1)
        self.park.InitializeBubbles()
        self.assertNotEquals(ball.newBubbleId, -1)
        self.assertEquals(ball.oldBubbleId, -1)

    def test_can_initialize_bubble_for_specific_ball(self):
        ball, other_ball = self.add_balls(2)
        self.assertEqual(ball.newBubbleId, -1)
        self.park.InitializeBubbles(ball.id)
        self.assertNotEquals(ball.newBubbleId, -1)
        self.assertEquals(ball.oldBubbleId, -1)
        self.assertEqual(other_ball.newBubbleId, -1)


class TestGetBallIdsInRange(helpers.BallparkTestCase):
    def test_get_ball_ids_in_range_for_ball_id(self):
        ball, other = self.add_balls(2)
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsInRange(ball.id, 100)
        self.assertListEqual(result, [other.id])

    def test_ball_on_edge_of_range_gets_included(self):
        ball, other = self.add_balls(2)
        other.x += 100.0
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsInRange(ball.id, 100.0)
        self.assertListEqual(result, [other.id])

    def test_ignores_out_of_range_balls(self):
        ball, other = self.add_balls(2)
        other.x += 100.1
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsInRange(ball.id, 100)
        self.assertListEqual(result, [])

    def test_get_ball_ids_in_range_for_location(self):
        ball, other = self.add_balls(2)
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsInRange(ball.x, ball.y, ball.z, 100)
        self.assertListEqual(result, [ball.id, other.id])


class TestGetBallIdsAndDistInRange(helpers.BallparkTestCase):
    def test_get_ball_ids_in_range_for_ball_id(self):
        ball, other = self.add_balls(2)
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsAndDistInRange(ball.id, 100)
        self.assertListEqual(result, [(0.0, 2L)])

    def test_returns_distance_squared(self):
        ball, other = self.add_balls(2)
        other.x += 2.0
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsAndDistInRange(ball.id, 100.0)
        self.assertListEqual(result, [(4.0, other.id)])

    def test_ball_on_edge_of_range(self):
        ball, other = self.add_balls(2)
        other.x += 100.0
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsAndDistInRange(ball.id, 100.0)
        self.assertListEqual(result, [(10000.0, other.id)])

    def test_ignores_out_of_range_balls(self):
        ball, other = self.add_balls(2)
        other.x += 100.1
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsAndDistInRange(ball.id, 100)
        self.assertListEqual(result, [])

    def test_get_ball_ids_in_range_for_location(self):
        ball, other = self.add_balls(2)
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsAndDistInRange(ball.x, ball.y, ball.z, 100)
        self.assertListEqual(result, [(0.0, ball.id), (0.0, other.id)])


class TestGetBallIdInRangeOfTriangle(helpers.BallparkTestCase):
    def test_source_does_not_exist_raises_runtime_error(self):
        self.park.InitializeBubbles()
        ballNotInPark = 12345
        self.assertRaises(RuntimeError, self.park.GetBallIdsInRangeOfTriangle,
            ballNotInPark,
            10.0, 0.0, 0.0,
            0.0, 10.0, 0.0,
            5.0)

    def test_no_other_ball(self):
        ball, = self.add_balls(1)
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsInRangeOfTriangle(
            ball.id, 
            10.0, 0.0, 0.0,
            0.0, 10.0, 0.0,
            5.0)
        self.assertListEqual(result, [])

    def test_other_ball_in_triangle(self):
        ball, other = self.add_balls(2)
        other.radius = 1.0
        other.x += 5.0
        other.y += 5.0
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsInRangeOfTriangle(
            ball.id, 
            10.0, 0.0, 0.0,
            0.0, 10.0, 0.0,
            5.0)
        self.assertListEqual(result, [other.id])

    def test_other_ball_out_of_range(self):
        ball, other = self.add_balls(2)
        other.radius = 1.0
        other.x += 10.0
        other.y += 10.0
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsInRangeOfTriangle(
            ball.id, 
            10.0, 0.0, 0.0,
            0.0, 10.0, 0.0,
            5.0)
        self.assertListEqual(result, [])

    def test_other_ball_on_edge_of_range(self):
        ball, other = self.add_balls(2)
        other.radius = 1.0
        other.x += 16.0
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsInRangeOfTriangle(
            ball.id, 
            10.0, 0.0, 0.0,
            0.0, 10.0, 0.0,
            5.0)
        self.assertListEqual(result, [other.id])

    def test_multiple_balls_in_triangle(self):
        ball, a, b = self.add_balls(3)
        a.radius = 1.0
        b.radius = 1.0
        a.x += 5.0
        b.y += 5.0
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsInRangeOfTriangle(
            ball.id, 
            10.0, 0.0, 0.0,
            0.0, 10.0, 0.0,
            5.0)
        self.assertEqual(set(result), set([a.id, b.id]))


class TestGetBallIdsInCapsule(helpers.BallparkTestCase):
    def test_source_does_not_exist_raises_runtime_error(self):
        self.park.InitializeBubbles()
        ballNotInPark = 12345
        self.assertRaises(RuntimeError, self.park.GetBallIdsInCapsule,
            ballNotInPark,
            0.0, 0.0, 10.0,
            5.0)

    def test_no_other_ball(self):
        ball, = self.add_balls(1)
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsInCapsule(
            ball.id,
            0.0, 0.0, 10.0,
            5.0)
        self.assertListEqual(result, [])

    def test_other_ball_in_capsule(self):
        ball, other = self.add_balls(2)
        other.radius = 1.0
        other.z += 5.0
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsInCapsule(
            ball.id,
            0.0, 0.0, 10.0,
            5.0)
        self.assertListEqual(result, [other.id])

    def test_other_ball_in_capsule_and_on_the_edge_of_one_side(self):
        ball, other = self.add_balls(2)
        other.radius = 1.0
        other.z += 5.0
        other.x += 6.0
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsInCapsule(
            ball.id,
            0.0, 0.0, 10.0,
            5.0)
        self.assertListEqual(result, [other.id])

    def test_other_ball_not_in_capsule_too_far_from_the_edge_of_one_side(self):
        ball, other = self.add_balls(2)
        other.radius = 1.0
        other.z += 5.0
        other.x += 7.0
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsInCapsule(
            ball.id,
            0.0, 0.0, 10.0,
            5.0)
        self.assertListEqual(result, [])

    def test_other_ball_not_in_capsule(self):
        ball, other = self.add_balls(2)
        other.radius = 1.0
        other.x += 10.0
        other.y += 10.0
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsInCapsule(
            ball.id,
            0.0, 0.0, 10.0,
            5.0)
        self.assertListEqual(result, [])

    def test_other_ball_on_edge_of_capsule(self):
        ball, other = self.add_balls(2)
        other.radius = 1.0
        other.z += 16.0
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsInCapsule(
            ball.id,
            0.0, 0.0, 10.0,
            5.0)
        self.assertListEqual(result, [other.id])

    def test_multiple_balls_in_capsule(self):
        ball, a, b = self.add_balls(3)
        a.radius = 1.0
        b.radius = 1.0
        a.z += 0.0
        b.z += 10.0
        self.park.InitializeBubbles()
        result = self.park.GetBallIdsInCapsule(
            ball.id,
            0.0, 0.0, 10.0,
            5.0)
        self.assertEqual(set(result), set([a.id, b.id]))


class TestGetBallIdsInCone(helpers.BallparkTestCase):
    def _get_colliding_balls(self, ball):
        return self.park.GetBallIdsInCone(
            ball.id,
            0.0, 0.0, 10.0,
            math.radians(45.0))

    def test_no_other_ball(self):
        ball, = self.add_balls(1)
        result = self._get_colliding_balls(ball)
        self.assertListEqual(result, [])

    def test_source_does_not_exist_raises_runtime_error(self):
        self.park.InitializeBubbles()
        ballNotInPark = 12345
        self.assertRaises(RuntimeError, self.park.GetBallIdsInCone,
            ballNotInPark,
            0.0, 0.0, 10.0,
            math.radians(45.0)
        )

    def test_ball_in_cone(self):
        ball, other = self.add_balls(2)
        self.park.InitializeBubbles()
        other.radius = 1.0
        other.z += 5.0
        result = self._get_colliding_balls(ball)
        self.assertListEqual(result, [other.id])

    def test_ball_too_far_away(self):
        ball, other = self.add_balls(2)
        other.radius = 1.0
        other.z += 12.0
        result = self._get_colliding_balls(ball)
        self.assertListEqual(result, [])

    def test_ball_intersects_front_of_cone(self):
        ball, other = self.add_balls(2)
        self.park.InitializeBubbles()
        other.radius = 1.0
        other.z += 11.0
        result = self._get_colliding_balls(ball)
        self.assertListEqual(result, [other.id])

    def test_ball_intersects_cone(self):
        ball, other = self.add_balls(2)
        self.park.InitializeBubbles()
        other.radius = 1.0
        other.z += 5.0
        other.x += 6.0
        result = self._get_colliding_balls(ball)
        self.assertListEqual(result, [other.id])

    def test_ball_does_not_intersect_cone(self):
        ball, other = self.add_balls(2)
        other.radius = 1.0
        other.z += 5.0
        other.x += 7.0
        result = self._get_colliding_balls(ball)
        self.assertListEqual(result, [])


class TestBallBox(helpers.BallparkTestCase):
    def test_box_zero_zero_zero(self):
        ball, = self.add_balls(1)
        ball.x = 1.0
        ball.y = 1.0
        ball.z = 1.0
        box_width, (x,y,z) = self.park.GetBallBox(ball.id)
        self.assertEqual(box_width, MIN_BOX_SIZE)
        self.assertAlmostEqual(x, MIN_BOX_SIZE / 2.0)
        self.assertAlmostEqual(y, MIN_BOX_SIZE / 2.0)
        self.assertAlmostEqual(z, MIN_BOX_SIZE / 2.0)

    def test_box_zero_zero_one(self):
        ball, = self.add_balls(1)
        ball.x = 1.0
        ball.y = 1.0
        ball.z = MIN_BOX_SIZE + 1.0
        box_width, (x,y,z) = self.park.GetBallBox(ball.id)
        self.assertEqual(box_width, MIN_BOX_SIZE)
        self.assertAlmostEqual(x, MIN_BOX_SIZE / 2.0)
        self.assertAlmostEqual(y, MIN_BOX_SIZE / 2.0)
        self.assertAlmostEqual(z, MIN_BOX_SIZE * 1.5)


class TestGetStaticCollidableBox(helpers.BallparkTestCase):
    def test_capsule_in_box_zero_zero_zero(self):
        parentBall = helpers.add_ball_to_park(self.park, x=0, y=0, z=0)
        parentBall.AddMiniCapsule(
            1.0, 1.0, 1.0,
            2.0, 2.0, 2.0,
            1.0
        )
        capsule = parentBall.miniCapsules[-1]
        box_width, (x,y,z) = self.park.GetStaticCollidableBox(capsule.id)
        self.assertEqual(box_width, MIN_BOX_SIZE)
        self.assertAlmostEqual(x, MIN_BOX_SIZE / 2.0)
        self.assertAlmostEqual(y, MIN_BOX_SIZE / 2.0)
        self.assertAlmostEqual(z, MIN_BOX_SIZE / 2.0)

    def test_capsule_in_box_zero_zero_one(self):
        parentBall = helpers.add_ball_to_park(self.park, x=0, y=0, z=0)
        parentBall.AddMiniCapsule(
            1.0, 1.0, MIN_BOX_SIZE + 1.0,
            1.0, 1.0, MIN_BOX_SIZE + 3.0,
            1.0
        )
        capsule = parentBall.miniCapsules[-1]
        box_width, (x,y,z) = self.park.GetStaticCollidableBox(capsule.id)
        self.assertEqual(box_width, MIN_BOX_SIZE)
        self.assertAlmostEqual(x, MIN_BOX_SIZE / 2.0)
        self.assertAlmostEqual(y, MIN_BOX_SIZE / 2.0)
        self.assertAlmostEqual(z, MIN_BOX_SIZE * 1.5)

    def test_oriented_box_in_box_zero_zero_zero(self):
        parentBall = helpers.add_ball_to_park(self.park, x=0, y=0, z=0)
        parentBall.AddMiniBox(
           MIN_BOX_SIZE/4.0, MIN_BOX_SIZE/4.0, MIN_BOX_SIZE/4.0,
           1.0, 0.0, 0.0,
           0.0, 1.0, 0.0,
           0.0, 0.0, 1.0
        )
        obb = parentBall.miniBoxes[-1]
        box_width, (x,y,z) = self.park.GetStaticCollidableBox(obb.id)
        self.assertEqual(box_width, MIN_BOX_SIZE)
        self.assertAlmostEqual(x, MIN_BOX_SIZE / 2.0)
        self.assertAlmostEqual(y, MIN_BOX_SIZE / 2.0)
        self.assertAlmostEqual(z, MIN_BOX_SIZE / 2.0)

    def test_oriented_box_in_box_zero_zero_one(self):
        parentBall = helpers.add_ball_to_park(self.park, x=0, y=0, z=0)
        parentBall.AddMiniBox(
           MIN_BOX_SIZE/4.0, MIN_BOX_SIZE/4.0, MIN_BOX_SIZE*1.5,
           1.0, 0.0, 0.0,
           0.0, 1.0, 0.0,
           0.0, 0.0, 1.0
        )
        obb = parentBall.miniBoxes[-1]
        box_width, (x, y, z) = self.park.GetStaticCollidableBox(obb.id)
        self.assertEqual(box_width, MIN_BOX_SIZE)
        self.assertAlmostEqual(x, MIN_BOX_SIZE / 2.0)
        self.assertAlmostEqual(y, MIN_BOX_SIZE / 2.0)
        self.assertAlmostEqual(z, MIN_BOX_SIZE * 1.5)

    def test_adding_a_capsule_makes_a_box_active(self):
        parentBall = helpers.add_ball_to_park(self.park, x=0, y=0, z=0)
        parentBall.AddMiniCapsule(
            2.0, 2.0, 2.0,
            3.0, 3.0, 3.0,
            1.0
        )
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes = [(0.0, 0.0, 0.0)]
        self.assertEqual(active_boxes, expected_active_boxes)

    def test_adding_a_large_capsule_makes_adjecent_boxes_active(self):
        parentBall = helpers.add_ball_to_park(self.park, x=0, y=0, z=0)
        parentBall.AddMiniCapsule(
            1.0, 1.0, 1.0,
            2.0, 2.0, 2.0,
            2.0
        )
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes = [
            (-MIN_BOX_SIZE, -MIN_BOX_SIZE, -MIN_BOX_SIZE),
            (-MIN_BOX_SIZE, -MIN_BOX_SIZE, 0.0),
            (-MIN_BOX_SIZE, 0.0, -MIN_BOX_SIZE),
            (-MIN_BOX_SIZE, 0.0, 0.0),
            (0.0, -MIN_BOX_SIZE, -MIN_BOX_SIZE),
            (0.0, -MIN_BOX_SIZE, 0.0),
            (0.0, 0.0, -MIN_BOX_SIZE),
            (0.0, 0.0, 0.0)
        ]
        self.assertListEqual(sorted(active_boxes), expected_active_boxes)

    def test_adding_a_long_capsule_makes_adjecent_boxes_active(self):
        parentBall = helpers.add_ball_to_park(self.park, x=0, y=0, z=0)
        parentBall.AddMiniCapsule(
            MIN_BOX_SIZE / 2.0, MIN_BOX_SIZE / 2.0, MIN_BOX_SIZE / 2.0,
            MIN_BOX_SIZE * 6.0, MIN_BOX_SIZE / 2.0, MIN_BOX_SIZE / 2.0,
            MIN_BOX_SIZE / 3.0)

        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes = [
            (0.0, 0.0, 0.0),
            (MIN_BOX_SIZE, 0.0, 0.0),
            (MIN_BOX_SIZE * 2, 0.0, 0.0),
            (MIN_BOX_SIZE * 3, 0.0, 0.0),
            (MIN_BOX_SIZE * 4, 0.0, 0.0),
            (MIN_BOX_SIZE * 5, 0.0, 0.0),
            (MIN_BOX_SIZE * 6, 0.0, 0.0)
        ]
        self.assertListEqual(sorted(active_boxes), expected_active_boxes)


class TestGetActiveBoxes(helpers.BallparkTestCase):
    def test_returns_none_when_there_are_no_active_boxes(self):
        level = 0
        active_boxes = self.park.GetActiveBoxes(level)
        self.assertIsNone(active_boxes)

    def test_returns_list_of_boxes_when_there_are_active_boxes(self):
        ball, = self.add_balls(1)
        ball.x = 1
        ball.y = 1
        ball.z = 1
        level = 7
        level_width, box_low_coord_list = self.park.GetActiveBoxes(level)
        self.assertEqual(level_width, MIN_BOX_SIZE)
        self.assertEqual(box_low_coord_list, [(0.0, 0.0, 0.0)])


class TestActiveBoxesForOrientedBox(helpers.BallparkTestCase):
    def test_adding_a_box_makes_a_box_active(self):
        parentBall = helpers.add_ball_to_park(self.park, x=0.0, y=0.0, z=0.0, radius=500.0)
        parentBall.AddMiniBox(
            2.0, 2.0, 2.0,
            1.0, 0.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0
        )
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes = [(0.0, 0.0, 0.0)]
        self.assertEqual(expected_active_boxes, active_boxes)

    def test_tight_fit(self):
        parentBall = helpers.add_ball_to_park(self.park, x=0, y=0, z=0)
        parentBall.AddMiniBox(
            0.0, 0.0, 0.0,
            10.0, 0.0, 0.0,
            0.0, 10.0, 0.0,
            0.0, 0.0, 10.0
        )
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0)]
        self.assertEqual(expected_active_boxes, active_boxes)

    def test_level_six(self):
        parentBall = helpers.add_ball_to_park(self.park, x=0, y=0, z=0)
        parentBall.AddMiniBox(
             0.0, 0.0, 0.0,
             10.1, 0.0, 0.0,
             0.0, 10.1, 0.0,
             0.0, 0.0, 10.1
        )
        level = 6
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0)]
        self.assertEqual(expected_active_boxes, active_boxes)

    def test_intersect_left(self):
        parentBall = helpers.add_ball_to_park(self.park, x=0, y=0, z=0)
        parentBall.AddMiniBox(
             -1.0, 0.0, 0.0,
             5.0, 0.0, 0.0,
             0.0, 5.0, 0.0,
             0.0, 0.0, 5.0
        )
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0), (-MIN_BOX_SIZE, 0.0, 0.0)]
        self.assertSetEqual(set(expected_active_boxes), set(active_boxes))

    def test_intersect_right(self):
        parentBall = helpers.add_ball_to_park(self.park, x=0, y=0, z=0)
        parentBall.AddMiniBox(
             MIN_BOX_SIZE - 4.0, 0.0, 0.0,
             5.0, 0.0, 0.0,
             0.0, 5.0, 0.0,
             0.0, 0.0, 5.0
        )
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0), (MIN_BOX_SIZE, 0.0, 0.0)]
        self.assertSetEqual(set(expected_active_boxes), set(active_boxes))

    def test_intersect_bottom(self):
        parentBall = helpers.add_ball_to_park(self.park, x=0, y=0, z=0)
        parentBall.AddMiniBox(
             0.0, -1.0, 0.0,
             5.0, 0.0, 0.0,
             0.0, 5.0, 0.0,
             0.0, 0.0, 5.0
        )
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0), (0.0, -MIN_BOX_SIZE, 0.0)]
        self.assertSetEqual(set(expected_active_boxes), set(active_boxes))

    def test_intersect_top(self):
        parentBall = helpers.add_ball_to_park(self.park, x=0, y=0, z=0)
        parentBall.AddMiniBox(
             0.0, MIN_BOX_SIZE - 4.0, 0.0,
             5.0, 0.0, 0.0,
             0.0, 5.0, 0.0,
             0.0, 0.0, 5.0
        )
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0), (0.0, MIN_BOX_SIZE, 0.0)]
        self.assertSetEqual(set(expected_active_boxes), set(active_boxes))

    def test_intersect_back(self):
        parentBall = helpers.add_ball_to_park(self.park, x=0, y=0, z=0)
        parentBall.AddMiniBox(
             0.0, 0.0, -1.0,
             5.0, 0.0, 0.0,
             0.0, 5.0, 0.0,
             0.0, 0.0, 5.0
        )
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0), (0.0, 0.0, -MIN_BOX_SIZE)]
        self.assertEqual(set(expected_active_boxes), set(active_boxes))

    def test_intersect_front(self):
        parentBall = helpers.add_ball_to_park(self.park, x=0, y=0, z=0)
        parentBall.AddMiniBox(
             0.0, 0.0, MIN_BOX_SIZE -  4.0,
             5.0, 0.0, 0.0,
             0.0, 5.0, 0.0,
             0.0, 0.0, 5.0
        )
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0), (0.0, 0.0, MIN_BOX_SIZE)]
        self.assertSetEqual(set(expected_active_boxes), set(active_boxes))

    def test_intersect_top_front_right(self):
        parentBall = helpers.add_ball_to_park(self.park, x=0, y=0, z=0)
        parentBall.AddMiniBox(
             MIN_BOX_SIZE - 4.0, MIN_BOX_SIZE - 4.0, MIN_BOX_SIZE - 4.0,
             5.0, 0.0, 0.0,
             0.0, 5.0, 0.0,
             0.0, 0.0, 5.0
        )
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0),
                                  (MIN_BOX_SIZE, 0.0, 0.0),
                                  (0.0, MIN_BOX_SIZE, 0.0),
                                  (0.0, 0.0, MIN_BOX_SIZE),
                                  (0.0, MIN_BOX_SIZE, MIN_BOX_SIZE),
                                  (MIN_BOX_SIZE, MIN_BOX_SIZE, 0.0),
                                  (MIN_BOX_SIZE, 0.0, MIN_BOX_SIZE),
                                  (MIN_BOX_SIZE, MIN_BOX_SIZE, MIN_BOX_SIZE)]
        self.assertSetEqual(set(expected_active_boxes), set(active_boxes))

class TestGetAndSetBoxObject(helpers.BallparkTestCase):
    def test_setting_object_on_existing_box_succeeds(self):
        x, y, z = 0.0, 0.0, 0.0
        ball, = self.add_balls(1)
        ball.x = 1
        ball.y = 1
        ball.z = 1
        d = {"foo": 3.14}
        self.assertTrue(self.park.SetBoxObject(x, y, z, d))

    def test_setting_object_on_nonexisting_box_fails(self):
        x, y, z = 0.0, 0.0, 0.0
        d = {"foo": 3.14}
        self.assertFalse(self.park.SetBoxObject(x, y, z, d))

    def test_getting_object_that_has_not_been_set_returns_none(self):
        self.assertEqual(self.park.GetBoxObject(0.0, 0.0, 0.0), None)

    def test_can_retrieve_stored_object(self):
        x, y, z = 0.0, 0.0, 0.0
        ball, = self.add_balls(1)
        ball.x = 1
        ball.y = 1
        ball.z = 1
        d = {"foo": 3.14}
        self.park.SetBoxObject(x, y, z, d)
        self.assertDictEqual(self.park.GetBoxObject(0.0, 0.0, 0.0), d)


class TestGetBoxCenter(helpers.BallparkTestCase):
    def test_get_box_on_topmost_level(self):
        x, y, z = 0.0, 0.0, 0.0
        topmost_level = 7
        center_of_box = self.park.GetBoxCenter(topmost_level, x, y, z)
        self.assertEqual(center_of_box, (MIN_BOX_SIZE / 2.0, MIN_BOX_SIZE / 2.0, MIN_BOX_SIZE / 2.0))


class TestGetBubbleAtCoordinates(helpers.BallparkTestCase):
    def test_get_bubble_where_none_exists_returns_none(self):
        x, y, z = 0.0, 0.0, 0.0
        bubble = self.park.GetBubbleAtCoordinates(x, y, z)
        self.assertEqual(bubble, None)

    def test_get_existing_bubble(self):
        x, y, z = 0.0, 0.0, 0.0
        ball = helpers.add_ball_to_park(self.park, x=x, y=y, z=z)
        bubble = self.park.GetBubbleAtCoordinates(x, y, z)
        self.assertEqual(bubble, ball.newBubbleId)
