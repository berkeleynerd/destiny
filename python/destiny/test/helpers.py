import struct
import unittest
import destiny

MAX_FORMATION_SLOTS_FOR_BALLS = 16
TEN_BILLION = 10000000000.0
AU = .1495978707e12
MIN_BOX_SIZE = 480.0

FORMATIONS = (

    ( 'Diamond',
      (
          ( 100.0,   0.0, 0.0),
          (   0.0, 100.0, 0.0),
          (-100.0,   0.0, 0.0),
          (0.0,   -100.0, 0.0)
      )
    ),

    ( 'Arrow',
      (
          ( 100.0,   0.0, -50.0),
          (  50.0,   0.0,   0.0),
          (-100.0,   0.0, -50.0),
          ( -50.0,   0.0,   0.0)
      )
    ),
    ( 'FormationOfOne',
      (
          ( 0.0,   0.0, 0.0),
      )
    ),

)

class BallparkTestCase(unittest.TestCase):
    def setUp(self):
        self.park = destiny.Ballpark()

    def tearDown(self):
        self.park.ClearAll()
        del self.park

    def add_balls(self, n):
        for i in xrange(n):
            yield add_ball_to_park(self.park, objectID=i+1)

    def assertMiniBallEqual(self, first, second):
        self.assertEqual(first.id, second.id)
        self.assertEqual(first.x, second.x)
        self.assertEqual(first.y, second.y)
        self.assertEqual(first.z, second.z)

    def assertMiniCapsuleEqual(self, first, second):
        self.assertEqual(first.id, second.id)
        self.assertEqual(first.ax, second.ax)
        self.assertEqual(first.ay, second.ay)
        self.assertEqual(first.az, second.az)
        self.assertEqual(first.bx, second.bx)
        self.assertEqual(first.by, second.by)
        self.assertEqual(first.bz, second.bz)

    def assertMiniBoxEqual(self, first, second):
        self.assertEqual(first.id, second.id)
        self.assertEqual(first.c0, second.c0)
        self.assertEqual(first.c1, second.c1)
        self.assertEqual(first.c2, second.c2)
        self.assertEqual(first.x0, second.x0)
        self.assertEqual(first.x1, second.x1)
        self.assertEqual(first.x2, second.x2)
        self.assertEqual(first.y0, second.y0)
        self.assertEqual(first.y1, second.y1)
        self.assertEqual(first.y2, second.y2)
        self.assertEqual(first.z0, second.z0)
        self.assertEqual(first.z1, second.z1)
        self.assertEqual(first.z2, second.z2)


    def assertBallEqual(self, first, second):
        self.assertEqual(first.Agility, second.Agility)
        self.assertEqual(first.allianceID, second.allianceID)
        self.assertEqual(first.centerDist, second.centerDist)
        self.assertEqual(first.corporationID, second.corporationID)
        self.assertEqual(first.effectStamp, second.effectStamp)
        self.assertEqual(first.followId, second.followId)
        self.assertEqual(first.followRange, second.followRange)
        self.assertEqual(first.formationID, second.formationID)
        self.assertEqual(first.gotoX, second.gotoX)
        self.assertEqual(first.gotoY, second.gotoY)
        self.assertEqual(first.gotoZ, second.gotoZ)
        self.assertEqual(first.harmonic, second.harmonic)
        self.assertEqual(first.id, second.id)
        self.assertEqual(first.isCloaked, second.isCloaked)
        self.assertEqual(first.isFree, second.isFree)
        self.assertEqual(first.isGlobal, second.isGlobal)
        self.assertEqual(first.isInteractive, second.isInteractive)
        self.assertEqual(first.isMassive, second.isMassive)
        self.assertEqual(first.isMoribund, second.isMoribund)
        self.assertEqual(first.mass, second.mass)
        self.assertEqual(first.maxVelocity, second.maxVelocity)
        self.assertEqual(len(first.miniBalls), len(second.miniBalls))
        self.assertEqual(first.mode, second.mode)
        self.assertEqual(first.newBubbleId, second.newBubbleId)
        self.assertEqual(first.oldBubbleId, second.oldBubbleId)
        self.assertEqual(first.ownerId, second.ownerId)
        self.assertEqual(first.pitch, second.pitch)
        self.assertEqual(first.radius, second.radius)
        self.assertEqual(first.roll, second.roll)
        self.assertEqual(first.showBoxes, second.showBoxes)
        self.assertEqual(first.speedFraction, second.speedFraction)
        self.assertEqual(first.surfaceDist, second.surfaceDist)
        self.assertEqual(first.vx, second.vx)
        self.assertEqual(first.vy, second.vy)
        self.assertEqual(first.vz, second.vz)
        self.assertEqual(first.x, second.x)
        self.assertEqual(first.y, second.y)
        self.assertEqual(first.yaw, second.yaw)
        self.assertEqual(first.z, second.z)

    def assertListsAlmostEqual(self, list_one, list_two):
        self.assertEqual(len(list_one), len(list_two))
        for i, v in enumerate(list_one):
            self.assertAlmostEqual(v, list_two[i])

    def assertListOfPointsAlmostEqual(self, first, second, places=4):
        len_first = len(first)
        len_second = len(second)
        limit = min(len_first, len_second)

        for i, first_point in enumerate(first):
            if i == limit:
                break
            second_point = second[i]
            if first_point != second_point:
                for j in xrange(3):
                    if round(first_point[j], places) != round(second_point[j], places):
                        msg = "Points differ. First differing element: %s. %s != %s up to %s decimal places" % (
                            i, first_point, second_point, places)
                        self.fail(msg)

        if len_first > len_second:
            msg = "First list contains %s additional elements" % (len_first - len_second)
            self.fail(msg)
        elif len_first < len_second:
            msg = "Second list contains %s additional elements" % (len_second - len_first)
            self.fail(msg)

    def evolve_ball_and_get_coordinates(self, ball, step_count):
        coordinates = []
        for i in xrange(step_count):
            self.park.Evolve()
            coordinates.append((ball.x, ball.y, ball.z))
        return coordinates

class BallPOPO(object):
    objectID = 0
    mass = 0.0
    radius = 0.0
    maxVel = 0.0
    isFree = False
    isGlobal = False
    isMassive = False
    isInteractive = False
    isSpaceJunk = False
    x = TEN_BILLION
    y = TEN_BILLION
    z = TEN_BILLION
    vx = 0.0
    vy = 0.0
    vz = 0.0
    agility = 0.0
    speedFraction = 0.0


def add_ball_to_park(park, **kwargs):
    ball = BallPOPO()
    for i in kwargs:
        ball.__setattr__(i, kwargs[i])
    return park.AddBall(
        ball.objectID,
        ball.mass,
        ball.radius,
        ball.maxVel,
        ball.isFree,
        ball.isGlobal,
        ball.isMassive,
        ball.isInteractive,
        ball.isSpaceJunk,
        ball.x,
        ball.y,
        ball.z,
        ball.vx,
        ball.vy,
        ball.vz,
        ball.agility,
        ball.speedFraction,
    )

def add_ball_event_spy_to_park(park, **kwargs):
    return BallEventSpy(add_ball_to_park(park, **kwargs))


def create_space_ball(park, x=0, y=0, z=0, vx=0, vy=0, vz=0, mass=None, max_velocity=None):
    """
    This type of ball has some non-zero values set for parameters that affect movement.
    """
    ball = add_ball_to_park(
        park,
        objectID=create_space_ball.next_object_id,
        x=x,
        y=y,
        z=z,
        vx=vx,
        vy=vy,
        vz=vz
    )
    create_space_ball.next_object_id += 1

    park.SetBallFree(ball.id, True)
    ball.maxVelocity = max_velocity if max_velocity is not None else 10.0
    ball.Agility = 0.9
    ball.mass = mass if mass is not None else 13000000.0
    ball.radius = 2.0
    ball.isMassive = True
    ball.speedFraction = 0.95

    return ball
create_space_ball.next_object_id = 1


def reinterpret_double_as_int(d):
    p = struct.pack('!d', d)
    return struct.unpack('!q', p)[0]


def get_level_width(level_no):
    no_levels = 8
    base_width = MIN_BOX_SIZE
    if level_no >= no_levels:
        return None
    return base_width * 4 ** (no_levels - level_no - 1)


class BallEventSpy(destiny.Ball):
    def __init__(self, *args):
        super(BallEventSpy, self).__init__(*args)
        self.collision_callback_args = []
        self.proximity_callback_args = []
        self.range_callback_args = []
        self.target_tracking_callback_args = []

    def DoProximity(self, intruderID, entered):
        self.proximity_callback_args.append((intruderID, entered))

    def DoRange(self, in_range):
        self.range_callback_args.append(in_range)

    def DoTargetTracking(self, entered):
        self.target_tracking_callback_args.append(entered)

    def DoCollision(self, id, x, y, z):
        self.collision_callback_args.append((id, x, y, z))