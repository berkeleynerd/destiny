from destiny.test import helpers


class TestCollision(helpers.BallparkTestCase):
    def setUp(self):
        self.maxDiff = None
        super(TestCollision, self).setUp()

    def test_stopped_balls_with_same_location(self):
        ball_a = helpers.create_space_ball(self.park)
        ball_b = helpers.create_space_ball(self.park)

        a_coordinates = []
        b_coordinates = []

        for i in xrange(10):
            self.park.Evolve()
            a_coordinates.append((ball_a.x, ball_a.y, ball_a.z))
            b_coordinates.append((ball_b.x, ball_b.y, ball_b.z))

        expected_a_coordinates = [
            (1.121144335565424, 0.0, 0.0),
            (3.2401005010556143, 0.0, 0.0),
            (5.185473058408457, 0.0, 0.0),
            (6.9714818718687654, 0.0, 0.0),
            (8.61118192299853, 0.0, 0.0),
            (10.116558737162107, 0.0, 0.0),
            (11.498615992731432, 0.0, 0.0),
            (12.76745595339809, 0.0, 0.0),
            (13.93235331151902, 0.0, 0.0),
            (15.001822982259961, 0.0, 0.0)
        ]

        expected_b_coordinates = [
            (-1.121144335565424, 0.0, 0.0),
            (-3.2401005010556143, 0.0, 0.0),
            (-5.185473058408457, 0.0, 0.0),
            (-6.9714818718687654, 0.0, 0.0),
            (-8.61118192299853, 0.0, 0.0),
            (-10.116558737162107, 0.0, 0.0),
            (-11.498615992731432, 0.0, 0.0),
            (-12.76745595339809, 0.0, 0.0),
            (-13.93235331151902, 0.0, 0.0),
            (-15.001822982259961, 0.0, 0.0)
        ]

        self.assertListOfPointsAlmostEqual(a_coordinates, expected_a_coordinates)
        self.assertListOfPointsAlmostEqual(b_coordinates, expected_b_coordinates)


    def test_stopped_balls_intersecting(self):
        ball_a = helpers.create_space_ball(self.park, y=1.9)
        ball_b = helpers.create_space_ball(self.park)

        a_coordinates = []
        b_coordinates = []

        for i in xrange(10):
            self.park.Evolve()
            a_coordinates.append((ball_a.x, ball_a.y, ball_a.z))
            b_coordinates.append((ball_b.x, ball_b.y, ball_b.z))

        expected_a_coordinates = [
            (0.0, 2.5951094880505643, 0.0),
            (0.0, 3.736969417977334, 0.0),
            (0.0, 4.648125579572035, 0.0),
            (0.0, 5.375189844683108, 0.0),
            (0.0, 5.955356525086511, 0.0),
            (0.0, 6.418305116018683, 0.0),
            (0.0, 6.787718605426691, 0.0),
            (0.0, 7.082495020842102, 0.0),
            (0.0, 7.317714192790605, 0.0),
            (0.0, 7.505409191300473, 0.0)
        ]

        expected_b_coordinates = [
            (0.0, -0.6951094880505645, 0.0),
            (0.0, -1.836969417977334, 0.0),
            (0.0, -2.7481255795720347, 0.0),
            (0.0, -3.4751898446831078, 0.0),
            (0.0, -4.055356525086512, 0.0),
            (0.0, -4.5183051160186825, 0.0),
            (0.0, -4.8877186054266915, 0.0),
            (0.0, -5.182495020842103, 0.0),
            (0.0, -5.417714192790607, 0.0),
            (0.0, -5.605409191300473, 0.0)
        ]

        self.assertListOfPointsAlmostEqual(a_coordinates, expected_a_coordinates)
        self.assertListOfPointsAlmostEqual(b_coordinates, expected_b_coordinates)

    def test_goto_collision(self):
        a_coordinates = []
        b_coordinates = []

        ball_a = helpers.create_space_ball(self.park)
        ball_b = helpers.create_space_ball(self.park, x=10, y=10, z=10)

        self.park.GotoPoint(ball_a.id, 10, 10, 10)
        self.park.GotoPoint(ball_b.id, 0, 0, 0)

        for i in xrange(20):
            self.park.Evolve()
            a_coordinates.append((ball_a.x, ball_a.y, ball_a.z))
            b_coordinates.append((ball_b.x, ball_b.y, ball_b.z))

        expected_a_coordinates = [
            (0.22785672701553972, 0.22785672701553972, 0.22785672701553972),
            (0.8863613208951594, 0.8863613208951594, 0.8863613208951594),
            (1.9402353687182086, 1.9402353687182086, 1.9402353687182086),
            (3.3570904438241858, 3.3570904438241858, 3.3570904438241858),
            (3.745389410116848, 3.745389410116848, 3.745389410116848),
            (3.2276392358401793, 3.2276392358401793, 3.2276392358401793),
            (3.2016165950513056, 3.2016165950513056, 3.2016165950513056),
            (3.627039465463305, 3.627039465463305, 3.627039465463305),
            (3.8640182292625984, 3.8640182292625984, 3.8640182292625984),
            (3.9449236847646176, 3.9449236847646176, 3.9449236847646176),
            (4.014836947038442, 4.014836947038442, 4.014836947038442),
            (4.087400730922078, 4.087400730922078, 4.087400730922078),
            (4.152795682608074, 4.152795682608074, 4.152795682608074),
            (4.2242631173879985, 4.2242631173879985, 4.2242631173879985),
            (4.289057014884843, 4.289057014884843, 4.289057014884843),
            (4.360366727534607, 4.360366727534607, 4.360366727534607),
            (4.425074016382081, 4.425074016382081, 4.425074016382081),
            (4.496360773851808, 4.496360773851808, 4.496360773851808),
            (4.561055454563248, 4.561055454563248, 4.561055454563248),
            (4.624809527703686, 4.624809527703686, 4.624809527703686)
        ]

        expected_b_coordinates = [
            (9.772143272984462, 9.772143272984462, 9.772143272984462),
            (9.113638679104842, 9.113638679104842, 9.113638679104842),
            (8.059764631281793, 8.059764631281793, 8.059764631281793),
            (6.642909556175815, 6.642909556175815, 6.642909556175815),
            (6.254610589883153, 6.254610589883153, 6.254610589883153),
            (6.772360764159822, 6.772360764159822, 6.772360764159822),
            (6.7983834049486935, 6.7983834049486935, 6.7983834049486935),
            (6.372960534536694, 6.372960534536694, 6.372960534536694),
            (6.1359817707374, 6.1359817707374, 6.1359817707374),
            (6.055076315235381, 6.055076315235381, 6.055076315235381),
            (5.985163052961557, 5.985163052961557, 5.985163052961557),
            (5.912599269077921, 5.912599269077921, 5.912599269077921),
            (5.847204317391924, 5.847204317391924, 5.847204317391924),
            (5.775736882612, 5.775736882612, 5.775736882612),
            (5.710942985115155, 5.710942985115155, 5.710942985115155),
            (5.639633272465392, 5.639633272465392, 5.639633272465392),
            (5.574925983617919, 5.574925983617919, 5.574925983617919),
            (5.503639226148192, 5.503639226148192, 5.503639226148192),
            (5.438944545436752, 5.438944545436752, 5.438944545436752),
            (5.375190472296313, 5.375190472296313, 5.375190472296313)
        ]

        self.assertListOfPointsAlmostEqual(a_coordinates, expected_a_coordinates)
        self.assertListOfPointsAlmostEqual(b_coordinates, expected_b_coordinates)

    def test_balls_in_warp_should_not_collide(self):
        ball_a = helpers.create_space_ball(self.park, x=-4, y=0, z=0, vx=1000.0, max_velocity=1000.0, mass=1.0)
        ball_b = helpers.create_space_ball(self.park, x=4, y=0, z=0, vx=-1000.0, max_velocity=1000.0, mass=1.0)
        ball_a.radius = 2.0
        ball_b.radius = 2.0

        self.park.WarpTo(ball_a.id, 100000.0, 0, 0)
        self.park.WarpTo(ball_b.id, -100000.0, 0, 0)

        for i in xrange(80):
            self.park.Evolve()

        self.assertGreater(ball_a.x, ball_b.x)
        self.assertEqual(ball_a.y, 0.0)
        self.assertEqual(ball_a.z, 0.0)
        self.assertEqual(ball_b.y, 0.0)
        self.assertEqual(ball_b.z, 0.0)