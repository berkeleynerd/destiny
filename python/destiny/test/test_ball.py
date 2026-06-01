# Copyright © 2015 CCP ehf.

import destiny

from destiny.test import helpers



class BallTest(helpers.BallparkTestCase):
    def test_can_construct(self):
        b = destiny.Ball()
        self.assertIsNotNone(b)

    def test_can_add_miniball(self):
        b = helpers.add_ball_to_park(self.park)
        x, y, z = (1.0,2.0,3.0)
        radius = 1.0
        b.AddMiniBall(x, y, z, radius)
        self.assertEqual(len(b.miniBalls), 1)

    def test_get_rotated_vector_returns_original_vector_if_there_is_no_rotation(self):
        b = destiny.Ball()
        original_vector = [1.0,2.0,3.0]
        rotated_vector = b.GetRotatedVector(original_vector)
        self.assertListsAlmostEqual(original_vector, rotated_vector)

    def test_add_proximity_sensor(self):
        b = helpers.add_ball_to_park(self.park)
        sensor_range = 5.0
        period = 10.0
        shuffle = 1
        onlyInteractives = 0
        b.AddProximitySensor(sensor_range, period, shuffle, onlyInteractives)

    def test_reserve_formation_slot_returns_negative_one_when_ball_is_not_set_up_properly(self):
        b = destiny.Ball()
        slot = b.ReserveFormationSlot()
        self.assertEqual(slot, -1)

    def test_reserve_formation_slot(self):
        self.park.LoadFormations(helpers.FORMATIONS)
        b = helpers.add_ball_to_park(self.park)
        self.park.SetBallFormation(b.id, 1)
        slot = b.ReserveFormationSlot()
        self.assertEqual(slot, 0)

    def test_slots_are_reserved_in_incremental_order(self):
        self.park.LoadFormations(helpers.FORMATIONS)
        b = helpers.add_ball_to_park(self.park)
        self.park.SetBallFormation(b.id, 1)
        for i in range(len(helpers.FORMATIONS[b.formationID][1])):
            self.assertEqual(b.ReserveFormationSlot(), i)

    def test_reserving_too_many_formation_slots_fails(self):
        self.park.LoadFormations(helpers.FORMATIONS)
        b = helpers.add_ball_to_park(self.park)
        self.park.SetBallFormation(b.id, 1)
        for i in range(len(helpers.FORMATIONS[b.formationID][1])):
            b.ReserveFormationSlot()
        slot = b.ReserveFormationSlot()
        self.assertEqual(slot, -1)

    def test_free_formation_slot(self):
        self.park.LoadFormations(helpers.FORMATIONS)
        b = helpers.add_ball_to_park(self.park)
        self.park.SetBallFormation(b.id, 1)
        for i in range(helpers.MAX_FORMATION_SLOTS_FOR_BALLS):
            b.ReserveFormationSlot()
        b.FreeFormationSlot(2)
        slot = b.ReserveFormationSlot()
        self.assertEqual(slot, 2)

    def test_default_values(self):
        b = destiny.Ball()
        self.assertEqual(b.id, 0)
        self.assertEqual(b.mass, 0)
        self.assertEqual(b.radius, 0)
        self.assertEqual(b.maxVelocity, 0)
        self.assertFalse(b.isFree)
        self.assertFalse(b.isGlobal)
        self.assertFalse(b.isMassive)
        self.assertFalse(b.isInteractive)
        self.assertFalse(b.isCloaked)
        self.assertFalse(b.isMoribund)
        self.assertFalse(b.isMassive)
        self.assertEqual(b.harmonic, -1)
        self.assertEqual(b.corporationID, -1)
        self.assertEqual(b.allianceID, -1)
        self.assertEqual(b.x, helpers.TEN_BILLION)
        self.assertEqual(b.y, helpers.TEN_BILLION)
        self.assertEqual(b.z, helpers.TEN_BILLION)
        self.assertEqual(b.vx, 0)
        self.assertEqual(b.vy, 0)
        self.assertEqual(b.vz, 0)
        self.assertEqual(b.yaw, 0)
        self.assertEqual(b.pitch, 0)
        self.assertEqual(b.roll, 0)
        self.assertEqual(b.Agility, 1)
        self.assertEqual(b.speedFraction, 1)
        self.assertEqual(b.mode, destiny.DSTBALL_STOP)
        self.assertEqual(b.gotoX, 0)
        self.assertEqual(b.gotoY, 0)
        self.assertEqual(b.gotoZ, 0)
        self.assertEqual(b.followId, 0)
        self.assertEqual(b.followRange, 10)
        self.assertEqual(b.ownerId, 0)
        self.assertEqual(b.effectStamp, 0)
        self.assertEqual(b.newBubbleId, -1)
        self.assertEqual(b.oldBubbleId, -1)
        self.assertEqual(b.formationID, 255)
        self.assertEqual(len(b.miniBalls), 0)
        self.assertIsNone(b.ballpark)

