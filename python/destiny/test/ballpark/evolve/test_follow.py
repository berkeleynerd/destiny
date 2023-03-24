from destiny.test import helpers


class TestFormation(helpers.BallparkTestCase):
    def setUp(self):
        super(TestFormation, self).setUp()
        self.park.LoadFormations(helpers.FORMATIONS)
        self.maxDiff = None

    def tearDown(self):
        self.park.ClearAll()
        del self.park


    def test_formation_follow_stopped_ball(self):
        follower = helpers.create_space_ball(self.park)
        leader = helpers.create_space_ball(self.park, x=-5.0)
        self.park.SetBallFormation(leader.id, 1)
        slot = leader.ReserveFormationSlot()
        self.park.FormationFollow(follower.id, leader.id, slot)

        follower_coordinates = []

        for i in xrange(10):
            self.park.Evolve()
            follower_coordinates.append((follower.x, follower.y, follower.z))

        expected_follower_coordinates = [
            (0.34937874956616805, 0.0, -0.18354986649486965),
            (1.3590812701221973, 0.0, -0.7140078954325842),
            (2.9750142375238595, 0.0, -1.5629555798568222),
            (5.1475156200406165, 0.0, -2.7043024397217237),
            (7.8309916695598885, 0.0, -4.1140953113347125),
            (10.983583650252394, 0.0, -5.77034325969288),
            (14.56686186862378, 0.0, -7.65285683389491),
            (18.54554476843945, 0.0, -9.74310049065252),
            (22.887241037223315, 0.0, -12.024057107178583),
            (27.56221283923634, 0.0, -14.480103593097612)
        ]

        self.assertListOfPointsAlmostEqual(follower_coordinates, expected_follower_coordinates)

    def test_formation_follow_moving_ball(self):
        follower = helpers.create_space_ball(self.park)
        leader = helpers.create_space_ball(self.park, x=-5.0)
        self.park.SetBallFormation(leader.id, 1)
        slot = leader.ReserveFormationSlot()
        self.park.FormationFollow(follower.id, leader.id, slot)

        self.park.GotoPoint(leader.id, 0.0, 100.0, 0.0)

        follower_coordinates = []

        for i in xrange(10):
            self.park.Evolve()
            follower_coordinates.append((follower.x, follower.y, follower.z))

        expected_src_coordinates = [
            (0.34937874956616805, 0.0, -0.18354986649486965),
            (1.2522700311290196, -0.2984878365701446, -0.442015514428003),
            (2.6428549859837505, -1.083812623793305, -0.568440079311451),
            (4.525369455660191, -2.2133606150700387, -0.8465910315244277),
            (6.698838038625631, -3.6099869836316993, -1.5982896364292383),
            (8.916087445091419, -5.209637520522771, -2.9541942809113397),
            (11.011849619430125, -6.955575729117526, -4.920658718422647),
            (12.897996631839323, -8.79576410566157, -7.465035973264715),
            (14.532033823223955, -10.680120838194423, -10.547372880541618),
            (15.895577914276311, -12.55839407965528, -14.128291692594965)
        ]

        self.assertListOfPointsAlmostEqual(follower_coordinates, expected_src_coordinates)


class TestFollow(helpers.BallparkTestCase):
    def test_follow_stopped_ball(self):
        self.maxDiff = None
        follower = helpers.create_space_ball(self.park)
        leader = helpers.create_space_ball(self.park, x=100, y=200, z=300)
        self.park.FollowBall(follower.id, leader.id)

        follower_coordinates = []

        for i in xrange(10):
            self.park.Evolve()
            follower_coordinates.append((follower.x, follower.y, follower.z))

        expected_follower_coordinates = [
            (0.10547716886968704, 0.21095433773937408, 0.3164315066090625),
            (0.41030556327284373, 0.8206111265456875, 1.2309166898185306),
            (0.8981544513244621, 1.7963089026489243, 2.694463353973384),
            (1.5540309048233925, 3.108061809646785, 4.662092714470175),
            (2.364170207183283, 4.728340414366566, 7.092510621549848),
            (3.3159352390795633, 6.631870478159127, 9.94780571723868),
            (4.397724106363411, 8.795448212726821, 13.193172319090223),
            (5.59888533504116, 11.19777067008232, 16.796656005123477),
            (6.909640013429743, 13.819280026859486, 20.72892004028922),
            (8.321010312379672, 16.642020624759343, 24.963030937139006)
        ]

        self.assertListOfPointsAlmostEqual(follower_coordinates, expected_follower_coordinates)

    def test_follow_moving_ball(self):
        self.maxDiff = None
        follower = helpers.create_space_ball(self.park)
        leader = helpers.create_space_ball(self.park, x=100, y=0, z=0)
        self.park.FollowBall(follower.id, leader.id)
        self.park.GotoPoint(leader.id, 0, 100, 200)

        follower_coordinates = []

        for i in xrange(10):
            self.park.Evolve()
            follower_coordinates.append((follower.x, follower.y, follower.z))

        expected_follower_coordinates = [
            (0.3946594280372685, 0.0, 0.0),
            (1.5352202516956626, 0.0006394210607791958, 0.0012788421215583917),
            (3.360538351919784, 0.00437328037960026, 0.00874656075920052),
            (5.814319316187089, 0.01591817574944254, 0.03183635149888508),
            (8.844475669456193, 0.042139697272792834, 0.08427939454558567),
            (12.402376916827746, 0.09207177010911027, 0.18414354021822055),
            (16.441932909387862, 0.17705502464375017, 0.35411004928750034),
            (20.918410162782422, 0.31098291710272036, 0.6219658342054407),
            (25.786827618186656, 0.5106319284615736, 1.0212638569231471),
            (30.99971869544917, 0.796013087027414, 1.592026174054828)
        ]

        self.assertListOfPointsAlmostEqual(follower_coordinates, expected_follower_coordinates)
