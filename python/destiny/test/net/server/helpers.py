from collections import defaultdict
import unittest

import blue
import destiny

from destiny.net.server import (
    BallInfoInterface,
    NetworkInterface,
    CharacterInterestsInterface,
    ClientInterestsInterface
)


def add_ball_to_park(
        park,
        ball_id=-1,
        mass=1,
        radius=1.0,
        max_velocity=1.0,
        is_free=True,
        is_global=False,
        is_massive=True,
        is_interactive=True,
        is_space_junk=False,
        x=0,
        y=0,
        z=0,
        vx=0,
        vy=0,
        vz=0,
        agility=1.0,
        speed_fraction=1.0
):
    return park.AddBall(
        ball_id,
        mass,
        radius,
        max_velocity,
        is_free,
        is_global,
        is_massive,
        is_interactive,
        is_space_junk,
        x,
        y,
        z,
        vx,
        vy,
        vz,
        agility,
        speed_fraction,
    )


class TestBallInfo(BallInfoInterface):
    def __init__(self):
        self._characters_for_ball = defaultdict(list)

    def get_character_for_ball(self, ball_id):
        return self._characters_for_ball[ball_id][0]

    def get_characters_for_ball(self, ball_id):
        return self._characters_for_ball[ball_id]

    def set_characters_for_ball(self, ball_id, characters):
        self._characters_for_ball[ball_id] = characters


class TestNetwork(NetworkInterface):
    def __init__(self):
        self.singlecasts = []
        self.narrowcasts = []

    def narrowcast(self, updates):
        self.narrowcasts += updates

    def singlecast(self, updates):
        self.singlecasts += updates

    def clear(self):
        self.singlecasts = []
        self.narrowcasts = []


class TestCharacterInterests(CharacterInterestsInterface):
    def __init__(self):
        self._ball_to_char = defaultdict(list)
        self._char_to_client = {}

    def add_interest(self, ball_id, char_id):
        self._ball_to_char[ball_id].append(char_id)

    def map_to_client(self, char_id, client_id):
        self._char_to_client[char_id] = client_id

    def get_interested_character_ids_for_ball(self, ball_id):
        return self._ball_to_char[ball_id]

    def remove_character_interest(self, ball_id, character_id):
        self._ball_to_char[ball_id].remove(character_id)

    def get_client_id_for_character(self, character_id):
        return self._char_to_client.get(character_id, None)


class TestClientInterests(ClientInterestsInterface):
    def __init__(self):
        self._ball_id_to_client_ids = defaultdict(list)

    def add_client_interest(self, ball_id, client_id):
        self._ball_id_to_client_ids[ball_id].append(client_id)

    def get_all_interested_ball_ids(self):
        self._ball_id_to_client_ids.keys()

    def get_interested_client_ids_for_ball(self, ball_id):
        return self._ball_id_to_client_ids[ball_id]

    def remove_client_interest(self, ball_id, client_id):
        self._ball_id_to_client_ids.remove(ball_id)

    def has_client_interest(self, ball_id):
        return bool(self._ball_id_to_client_ids[ball_id])



class DestinyTestCase(unittest.TestCase):
    def assertBallsEqual(self, first, second, ignore_bubble_id=False):
        self.assertEqual(first.id, second.id)
        self.assertEqual(first.mass, second.mass)
        self.assertEqual(first.radius, second.radius)
        self.assertEqual(first.maxVelocity, second.maxVelocity)
        self.assertEqual(first.isFree, second.isFree)
        self.assertEqual(first.isGlobal, second.isGlobal)
        self.assertEqual(first.isMassive, second.isMassive)
        self.assertEqual(first.isInteractive, second.isInteractive)
        self.assertEqual(first.isCloaked, second.isCloaked)
        self.assertEqual(first.isMoribund, second.isMoribund)
        self.assertEqual(first.harmonic, second.harmonic)
        self.assertEqual(first.corporationID, second.corporationID)
        self.assertEqual(first.allianceID, second.allianceID)
        self.assertEqual(first.x, second.x)
        self.assertEqual(first.y, second.y)
        self.assertEqual(first.z, second.z)
        self.assertEqual(first.vx, second.vx)
        self.assertEqual(first.vy, second.vy)
        self.assertEqual(first.vz, second.vz)
        self.assertEqual(first.yaw, second.yaw)
        self.assertEqual(first.pitch, second.pitch)
        self.assertEqual(first.roll, second.roll)
        self.assertEqual(first.Agility, second.Agility)
        self.assertEqual(first.speedFraction, second.speedFraction)
        self.assertEqual(first.mode, second.mode)
        self.assertEqual(first.gotoX, second.gotoX)
        self.assertEqual(first.gotoY, second.gotoY)
        self.assertEqual(first.gotoZ, second.gotoZ)
        self.assertEqual(first.followId, second.followId)
        self.assertEqual(first.followRange, second.followRange)
        self.assertEqual(first.ownerId, second.ownerId)
        self.assertEqual(first.effectStamp, second.effectStamp)
        if not ignore_bubble_id:
            self.assertEqual(first.newBubbleId, second.newBubbleId)
            self.assertEqual(first.oldBubbleId, second.oldBubbleId)
        self.assertEqual(first.formationID, second.formationID)
        self.assertListEqual(list(first.miniBalls), list(second.miniBalls))
        self.assertListEqual(list(first.miniCapsules), list(second.miniCapsules))
        self.assertListEqual(list(first.miniBoxes), list(second.miniBoxes))

    def ballsAreEqual(self, first, second, ignore_bubble_id=False):
        if not ignore_bubble_id and (
                first.newBubbleId != second.newBubbleId or
                first.oldBubbleId != second.oldBubbleId):
            return False

        return (
            first.id == second.id and
            first.mass == second.mass and
            first.radius == second.radius and
            first.maxVelocity == second.radius and
            first.isFree == second.isFree and
            first.isGlobal == second.isGlobal and
            first.isMassive == second.isMassive and
            first.isInteractive == second.isInteractive and
            first.isCloaked == second.isCloaked and
            first.isMoribund == second.isMoribund and
            first.harmonic == second.harmonic and
            first.corporationID == second.corporationID and
            first.allianceID == second.allianceID and
            first.x == second.x and
            first.y == second.y and
            first.z == second.z and
            first.vx == second.vx and
            first.vy == second.vy and
            first.vz == second.vz and
            first.yaw == second.yaw and
            first.pitch == second.pitch and
            first.roll == second.roll and
            first.Agility == second.Agility and
            first.speedFraction == second.mass and
            first.speedFraction == second.speedFraction and
            first.mode == second.mode and
            first.gotoX == second.gotoX and
            first.gotoY == second.gotoY and
            first.gotoZ == second.gotoZ and
            first.followId == second.followId and
            first.followRange == second.followRange and
            first.ownerId == second.ownerId and
            first.effectStamp == second.effectStamp and
            first.formationID == second.formationID and
            list(first.miniBalls) == list(second.miniBalls) and
            list(first.miniCapsules) == list(second.miniCapsules) and
            list(first.miniBoxes) == list(second.miniBoxes)
        )

    def read_balls_from_stream_into_park(self, ball_stream):
        stream = blue.MemStream()
        stream.Write(ball_stream)
        park = destiny.Ballpark()
        park.ReadFullStateFromStream(stream)
        return park
