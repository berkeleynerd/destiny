import destiny
from destiny.test import helpers

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


class TestBallBox(helpers.BallparkTestCase):
    def test_box_zero_zero_zero(self):
        ball, = self.add_balls(1)
        ball.x = 1.0
        ball.y = 1.0
        ball.z = 1.0
        box_width, (x,y,z) = self.park.GetBallBox(ball.id)
        self.assertEqual(box_width, 15.0)
        self.assertAlmostEqual(x, 7.5)
        self.assertAlmostEqual(y, 7.5)
        self.assertAlmostEqual(z, 7.5)

    def test_box_zero_zero_one(self):
        ball, = self.add_balls(1)
        ball.x = 1.0
        ball.y = 1.0
        ball.z = 16.0
        box_width, (x,y,z) = self.park.GetBallBox(ball.id)
        self.assertEqual(box_width, 15.0)
        self.assertAlmostEqual(x, 7.5)
        self.assertAlmostEqual(y, 7.5)
        self.assertAlmostEqual(z, 22.5)

class TestGetStaticCollidableBox(helpers.BallparkTestCase):
    def test_capsule_in_box_zero_zero_zero(self):
        capsule = self.park.AddCapsule(-1, 1.0,1.0,1.0, 2.0,2.0,2.0, 1.0)
        box_width, (x,y,z) = self.park.GetStaticCollidableBox(capsule.id)
        self.assertEqual(box_width, 15.0)
        self.assertAlmostEqual(x, 7.5)
        self.assertAlmostEqual(y, 7.5)
        self.assertAlmostEqual(z, 7.5)

    def test_capsule_in_box_zero_zero_one(self):
        capsule = self.park.AddCapsule(-1, 1.0,1.0,16.0, 1.0,1.0,18.0, 1.0)
        box_width, (x,y,z) = self.park.GetStaticCollidableBox(capsule.id)
        self.assertEqual(box_width, 15.0)
        self.assertAlmostEqual(x, 7.5)
        self.assertAlmostEqual(y, 7.5)
        self.assertAlmostEqual(z, 22.5)

    def test_oriented_box_in_box_zero_zero_zero(self):
        obb = self.park.AddOrientedBox(-1,
                                       7.0, 7.0, 7.0,
                                       1.0, 0.0, 0.0,
                                       0.0, 1.0, 0.0,
                                       0.0, 0.0, 1.0)
        box_width, (x,y,z) = self.park.GetStaticCollidableBox(obb.id)
        self.assertEqual(box_width, 15.0)
        self.assertAlmostEqual(x, 7.5)
        self.assertAlmostEqual(y, 7.5)
        self.assertAlmostEqual(z, 7.5)

    def test_oriented_box_in_box_zero_zero_one(self):
        obb = self.park.AddOrientedBox(-1,
                                       7.0, 7.0, 22.0,
                                       1.0, 0.0, 0.0,
                                       0.0, 1.0, 0.0,
                                       0.0, 0.0, 1.0)
        box_width, (x,y,z) = self.park.GetStaticCollidableBox(obb.id)
        self.assertEqual(box_width, 15.0)
        self.assertAlmostEqual(x, 7.5)
        self.assertAlmostEqual(y, 7.5)
        self.assertAlmostEqual(z, 22.5)


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
        self.assertEqual(level_width, 15.0)
        self.assertEqual(box_low_coord_list, [(0.0, 0.0, 0.0)])

    def test_adding_a_capsule_makes_a_box_active(self):
        self.park.AddCapsule(0, 2.0,2.0,2.0, 3.0,3.0,3.0, 1.0)
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0)]
        self.assertEqual(active_boxes, expected_active_boxes)

    def test_adding_a_large_capsule_makes_adjecent_boxes_active(self):
        self.park.AddCapsule(0, 1.0,1.0,1.0, 2.0,2.0,2.0, 2.0)
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [
            (-15.0, -15.0, -15.0),
            (-15.0, -15.0, 0.0),
            (-15.0, 0.0, -15.0),
            (-15.0, 0.0, 0.0),
            (0.0, -15.0, -15.0),
            (0.0, -15.0, 0.0),
            (0.0, 0.0, -15.0),
            (0.0, 0.0, 0.0)
        ]
        self.assertListEqual(sorted(active_boxes), expected_active_boxes)

    def test_adding_a_long_capsule_makes_adjecent_boxes_active(self):
        self.park.AddCapsule(0, 7.5,7.5,7.5, 100.0,7.5,7.5, 5.0)
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [
            (0.0, 0.0, 0.0),
            (15.0, 0.0, 0.0),
            (30.0, 0.0, 0.0),
            (45.0, 0.0, 0.0),
            (60.0, 0.0, 0.0),
            (75.0, 0.0, 0.0),
            (90.0, 0.0, 0.0)
        ]
        self.assertListEqual(sorted(active_boxes), expected_active_boxes)


class TestActiveBoxesForOrientedBox(helpers.BallparkTestCase):
    def test_adding_a_box_makes_a_box_active(self):
        self.park.AddOrientedBox(-1,
                                 2.0, 2.0, 2.0,
                                 1.0, 0.0, 0.0,
                                 0.0, 1.0, 0.0,
                                 0.0, 0.0, 1.0)
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0)]
        self.assertEqual(expected_active_boxes, active_boxes)

    def test_tight_fit(self):
        self.park.AddOrientedBox(-1,
                                 0.0, 0.0, 0.0,
                                 10.0, 0.0, 0.0,
                                 0.0, 10.0, 0.0,
                                 0.0, 0.0, 10.0)
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0)]
        self.assertEqual(expected_active_boxes, active_boxes)

    def test_level_six(self):
        self.park.AddOrientedBox(-1,
                                 0.0, 0.0, 0.0,
                                 10.1, 0.0, 0.0,
                                 0.0, 10.1, 0.0,
                                 0.0, 0.0, 10.1)
        level = 6
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0)]
        self.assertEqual(expected_active_boxes, active_boxes)

    def test_intersect_left(self):
        self.park.AddOrientedBox(-1,
                                 -1.0, 0.0, 0.0,
                                 5.0, 0.0, 0.0,
                                 0.0, 5.0, 0.0,
                                 0.0, 0.0, 5.0)
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0), (-15.0, 0.0, 0.0)]
        self.assertEqual(expected_active_boxes, active_boxes)

    def test_intersect_right(self):
        self.park.AddOrientedBox(-1,
                                 11.0, 0.0, 0.0,
                                 5.0, 0.0, 0.0,
                                 0.0, 5.0, 0.0,
                                 0.0, 0.0, 5.0)
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0), (15.0, 0.0, 0.0)]
        self.assertEqual(expected_active_boxes, active_boxes)

    def test_intersect_bottom(self):
        self.park.AddOrientedBox(-1,
                                 0.0, -1.0, 0.0,
                                 5.0, 0.0, 0.0,
                                 0.0, 5.0, 0.0,
                                 0.0, 0.0, 5.0)
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0), (0.0, -15.0, 0.0)]
        self.assertEqual(expected_active_boxes, active_boxes)

    def test_intersect_top(self):
        self.park.AddOrientedBox(-1,
                                 0.0, 11.0, 0.0,
                                 5.0, 0.0, 0.0,
                                 0.0, 5.0, 0.0,
                                 0.0, 0.0, 5.0)
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0), (0.0, 15.0, 0.0)]
        self.assertEqual(expected_active_boxes, active_boxes)

    def test_intersect_back(self):
        self.park.AddOrientedBox(-1,
                                 0.0, 0.0, -1.0,
                                 5.0, 0.0, 0.0,
                                 0.0, 5.0, 0.0,
                                 0.0, 0.0, 5.0)
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0), (0.0, 0.0, -15.0)]
        self.assertEqual(expected_active_boxes, active_boxes)

    def test_intersect_front(self):
        self.park.AddOrientedBox(-1,
                                 0.0, 0.0, 11.0,
                                 5.0, 0.0, 0.0,
                                 0.0, 5.0, 0.0,
                                 0.0, 0.0, 5.0)
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0), (0.0, 0.0, 15.0)]
        self.assertEqual(expected_active_boxes, active_boxes)

    def test_intersect_top_front_right(self):
        self.park.AddOrientedBox(-1,
                                 11.0, 11.0, 11.0,
                                 5.0, 0.0, 0.0,
                                 0.0, 5.0, 0.0,
                                 0.0, 0.0, 5.0)
        level = 7
        level_width, active_boxes = self.park.GetActiveBoxes(level)
        expected_active_boxes =  [(0.0, 0.0, 0.0),
                                  (15.0, 0.0, 0.0),
                                  (0.0, 15.0, 0.0),
                                  (0.0, 0.0, 15.0),
                                  (0.0, 15.0, 15.0),
                                  (15.0, 15.0, 0.0),
                                  (15.0, 0.0, 15.0),
                                  (15.0, 15.0, 15.0)]
        self.assertEqual(expected_active_boxes, active_boxes)

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
        half_width = 7.5
        center_of_box = self.park.GetBoxCenter(topmost_level, x, y, z)
        self.assertEqual(center_of_box, (half_width, half_width, half_width))


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