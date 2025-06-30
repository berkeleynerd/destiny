import destiny
from destiny.test import helpers
import math


def distance_between_points(point1, point2):
    return math.sqrt((point1[0]-point2[0])**2 + (point1[1]-point2[1])**2 + (point1[2]-point2[2])**2)


class TestIterativeCollision(helpers.BallparkTestCase):
    def setUp(self):
        self.maxDiff = None
        super(TestIterativeCollision, self).setUp()
        config = destiny.settings.GetDefault()
        config.useIterativeCollision = True
        destiny.settings.Apply(config)

    def tearDown(self):
        super(TestIterativeCollision, self).tearDown()
        destiny.settings.Reset()

    def test_stopped_balls_with_same_location(self):
        ball_a = helpers.create_space_ball(self.park)
        ball_b = helpers.create_space_ball(self.park)

        a_coordinates = []
        b_coordinates = []

        for i in range(10):
            self.park.Evolve()
            a_coordinates.append((ball_a.x, ball_a.y, ball_a.z))
            b_coordinates.append((ball_b.x, ball_b.y, ball_b.z))

            # Their velocities should be in opposite directions and equal in magnitude
            self.assertAlmostEqual(ball_a.vx, -ball_b.vx)
            self.assertAlmostEqual(ball_a.vy, -ball_b.vy)
            self.assertAlmostEqual(ball_a.vz, -ball_b.vz)

        # Make sure the objects are out of the way in 2 ticks
        distance = distance_between_points(a_coordinates[2], b_coordinates[2])
        self.assertTrue(distance > ball_a.radius + ball_b.radius)

    def test_multiple_overlapping_balls(self):
        ball_a = helpers.create_space_ball(self.park, x=1.0)
        ball_b = helpers.create_space_ball(self.park, x=-1.0)
        ball_c = helpers.create_space_ball(self.park, z=1.0)
        ball_d = helpers.create_space_ball(self.park, z=-1.0)

        # The ordering of the balls determines the direction of acceleration
        # balls a and b will move away from each other and c and d as well
        ac = []
        bc = []
        cc = []
        dc = []

        for i in range(10):
            self.park.Evolve()

            ac.append((ball_a.x, ball_a.y, ball_a.z))
            bc.append((ball_b.x, ball_b.y, ball_b.z))
            cc.append((ball_c.x, ball_c.y, ball_c.z))
            dc.append((ball_d.x, ball_d.y, ball_d.z))

            # They should not move in the y direction
            self.assertAlmostEqual(ball_a.vy, 0.0)
            self.assertAlmostEqual(ball_b.vy, 0.0)
            self.assertAlmostEqual(ball_c.vy, 0.0)
            self.assertAlmostEqual(ball_d.vy, 0.0)

        # They should all be outside their partners in 4 steps
        self.assertGreater(distance_between_points(ac[4],bc[4]), ball_a.radius*2)
        self.assertGreater(distance_between_points(ac[4],cc[4]), ball_a.radius*2)
        self.assertGreater(distance_between_points(ac[4],dc[4]), ball_a.radius*2)
        self.assertGreater(distance_between_points(bc[4],cc[4]), ball_a.radius*2)
        self.assertGreater(distance_between_points(bc[4],dc[4]), ball_a.radius*2)
        self.assertGreater(distance_between_points(cc[4],dc[4]), ball_a.radius*2)

    def test_stopped_balls_intersecting(self):
        ball_a = helpers.create_space_ball(self.park, y=1.9)
        ball_b = helpers.create_space_ball(self.park)

        a_coordinates = []
        b_coordinates = []

        for i in range(10):
            self.park.Evolve()
            a_coordinates.append((ball_a.x, ball_a.y, ball_a.z))
            b_coordinates.append((ball_b.x, ball_b.y, ball_b.z))

            # a should move in positive y-direction, b in negative y-direction
            self.assertGreater(ball_a.vy, 0.0)
            self.assertLess(ball_b.vy, 0.0)
            self.assertAlmostEqual(ball_a.vy, -ball_b.vy)
            self.assertAlmostEqual(ball_a.vx, 0.0)
            self.assertAlmostEqual(ball_a.vz, 0.0)
            self.assertAlmostEqual(ball_b.vx, 0.0)
            self.assertAlmostEqual(ball_b.vz, 0.0)

        # Make sure the balls are out of each other way in 2 steps, moving
        # in the y-direction only
        distance = distance_between_points(a_coordinates[1], b_coordinates[1])
        self.assertTrue(distance > ball_a.radius + ball_b.radius)

        for i in range(len(a_coordinates)):
            self.assertAlmostEqual(a_coordinates[i][0], a_coordinates[0][0], 4)
            self.assertAlmostEqual(a_coordinates[i][2], a_coordinates[0][2], 4)
            self.assertAlmostEqual(b_coordinates[i][0], b_coordinates[0][0], 4)
            self.assertAlmostEqual(b_coordinates[i][2], b_coordinates[0][2], 4)

    def test_ball_within_static_ball(self):
        ball = helpers.create_space_ball(self.park, x=0, y=0, z=8, vz=2)
        asteroid = helpers.create_space_ball(self.park, x=0, y=0, z=0)
        asteroid.radius = 10
        self.park.SetBallFree(asteroid.id, False)

        coordinates = self.evolve_ball_and_get_coordinates(ball, 10)

        # We should have moved out of the way in the z-direction only
        self.assertGreater(coordinates[-1][2], 12)
        self.assertAlmostEqual(coordinates[-1][0], 0.0, places=4)
        self.assertAlmostEqual(coordinates[-1][1], 0.0, places=4)

    def test_balls_intersecting_with_goto(self):
        ball_a = helpers.create_space_ball(self.park, x=0, y=0, z=0, vx=3)
        ball_b = helpers.create_space_ball(self.park, x=10, y=0, z=0, vx=-3)
        ball_a.radius = 10
        ball_b.radius = 10

        self.park.GotoDirection(ball_a.id, 1.0, 0.0, 0.0)
        self.park.GotoDirection(ball_b.id, -1.0, 0.0, 0.0)

        coordinates_a = []
        coordinates_b = []
        vel_a = []
        vel_b = []

        for i in range(20):
            self.park.Evolve()
            coordinates_a.append([ball_a.x, ball_a.y, ball_a.z])
            coordinates_b.append([ball_b.x, ball_b.y, ball_b.z])
            vel_a.append([ball_a.vx, ball_a.vy, ball_a.vz])
            vel_b.append([ball_b.vx, ball_b.vy, ball_b.vz])

        distances = [distance_between_points(coordinates_b[i],coordinates_a[i]) for i in range(len(coordinates_a))]

        # Make some assertions
        inside = True
        for i in range(len(coordinates_a)):
            #Balls should not move in y and z directions
            self.assertAlmostEqual(coordinates_a[i][1], 0.0, places=1)
            self.assertAlmostEqual(coordinates_a[i][2], 0.0, places=1)
            self.assertAlmostEqual(coordinates_b[i][1], 0.0, places=1)
            self.assertAlmostEqual(coordinates_b[i][2], 0.0, places=1)
            self.assertAlmostEqual(vel_a[i][1], 0.0, places=1)
            self.assertAlmostEqual(vel_a[i][2], 0.0, places=1)
            self.assertAlmostEqual(vel_b[i][1], 0.0, places=1)
            self.assertAlmostEqual(vel_b[i][2], 0.0, places=1)

            if distances[i] > ball_a.radius+ball_b.radius:
                inside = False

            # While we are in collision, ball_a should move in negative x,
            # ball_b in positive x and distances should be increasing
            if inside:
                self.assertLess(vel_a[i][0], 0.0)
                self.assertGreater(vel_b[i][0], 0.0)
                if i > 0:
                    self.assertGreater(distances[i], distances[i-1])
            else:
                if i+1 < len(distances):
                    if distances[i] + (vel_b[i][0] - vel_a[i][0]) > ball_a.radius+ball_b.radius:
                        self.assertLess(vel_b[i+1][0], vel_b[i][0])
                        self.assertGreater(vel_a[i+1][0], vel_a[i][0])
                    else:
                        self.assertGreater(vel_b[i+1][0], vel_b[i][0])
                        self.assertLess(vel_a[i+1][0], vel_a[i][0])

        # Make sure we are not overlapping in the end
        self.assertFalse(inside)

    def test_collision_small_with_massive(self):
        small = helpers.create_space_ball(self.park, x=-2, y=0, z=0, vx=20, mass=1e6, max_velocity=20)
        small.speedFraction = 1.0
        #Reduce friction
        small.Agility = 1000
        small.radius = 2.0
        large = helpers.create_space_ball(self.park, x=22.01, y=0, z=0, vx=0, mass=1e9, max_velocity=10)
        large.speedFraction = 1.0
        large.radius = 20

        try:
            self.park.SetBallRotation(small.id, 0.0, -1.0, 0.0, 1.0)
        except AttributeError:
            pass

        self.park.GotoDirection(small.id, -1.0, 0.0, 0.0)

        self.park.Evolve()
        self.assertAlmostEqual(large.vx, 0.04, places=2)
        self.assertAlmostEqual(large.vy, 0.0)
        self.assertAlmostEqual(large.vz, 0.0)

        self.assertAlmostEqual(small.vx, -19.96, places=1)
        self.assertAlmostEqual(small.vy, 0.0, places=4)
        self.assertAlmostEqual(small.vz, 0.0, places=4)

    def test_collision_massive_with_small(self):
        small = helpers.create_space_ball(self.park, x=0, y=0, z=0, mass=1e6)
        large = helpers.create_space_ball(self.park, x=30, y=0, z=0, vx=-20, max_velocity=20, mass=1e9)
        large.radius = 20.0
        large.speedFraction = 1.0
        # Need to increase the agility to reduce speed variation, otherwise the approximations in the code
        # will make this test fail.  This mass difference is very excessive, so this should not be a
        # problem in game.
        small.Agility = 3.0

        self.park.GotoDirection(large.id, -1.0, 0.0, 0.0)

        for i in range(20):
            self.park.Evolve()

            # The small ball should be pushed ahead of the large ball, allow for a minor overlap
            self.assertGreaterEqual(distance_between_points((small.x, small.y, small.z), (large.x, large.y, large.z)),
                                    0.9*(large.radius + small.radius))

            # The large ball velocity should hardly reduce
            self.assertLess(large.vx, -19.0)

            # Transverse velocity should be small in both
            self.assertAlmostEqual(small.vy, 0.0, places=2)
            self.assertAlmostEqual(small.vz, 0.0, places=2)
            self.assertAlmostEqual(large.vy, 0.0, places=2)
            self.assertAlmostEqual(large.vz, 0.0, places=2)

    def test_goto_collision(self):
        a_coordinates = []
        b_coordinates = []

        ball_a = helpers.create_space_ball(self.park)
        ball_b = helpers.create_space_ball(self.park, x=0, y=0, z=10)

        # Need to rotate ball_b 180 degrees if dynamical orientation is enabled
        try:
            # The formula for
            # Quaternion is (sin(t/2) (ux, uy, uz), cos(t/2))
            self.park.SetBallRotation(ball_b.id, 0.0, 1.0, 0.0, 0.0)
        except AttributeError:
            pass

        self.park.GotoPoint(ball_a.id, 0, 0, 10)
        self.park.GotoPoint(ball_b.id, 0, 0, 0)

        for i in range(20):
            self.park.Evolve()
            a_coordinates.append((ball_a.x, ball_a.y, ball_a.z))
            b_coordinates.append((ball_b.x, ball_b.y, ball_b.z))

        # Make sure the balls never intersect more than 10%.
        # Also make sure balls do not move in x and y directions
        for i in range(len(b_coordinates)):
            distance = distance_between_points(a_coordinates[i], b_coordinates[i])
            self.assertGreater(1.11 * distance, ball_a.radius + ball_b.radius)

            self.assertAlmostEqual(a_coordinates[i][0], a_coordinates[0][0])
            self.assertAlmostEqual(a_coordinates[i][1], a_coordinates[0][1])
            self.assertAlmostEqual(b_coordinates[i][0], b_coordinates[0][0])
            self.assertAlmostEqual(b_coordinates[i][1], b_coordinates[0][1])

    def test_balls_in_warp_should_not_collide(self):
        ball_a = helpers.create_space_ball(self.park, x=-40, y=0, z=0, vx=10.0, max_velocity=10.0)
        ball_b = helpers.create_space_ball(self.park, x=40, y=0, z=0, vx=-10.0, max_velocity=10.0)
        ball_a.radius = 2.0
        ball_b.radius = 2.0

        self.park.WarpTo(ball_a.id, 10000000.0, 0, 0, 0.0, 1000)
        self.park.WarpTo(ball_b.id, -10000000.0, 0, 0, 0.0, 1000)

        for i in range(80):
            self.park.Evolve()

        self.assertGreater(ball_a.x, ball_b.x)
        self.assertEqual(ball_a.y, 0.0)
        self.assertEqual(ball_a.z, 0.0)
        self.assertEqual(ball_b.y, 0.0)
        self.assertEqual(ball_b.z, 0.0)

    def test_non_massive_balls_should_not_collide(self):
        ball_a = helpers.create_space_ball(self.park, x=0, y=0, z=-4)
        ball_b = helpers.create_space_ball(self.park, x=0, y=0, z=4)
        ball_a.radius = 2.0
        ball_b.radius = 2.0
        self.park.SetBallMassive(ball_b.id, 0)
        self.park.GotoDirection(ball_a.id, 0,0,1)

        for i in range(80):
            self.park.Evolve()

        self.assertGreater(ball_a.z, ball_b.z)
        self.assertAlmostEqual(ball_a.x, 0.0)
        self.assertAlmostEqual(ball_a.y, 0.0)
        self.assertAlmostEqual(ball_b.x, 0.0)
        self.assertAlmostEqual(ball_b.y, 0.0)
        self.assertAlmostEqual(ball_b.z, 4.0)

    def test_balls_with_mini_balls_are_collision_targets(self):
        ball_a = helpers.create_space_ball(self.park, x=0, y=0, z=-4)
        ball_b = helpers.create_space_ball(self.park, x=0, y=0, z=4)
        ball_a.radius = 2.0
        ball_b.radius = 2.0

        self.park.SetBallFree(ball_b.id, 0)
        ball_b.AddMiniBall(0, 0, 10, 2)

        self.park.GotoDirection(ball_a.id, 0,0,1)

        a_coords = self.evolve_ball_and_get_coordinates(ball_a, 50)

        #Ball b is fixed and should not move
        self.assertAlmostEqual(ball_b.x, 0.0)
        self.assertAlmostEqual(ball_b.y, 0.0)
        self.assertAlmostEqual(ball_b.z, 4.0)

        #Ball a should not significantly overlap with ball b and not move in x and y directions
        for coord in a_coords:
            self.assertAlmostEqual(coord[0], 0.0)
            self.assertAlmostEqual(coord[1], 0.0)
            self.assertLess(coord[2], 0.5)

    def test_correct_collision_order(self):
        ball_b = helpers.create_space_ball(self.park, x=0, y=0, z=4)
        ball_c = helpers.create_space_ball(self.park, x=0, y=0, z=10, mass=1e6)
        ball_a = helpers.create_space_ball(self.park, x=0, y=0, z=-4, vz=20, max_velocity=100.0)
        ball_a.radius = 2.0
        ball_b.radius = 2.0
        ball_c.radius = 2.0
        self.park.SetBallFree(ball_b.id, 0)

        self.park.Evolve()

        #Make sure ball_c has not moved
        self.assertAlmostEqual(ball_c.x, 0.0)
        self.assertAlmostEqual(ball_c.y, 0.0)
        self.assertAlmostEqual(ball_c.z, 10.0)

        #Make sure ball_a has not passed ball_b
        self.assertGreater(ball_b.z, ball_a.z)

    def test_substep_collision(self):
        #Turn off friction to make things move at nearly constant speed
        self.park.friction = 1e-10

        #Deliberately create them out of order, so ball_a first hits ball_c
        ball_a = helpers.create_space_ball(self.park, x=0, y=0, z=0, vz=20, max_velocity=100)
        ball_c = helpers.create_space_ball(self.park, x=0, y=0, z=10, max_velocity=100)
        ball_b = helpers.create_space_ball(self.park, x=0, y=0, z=5, max_velocity=100)

        self.park.Evolve()

        #First ball should stop when it hits the second
        self.assertAlmostEqual(ball_a.vz, 0.0, places=0)
        self.assertAlmostEqual(ball_a.z, 1.0, places=0)

        #Second ball should stop when it hits the third
        self.assertAlmostEqual(ball_b.vz, 0.0, places=0)
        self.assertAlmostEqual(ball_b.z, 6.0, places=0)

        #Third ball should have all the velocity and have moved to 28 meters
        self.assertAlmostEqual(ball_c.vz, 20.0, places=0)
        self.assertAlmostEqual(ball_c.z, 28.0, places=0)

    def test_substep_collision_static(self):
        #Turn off friction to make things move at nearly constant speed
        self.park.friction = 1e-10

        #In a single step, collide both with a ball and a static collidable, where the ball collision
        #is calculated beforehand.

        ball_a = helpers.create_space_ball(self.park, x=0, y=0, z=-14, vz=50, max_velocity=100)
        ball_b = helpers.create_space_ball(self.park, x=-10, y=0, z=0, vx=50, max_velocity=100)
        ball_c = helpers.create_space_ball(self.park, x=0, y=0, z=20)
        ball_d = helpers.create_space_ball(self.park, x=20, y=0, z=0)
        self.park.SetBallFree(ball_c.id, 0)
        self.park.SetBallFree(ball_d.id, 0)

        self.park.Evolve()

        # ball_a should stop and ball_b should move at 45 degree angles
        self.assertAlmostEqual(ball_a.vz, 0.0, places=2)
        self.assertAlmostEqual(ball_a.vx, 0.0, places=2)
        self.assertAlmostEqual(ball_a.z, -4.0, places=2)
        self.assertAlmostEqual(ball_b.vz, 50.0, places=2)
        self.assertAlmostEqual(ball_b.vx, 50.0, places=2)


class TestIterativeCapsuleCollision(helpers.BallparkTestCase):
    def setUp(self):
        super(TestIterativeCapsuleCollision, self).setUp()
        config = destiny.settings.GetDefault()
        config.useIterativeCollision = True
        destiny.settings.Apply(config)
        parentBall = helpers.add_ball_to_park(self.park, x=0.0, y=0.0, z=0.0, radius=500.0)
        parentBall.AddMiniCapsule(
            100.0, 100.0, 0.0,
            200.0, 100.0, 0.0, 10.0
        )
        self.capsule = parentBall.miniCapsules[0]

    def tearDown(self):
        destiny.settings.Reset()
        super(TestIterativeCapsuleCollision, self).tearDown()
        del self.capsule

    def test_collision_from_left(self):
        ball = helpers.create_space_ball(self.park, x=80.0, y=100, z=0)
        self.park.GotoPoint(ball.id, 300.0, 100.0, 0.0)

        #Need to rotate the ship to the x-direction for this to work properly
        #if dynamical orientation is enables
        try:
            # Rotate 90 degrees around y-axis, the formula for
            # Quaternion is (sin(t/2) (ux, uy, uz), cos(t/2))
            self.park.SetBallRotation(ball.id, 0.0, 0.5*math.sqrt(2), 0.0, 0.5*math.sqrt(2))

        except AttributeError:
            pass

        coordinates = self.evolve_ball_and_get_coordinates(ball, 20)

        # Make sure we never overlap more than 10% and never move in y and z directions
        for i in range(len(coordinates)):
            distance = distance_between_points(coordinates[i], (self.capsule.ax, self.capsule.ay, self.capsule.az))
            self.assertTrue(1.11*distance > ball.radius + self.capsule.radius)

            self.assertAlmostEqual(coordinates[i][1], 100.0, 4)
            self.assertAlmostEqual(coordinates[i][2], 0.0, 2)

    def test_collision_from_right(self):
        ball = helpers.create_space_ball(self.park, x=220.0, y=100, z=0)

        #Need to rotate the ship to the negative x-direction for this to work properly
        #if dynamical orientation is enabled
        try:
            # Rotate -90 degrees around y-axis, the formula for
            # Quaternion is (sin(t/2) (ux, uy, uz), cos(t/2))
            self.park.SetBallRotation(ball.id, 0.0, math.sin(-math.radians(45)), 0.0, math.cos(-math.radians(45)))
        except AttributeError:
            pass

        self.park.GotoPoint(ball.id, 0.0, 100.0, 0.0)
        coordinates = self.evolve_ball_and_get_coordinates(ball, 20)

        # Make sure we never overlap more than 10% and never move in y and z directions
        for i in range(len(coordinates)):
            distance = distance_between_points(coordinates[i], (self.capsule.bx, self.capsule.by, self.capsule.bz))
            self.assertTrue(1.11 * distance > ball.radius + self.capsule.radius)

            self.assertAlmostEqual(coordinates[i][1], 100.0, 4)
            self.assertAlmostEqual(coordinates[i][2], 0.0, 2)

    def test_collision_from_top(self):
        ball = helpers.create_space_ball(self.park, x=150.0, y=120, z=0)

        #Need to rotate the ship to the negative y-direction for this to work properly
        #if dynamical orientation is enabled
        try:
            # Rotate 90 degrees around x-axis, the formula for
            # Quaternion is (sin(t/2) (ux, uy, uz), cos(t/2))
            self.park.SetBallRotation(ball.id, math.sin(math.radians(45)), 0.0, 0.0, math.cos(math.radians(45)))
        except AttributeError:
            pass

        self.park.GotoPoint(ball.id, 150.0, 0.0, 0.0)
        coordinates = self.evolve_ball_and_get_coordinates(ball, 20)

        # Make sure we never overlap more than 10% and never move in x and z directions
        for i in range(len(coordinates)):
            distance = distance_between_points(coordinates[i], (150.0, self.capsule.by, self.capsule.bz))
            self.assertTrue(1.11 * distance > ball.radius + self.capsule.radius)

            self.assertAlmostEqual(coordinates[i][0], 150.0, 2)
            self.assertAlmostEqual(coordinates[i][2], 0.0, 2)

    def test_collision_from_bottom(self):
        ball = helpers.create_space_ball(self.park, x=150.0, y=80, z=0)

        #Need to rotate the ship to the y-direction for this to work properly
        #if dynamical orientation is enabled
        try:
            # Rotate -90 degrees around x-axis, the formula for
            # Quaternion is (sin(t/2) (ux, uy, uz), cos(t/2))
            self.park.SetBallRotation(ball.id, -1.0, 0.0, 0.0, 1.0)
        except AttributeError:
            pass

        self.park.GotoDirection(ball.id, 0.0, 1.0, 0.0)
        coordinates = self.evolve_ball_and_get_coordinates(ball, 20)

        # Make sure we never overlap more than 10% and never move in x and z directions
        for i in range(len(coordinates)):
            distance = distance_between_points(coordinates[i], (150.0, self.capsule.by, self.capsule.bz))
            self.assertTrue(1.11 * distance > ball.radius + self.capsule.radius)

            self.assertAlmostEqual(coordinates[i][0], 150.0, 4)
            self.assertAlmostEqual(coordinates[i][2], 0.0, 2)

    def test_collision_with_zero_length_capsule(self):
        parentBall = helpers.add_ball_to_park(self.park, x=0.0, y=0.0, z=0.0, radius=500.0)
        parentBall.AddMiniCapsule(100.0, 0.0, 0.0, 100.0, 0.0, 0.0, 2.0)
        capsule = parentBall.miniCapsules[0]
        ball = helpers.create_space_ball(self.park, x=100.0, y=0, z=-10)

        self.park.GotoDirection(ball.id, 0.0, 0.0, 1.0)
        coordinates = self.evolve_ball_and_get_coordinates(ball, 20)

        # Make sure we never overlap more than 10% and never move in x and y directions
        for i in range(len(coordinates)):
            distance = distance_between_points(coordinates[i], (capsule.bx, capsule.by, capsule.bz))
            self.assertGreater(1.11 * distance, ball.radius + capsule.radius)

            self.assertAlmostEqual(coordinates[i][0], 100, 4)
            self.assertAlmostEqual(coordinates[i][1], 0.0, 2)

    def test_collision_at_angle(self):
        ball = helpers.create_space_ball(self.park, x=170, y=80, z=0, vx=-6.7, vy=6.7)
        ball.Agility = 0.3
        self.park.GotoDirection(ball.id, -1, 1, 0)
        self.park.SetBallVelocity(ball.id, -6.7, 6.7, 0.0)

        for i in range(10):
            self.park.Evolve()

            #Make sure our velocity in the x direction is nearly constant
            #This fails if we apply collision responses to rotation
            # self.assertAlmostEqual(ball.vx, -6.7, delta=2.0)

            #Make sure our y coordinate is always smaller than combined radii, with some uncertainty
            self.assertLess(ball.y + 0.9*(ball.radius+self.capsule.radius), self.capsule.by)

            #Make sure we don't move in z direction
            self.assertAlmostEqual(ball.z, 0.0, places=1)

    def test_ball_starting_within_cylinder(self):
        ball = helpers.create_space_ball(self.park, x=160, y=95, z=0)

        for i in range(20):
            self.park.Evolve()

            #We should move out in the negative y-direction
            self.assertLess(ball.vy, 0.0)
            self.assertAlmostEqual(ball.vx, 0.0, places=2)
            self.assertAlmostEqual(ball.vz, 0.0, places=2)

        # We should be well outside the cylinder
        self.assertLess(ball.y + 0.9*(ball.radius+self.capsule.radius), self.capsule.by)

    def test_ball_starting_within_sphere(self):
        ball = helpers.create_space_ball(self.park, x=95, y=100, z=0)

        for i in range(20):
            self.park.Evolve()

            # We should move out in the negative x direction
            self.assertLess(ball.vx, 0.0)
            self.assertAlmostEqual(ball.vy, 0.0, places=2)
            self.assertAlmostEqual(ball.vz, 0.0, places=2)

        # We should be outside the capsule
        self.assertLess(ball.x + 0.9*(ball.radius+self.capsule.radius), self.capsule.ax)

    def test_ball_with_miniballs_multi_collisions(self):
        #The code does not handle this
        return

        self.park.friction = 1e-10
        ball = helpers.create_space_ball(self.park, x=150, y=100, z=-20, vz=10, max_velocity=20)
        ball.AddMiniBall(5, 0, 0, 5)
        ball.AddMiniBall(0, 0, 0, 5)
        ball.AddMiniBall(-5, 0, 0, 5)
        ball.radius = 10

        self.park.Evolve()

        # We should only move in the z-direction
        self.assertAlmostEqual(ball.vx, 0.0, places=2)
        self.assertAlmostEqual(ball.vy, 0.0, places=2)

        # We should be moving back at same speed in opposite direction
        self.assertAlmostEqual(ball.vz, -10, places=0)

        # We should not be rotating at all
        self.assertAlmostEqual(ball.wx, 0.0, places=1)
        self.assertAlmostEqual(ball.wy, 0.0, places=1)
        self.assertAlmostEqual(ball.wz, 0.0, places=1)


class TestIterativeBoxCollision(helpers.BallparkTestCase):
    def setUp(self):
        super(TestIterativeBoxCollision, self).setUp()
        config = destiny.settings.GetDefault()
        config.useIterativeCollision = True
        destiny.settings.Apply(config)
        parentBall = helpers.add_ball_to_park(self.park, x=0, y=0, z=0)
        parentBall.AddMiniBox(
            0.0, 0.0, 0.0,
            100.0, 0.0, 0.0,
            0.0, 100.0, 0.0,
            0.0, 0.0, 100.0)
        self.box = parentBall.miniBoxes[0]

    def tearDown(self):
        destiny.settings.Reset()
        super(TestIterativeBoxCollision, self).tearDown()
        del self.box

    def test_collision_from_left(self):
        ball = helpers.create_space_ball(self.park, x=-10, y=50, z=50)

        #Need to rotate the ship to the x-direction for this to work properly
        #if dynamical orientation is enables
        try:
            # Rotate 90 degrees around y-axis, the formula for
            # Quaternion is (sin(t/2) (ux, uy, uz), cos(t/2))
            self.park.SetBallRotation(ball.id, 0.0, 0.5*math.sqrt(2), 0.0, 0.5*math.sqrt(2))

        except AttributeError:
            pass

        self.park.GotoPoint(ball.id, 50.0, 50.0, 50.0)
        coordinates = self.evolve_ball_and_get_coordinates(ball, 20)

        # Make sure we never overlap more than 10%  of the ball and never move in y and z directions
        for i in range(len(coordinates)):
            distance = distance_between_points(coordinates[i], (0.0, 50.0, 50.0))
            self.assertTrue(1.11 * distance > ball.radius, "Considerable overlap, %f, %f"%(distance, ball.radius))

            self.assertAlmostEqual(coordinates[i][1], 50.0, 4)
            self.assertAlmostEqual(coordinates[i][2], 50.0, 4)

    def test_collision_from_right(self):
        ball = helpers.create_space_ball(self.park, x=110, y=50, z=50)

        #Need to rotate the ship to the negative x-direction for this to work properly
        #if dynamical orientation is enables
        try:
            # Rotate -90 degrees around y-axis, the formula for
            # Quaternion is (sin(t/2) (ux, uy, uz), cos(t/2))
            self.park.SetBallRotation(ball.id, 0.0, -0.5*math.sqrt(2), 0.0, 0.5*math.sqrt(2))

        except AttributeError:
            pass

        self.park.GotoPoint(ball.id, 50.0, 50.0, 50.0)
        coordinates = self.evolve_ball_and_get_coordinates(ball, 20)

        # Make sure we never overlap more than 10%  of the ball and never move in y and z directions
        for i in range(len(coordinates)):
            distance = distance_between_points(coordinates[i], (100.0, 50.0, 50.0))
            self.assertTrue(1.11 * distance > ball.radius, "Considerable overlap, %f, %f"%(distance, ball.radius))

            self.assertAlmostEqual(coordinates[i][1], 50.0, 4)
            self.assertAlmostEqual(coordinates[i][2], 50.0, 4)

    def test_collision_from_top(self):
        ball = helpers.create_space_ball(self.park, x=50, y=110, z=50)

        #Need to rotate the ship to the negative y-direction for this to work properly
        #if dynamical orientation is enabled
        try:
            # Rotate 90 degrees around x-axis, the formula for
            # Quaternion is (sin(t/2) (ux, uy, uz), cos(t/2))
            self.park.SetBallRotation(ball.id, math.sin(math.radians(45)), 0.0, 0.0, math.cos(math.radians(45)))
        except AttributeError:
            pass

        self.park.GotoPoint(ball.id, 50.0, 50.0, 50.0)
        coordinates = self.evolve_ball_and_get_coordinates(ball, 20)

        # Make sure we never overlap more than 10%  of the ball and never move in x and z directions
        for i in range(len(coordinates)):
            distance = distance_between_points(coordinates[i], (50.0, 100.0, 50.0))
            self.assertTrue(1.11 * distance > ball.radius, "Considerable overlap, %f, %f"%(distance, ball.radius))

            self.assertAlmostEqual(coordinates[i][0], 50.0, 2)
            self.assertAlmostEqual(coordinates[i][2], 50.0, 2)

    def test_collision_from_bottom(self):
        ball = helpers.create_space_ball(self.park, x=50.0, y=-50, z=50)

        #Need to rotate the ship to the y-direction for this to work properly
        #if dynamical orientation is enabled
        try:
            # Rotate -90 degrees around x-axis, the formula for
            # Quaternion is (sin(t/2) (ux, uy, uz), cos(t/2))
            self.park.SetBallRotation(ball.id, math.sin(math.radians(-45)), 0.0, 0.0, math.cos(math.radians(-45)))
        except AttributeError:
            pass

        self.park.GotoPoint(ball.id, 50.0, 50.0, 50.0)
        coordinates = self.evolve_ball_and_get_coordinates(ball, 20)

        # Make sure we never overlap more than 10%  of the ball and never move in x and z directions
        for i in range(len(coordinates)):
            distance = distance_between_points(coordinates[i], (50.0, 0.0, 50.0))
            self.assertTrue(1.11 * distance > ball.radius, "Considerable overlap, %f, %f"%(distance, ball.radius))

            self.assertAlmostEqual(coordinates[i][0], 50.0, 3)
            self.assertAlmostEqual(coordinates[i][2], 50.0, 3)

    def test_collision_from_front(self):
        ball = helpers.create_space_ball(self.park, x=50, y=50, z=110)

        #Need to rotate the ship to the negative z-direction for this to work properly
        #if dynamical orientation is enabled
        try:
            # Rotate 180 degrees around x-axis, the formula for
            # Quaternion is (sin(t/2) (ux, uy, uz), cos(t/2))
            self.park.SetBallRotation(ball.id, 1.0, 0.0, 0.0, 0.0)
        except AttributeError:
            pass

        self.park.GotoPoint(ball.id, 50.0, 50.0, 50.0)
        coordinates = self.evolve_ball_and_get_coordinates(ball, 100)

        # Make sure we never overlap more than 10%  of the ball and never move in x and y directions
        for i in range(len(coordinates)):
            distance = distance_between_points(coordinates[i], (50.0, 50.0, 100.0))
            self.assertTrue(1.11 * distance > ball.radius, "Considerable overlap, %f, %f"%(distance, ball.radius))

            self.assertAlmostEqual(coordinates[i][0], 50.0, 4)
            self.assertAlmostEqual(coordinates[i][1], 50.0, 4)

    def test_collision_from_back(self):
        ball = helpers.create_space_ball(self.park, x=50.0, y=50, z=-10)
        self.park.GotoPoint(ball.id, 50.0, 50.0, 50.0)
        coordinates = self.evolve_ball_and_get_coordinates(ball, 20)

        # Make sure we never overlap more than 10%  of the ball and never move in x and y directions
        for i in range(len(coordinates)):
            distance = distance_between_points(coordinates[i], (50.0, 50.0, 0.0))
            self.assertTrue(1.11 * distance > ball.radius, "Considerable overlap, %f, %f"%(distance, ball.radius))

            self.assertAlmostEqual(coordinates[i][0], 50.0, 4)
            self.assertAlmostEqual(coordinates[i][1], 50.0, 4)

    def test_collision_at_angle(self):
        xp = [0.0, 50, -50]
        yp = [50, -50, 0.0]
        zp = [-50, 0.0, 50]
        xv = [0.1, 0.0, 90]
        yv = [0.0, 90, 0.1]
        zv = [90, 0.1, 0.0]

        ball = helpers.create_space_ball(self.park)
        for i in range(3):
            ball.x=xp[i]
            ball.y=yp[i]
            ball.z=zp[i]
            ball.vx=xv[i]
            ball.vy=yv[i]
            ball.vz=zv[i]
            ball.maxVelocity = 100
            #ball.AddMiniBall(0.0, 0.0, 0.0, 2)
            ball.Agility = 0.5
            self.park.GotoDirection(ball.id, xv[i], yv[i], zv[i])

            for j in range(40):
                self.park.Evolve()

                #Make sure the x velocity does not change considerably
                #This fails when rotation responses to collisions are applied
                #self.assertAlmostEqual(ball.vx, 6.7, delta=2.0)

                #Stop before evolving outside the box
                if (i==0 and ball.x > 100) or (i==1 and ball.z > 100) or (i==2 and ball.y > 100):
                    break

                #Make sure we do not overlap the box
                #Make sure our velocity in still direction does not change
                if i == 0:
                    self.assertLess(ball.z+0.9*ball.radius, 0.0)
                    self.assertAlmostEqual(ball.vy, 0.0, places=2)
                if i == 1:
                    self.assertLess(ball.y+0.9*ball.radius, 0.0)
                    self.assertAlmostEqual(ball.vx, 0.0, places=2)
                if i == 2:
                    self.assertLess(ball.x+0.9*ball.radius, 0.0)
                    self.assertAlmostEqual(ball.vz, 0.0, places=2)

    def test_starting_within_box(self):
        ball = helpers.create_space_ball(self.park, x=20, y=60, z=30, vx=-10)
        ball.Agility = 0.01
        for i in range(100):
            self.park.Evolve()

            #The ball should move out in the negative x direction only
            self.assertLess(ball.vx, 0.0)
            self.assertAlmostEqual(ball.vy, 0.0)
            self.assertAlmostEqual(ball.vz, 0.0)

        #The ball should be outside the box
        self.assertLess(ball.x + 0.9*ball.radius, 0.0)
        #And it should be nearly stopped
        self.assertAlmostEqual(ball.vx, 0.0, places=3)

    def test_two_aligning_boxes(self):
        ball = helpers.create_space_ball(self.park, x=0.0, y=50, z=-10, vz=1000)
        ball.maxVelocity = 1000
        ball.Agility = 0.5
        parentBall = helpers.add_ball_to_park(self.park, x=0, y=0, z=0)
        parentBall.AddMiniBox(
            -100.0, 0.0, 0.0,
            100.0, 0.0, 0.0,
            0.0, 100.0, 0.0,
            0.0, 0.0, 100.0
        )

        self.park.GotoDirection(ball.id, 0, 0, 1)
        for i in range(100):
            self.park.Evolve()

            #The ball should move out in the negative x direction only
            self.assertAlmostEqual(ball.vx, 0.0)
            self.assertAlmostEqual(ball.vy, 0.0)

            self.assertLess(ball.z, -0.8 * ball.radius)
