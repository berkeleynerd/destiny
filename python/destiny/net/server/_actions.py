import logging

import blue

from destiny._util.settings import is_dynamical_orientation_enabled
from destiny._util.signal import Signal
from destiny._util.timing import TimedFunction

logger = logging.getLogger(__name__)


class Actions(object):
    """
    Exposes a simple API for destiny actions.
    Actions get added to the system history to
    be applied on the next tick.
    """
    def __init__(self, park):
        """
        :param park: The ballpark in which the actions get performed
        :type park: destiny.Ballpark
        """
        self._park = park
        self._system_history = []
        self.on_ball_made_global = Signal()
        self.on_add_to_system_history = Signal()
        self._stamp_for_system = self._get_initial_stamp_for_system()

    def _get_initial_stamp_for_system(self):
        if self._park.currentTime == 0:
            return 0
        return self._park._currentTime + 1

    def _add_to_system_history(self, ball_id, eventName, *args, **kwargs):
        """
        Add the new event to the system history.
        :param ball_id: The ball performing the action.
        :type ball_id: int
        :param eventName: the name of the action that happens.
        :type event: str
        :param args: arguments for the action.
        :type args: any
        :param insert_ball_id_into_args: When True, the ball_id gets inserted as the first argument.
                                         This is a keyword argument, which defaults to True.
        :type insert_ball_id_into_args: bool
        :param local_args: Some events have local arguments to the ticker that should not
                           get sent out as part of the event. (CloakBall uses this keyword argument
                          to filter out which bubble to send the event to).
        :type local_args: any
        """
        insert_ball_id_into_args = kwargs.get("insert_ball_id_into_args", True)
        if insert_ball_id_into_args:
            args = (ball_id,) + args
        local_args = kwargs.get("local_args", None)
        if local_args is None:
            event = (eventName, args)
        else:
            event = (eventName, args, local_args)
        action = ball_id, self._stamp_for_system, event 
        if not self._system_history or self._system_history[-1] != action: # No use adding the same action twice in a row.
            self._system_history.append(action)
        self.on_add_to_system_history(eventName)

    def set_stamp_for_system(self, stamp):
        """
        :param stamp: the timestamp at which new actions get added
        :type stamp: int
        """
        self._stamp_for_system = stamp

    def flush_history(self):
        """
        :return: the history accumulated since the last flush.
        :rtype: list
        """
        history, self._system_history = self._system_history, []
        return history

    def go_to_point(self, src_id, x, y, z):
        """
        Make ball travel to the specified point.
        :param src_id: The ball that should travel to the point.
        :type src_id int
        :param x: The x coordinate of the point.
        :type x: float
        :param y: The y coordinate of the point.
        :type y: float
        :param z: The z coordinate of the point.
        :type z: float
        """
        self._add_to_system_history(
            src_id,
            'GotoPoint',
            x, y, z
        )

    def go_to_direction(self, src_id, x, y, z):
        """
        Make ball travel in the specified direction.
        :param src_id: The ball that should travel in that direction.
        :type src_id int
        :param x: The x coordinate of the direction vector.
        :type x: int
        :param y: The y coordinate of the direction vector.
        :type y: int
        :param z: The z coordinate of the direction vector.
        :type z: int
        """
        self._add_to_system_history(
            src_id,
            "GotoDirection",
            x, y, z
        )

    def orbit(self, src_id, dst_id, orbit_range_meters):
        """
        Make src_id ball orbit dst_id ball at orbit_range_meters range.
        :param src_id: The ball that should orbit dst_id
        :type src_id: int
        :param dst_id: The ball orbited by src_id
        :type dst_id: int
        :param orbit_range_meters: The range at which src_id should orbit dst_id.
        :type orbit_range_meters: float
        """
        self._add_to_system_history(
            src_id,
            "Orbit",
            dst_id, orbit_range_meters
        )

    def set_ball_troll(self, src_id, delay_ticks):
        """
        Freeze the ball after a delay, like what happens
        to a troll once the sun comes up.

        :param src_id: The ball to freeze
        :type src_id: int
        :param delay_ticks: The number of ticks to wait before
                            freezing the ball
        :type delay_ticks: int
        """
        self._add_to_system_history(
            src_id,
            'SetBallTroll',
            delay_ticks,
        )

    def set_ball_velocity(self, src_id, vx, vy, vz):
        """
        Set the direction and velocity of the ball.

        :param src_id: The ball whose velocity and direction we are changing.
        :type src_id: int
        :param vx: The x part of the velocity vector.
        :type vx: float
        :param vy: The y part of the velocity vector.
        :type vy: float
        :param vz: The z part of the velocity vector.
        :type vz: float
        """
        self._add_to_system_history(
            src_id,
            'SetBallVelocity',
            vx, vy, vz
        )

    def set_ball_angular_velocity(self, src_id, wx, wy, wz):
        """
        Set the angular velocity of the ball. (rad/s)

        :param src_id: The ball whose angular velocity we are changing.
        :type src_id: int
        :param wx: The x part of the angular velocity vector.
        :type wx: float
        :param wy: The y part of the angular velocity vector.
        :type wy: float
        :param wz: The z part of the angular velocity vector.
        :type wz: float
        """
        if not is_dynamical_orientation_enabled():
            raise RuntimeError("Attempted to set ball angular velocity, but Dynamical Orientation is disabled.")
        self._add_to_system_history(
            src_id,
            'SetBallAngularVelocity',
            wx, wy, wz
        )

    def set_max_angular_velocity(self, src_id, wx, wy, wz):
        """
        Set the maximum angular velocity of the ball. (rad/s)

        :param src_id: The ball whose maximum angular velocity we are changing.
        :type src_id: int
        :param wx: The x part of the max-angular-velocity vector.
        :type wx: float
        :param wy: The y part of the max-angular-velocity vector.
        :type wy: float
        :param wz: The z part of the max-angular-velocity vector.
        :type wz: float
        """
        if not is_dynamical_orientation_enabled():
            raise RuntimeError("Attempted to set max angular velocity, but Dynamical Orientation is disabled.")
        self._add_to_system_history(
            src_id,
            'SetMaxAngularVelocity',
            wx, wy, wz
        )

    def set_ball_rotation(self, src_id, rx, ry, rz, rw):
        """
        Set the rotation quaternion of the ball. (rad/s)

        :param src_id: The ball whose rotation we are changing.
        :type src_id: int
        :param rx: The x part of the rotation quaternion.
        :type rx: float
        :param ry: The y part of the rotation quaternion.
        :type ry: float
        :param rz: The z part of the rotation quaternion.
        :type rz: float
        :param rw: The w part of the rotation quaternion.
        :type rw: float
        """
        if not is_dynamical_orientation_enabled():
            raise RuntimeError("Attempted to set ball rotation, but Dynamical Orientation is disabled.")
        self._add_to_system_history(
            src_id,
            'SetBallRotation',
            rx, ry, rz, rw
        )

    def set_ball_massive(self, src_id, is_massive):
        """
        Set the ball as massive or not massive.
        Non-massive balls do not collide with anything.

        :param src_id: The ball whose massiveness we want to change.
        :type src_id: int
        :param is_massive: True if ball should be massive.
        :type is_massive: bool
        """
        self._add_to_system_history(
            src_id,
            'SetBallMassive',
            is_massive
        )

    def stop(self, src_id):
        """
        Stops the ball.
        :param src_id: The ball to stop
        :type src_id: int
        """
        self._add_to_system_history(
            src_id,
            'Stop'
        )

    def set_ball_mass(self, src_id, mass_kg):
        """
        Set the mass of the ball in kilograms.

        :param src_id: The ball whose mass we want to change
        :type src_id: int
        :param mass_kg: The new mass for the ball.
        :type mass_kg: float
        """
        self._add_to_system_history(
            src_id,
            'SetBallMass',
            mass_kg,
        )

    def set_ball_agility(self, src_id, agility):
        """
        Set the balls agility, which influences its maneuverability.
        Agility is multiplied with mass when updating the position of a ship.
        Increasing this value makes the ship turn faster when aligning to a point.

        :param src_id: The ball whose agility we want to change.
        :type src_id: int
        :param agility: The agility modifier. Must be positive.
        :type agility: float
        """
        self._add_to_system_history(
            src_id,
            'SetBallAgility',
            agility
        )

    def set_ball_angular_agility(self, src_id, angular_agility):
        """
        Set the balls angular agility, which influences its rotational
        maneuverability. Smaller numbers give more agility.

        :param src_id: The ball whose angular agility we want to change.
        :type src_id: int
        :param angular_agility: The rotational maneuverability of the ball.
        :type angular_agility: float
        """
        if not is_dynamical_orientation_enabled():
            raise RuntimeError("Attempting to set angular agility, but Dynamical Orientation is disabled.")
        self._add_to_system_history(
            src_id,
            'SetBallAngularAgility',
            angular_agility
        )

    def add_ball_to_client_parks(self, src_id):
        """
        Make sure everyone is made aware of the presence
        of a global ball.
        :param src_id: The global ball
        :type src_id: int
        """
        dump = blue.MemStream()
        self._park.WriteBallsToStream([src_id], dump)
        ball_stream = dump.Read()
        self._add_to_system_history(
            src_id,
            'AddBallsToPark',
            ball_stream,
            insert_ball_id_into_args=False
        )

    def make_ball_local(self, src_id):
        """
        This wraps around the destiny function SetBallGlobal(ballID, False)
        to force the ball to be removed from all but the local bubble
        Should only be used to change the global status of balls after they are
        added to park.

        :param src_id: The ball which is no longer global.
        :type src_id: int
        """
        self._park.SetBallGlobal(src_id, False)
        if src_id not in self._park.balls:
            logger.error("Attempted to make ball local, but it's not in the park %s", src_id)
            return
        ball = self._park.balls[src_id]
        self._add_to_system_history(
            src_id,
            'BallNotGlobal',
            ball.newBubbleId,
            insert_ball_id_into_args=False
        )

    def make_ball_global(self, src_id):
        """
        This wraps around the destiny function SetBallGlobal(ballID, True)
        to force the ball to be added to all interactive bubbles.
        Should only be used to change the global status of balls after they are
        added to park.

        :param src_id: The ball to make global
        :type src_id: int.
        """
        self._park.SetBallGlobal(src_id, True)
        self.add_ball_to_client_parks(src_id)
        self.on_ball_made_global(src_id)

    def remove_ball(self, src_id):
        """
        Remove a ball from the ballpark. If the ball is global
        we let everyone know it's gone.

        IMPORTANT: Ball removal is immediate. We do NOT wait for
                   RemoveGlobalBall in the system history to be performed
                   during the next tick, which results in a different
                   result than one might expect if one assumes that
                   everything will execute in order of methods called.

        :param src_id: The ball to remove:
        :type src_id: int
        """
        ball = self._park.balls[src_id]
        if ball.isGlobal:
            # Need to notify all interactive bubbles of this removal
            self._notify_remove_global_ball(src_id)
        self._park._parent_RemoveBall(src_id)

    def _notify_remove_global_ball(self, src_id):
        self._add_to_system_history(
            src_id,
            'RemoveGlobalBall'
        )

    def set_speed_fraction(self, src_id, speed_fraction):
        """
        Set the speed as a fraction of max velocity

        :param src_id: The ball for which we want to set the speed fraction.
        :type src_id: int
        :param speed_fraction: Between 0.0 and 1.0.
        :type speed_fraction: float
        """
        self._add_to_system_history(
            src_id,
            "SetSpeedFraction",
            speed_fraction
        )

    def warp_to(self, src_id, x, y, z, minimum_range, warp_speed):
        """
        Warp to within a certain distance from a certain point.

        :param src_id: The ball to warp.
        :type src_id: int
        :param x: The x coordinate of the warp-to point.
        :type x: float
        :param y: The y coordinate of the warp-to point.
        :type y: float
        :param z: The z coordinate of the warp-to point.
        :type z: float
        :param minimum_range: The distance from the specified point to which we want to warp.
        :type minimum_range: float
        :param warp_speed: The maximum speed attainable during warp.
                           The unit for warp_speed is mAU/s (1/1000th of an AU per second).
        :type warp_speed: int
        """
        self._add_to_system_history(
            src_id,
            "WarpTo",
            x, y, z, minimum_range, warp_speed
        )

    def entity_warp_in(self, src_id, x, y, z, warp_speed):
        """
        Warps an entity into a position without having to enter warp first.

        :param src_id: The ball to warp.
        :type src_id: int
        :param x: The x coordinate of the warp-to point.
        :type x: float
        :param y: The y coordinate of the warp-to point.
        :type y: float
        :param z: The z coordinate of the warp-to point.
        :type z: float
        :param warp_speed: The maximum speed attainable during warp.
                           The unit for warp_speed is mAU/s (1/1000th of an AU per second).
        :type warp_speed: int
        """
        # Warp speed unit is milli-AU per second (mAU/s), i.e. 1/1000th of an AU per second.
        self._add_to_system_history(
            src_id,
            'EntityWarpIn',
            x, y, z, warp_speed
        )

    def set_ball_position(self, src_id, x, y, z, is_local_ball=False):
        """
        Instantly moves a ball to the desired position.

        :param src_id: The ball to teleport.
        :type src_id: int
        :param x: The x coordinate of the point.
        :type x: float
        :param y: The y coordinate of the point.
        :type y: float
        :param z: The z coordinate of the point.
        :type z: float
        :param is_local_ball: True if the ball only lives server-side,
                              which means noone gets notified when it moves.
        :type is_local_ball: bool
        """
        self._add_to_system_history(
            src_id,
            'SetBallPosition',
            x, y, z,
            local_args=is_local_ball
        )

    def set_ball_harmonic(self, src_id, harmonic_value, corporation_id, alliance_id, is_forcefield):
        """
        Sets the harmonic, corporationId and allianceId attributes on the ball,
        and if the field argument is true, stops the ball and sets the destiny mode to DSTBALL_FIELD.
        If the ball was already in this destiny mode and the field flag is false,
        it just stops the ball, resulting in mode DSTBALL_STOP.
        A ball can enter a forcefield if any of the following attributes match:
            - harmonic
            - corporationID
            - allianceID
        A ball with its harmonic attribute set to -2 can enter any forcefield.

        :param src_id: the ball whose attributes we want to change.
        :type src_id: int
        :param harmonic_value: The harmonic value to set on the ball.
        :type harmonic_value: int
        :param corporation_id: The corporation_id to set on the ball.
        :type corporation_id: int
        :param alliance_id: The alliance_id to set on the ball.
        :type alliance_id: int
        :param is_forcefield: True if we want this ball to stop and switch its mode to DSTBALL_FIELD.
        :type is_forcefield: bool
        """
        if is_forcefield:
            if harmonic_value != -1 or corporation_id != -1 or alliance_id != -1:
                self.set_ball_massive(src_id, True)
            else:
                self.set_ball_massive(src_id, False)

        self._add_to_system_history(
            src_id,
            'SetBallHarmonic',
            harmonic_value, corporation_id, alliance_id, is_forcefield
        )

    def set_ball_radius(self, src_id, radius_meters):
        """
        Sets the radius of the ball in meters.

        :param src_id: The ball whose radius we want to change.
        :type src_id: int
        :param radius_meters: The desired new radius (m)
        :type radius_meters: float
        """
        self._add_to_system_history(
            src_id,
            'SetBallRadius',
            radius_meters
        )

    def set_ball_free(self, src_id, is_free=True):
        """
        Control the balls ability to move.

        :param src_id: The ball whose freedom to move we want to change.
        :type src_id: int
        :param is_free: True if ball should be free to move
        :type is_free: bool
        """
        self._add_to_system_history(
            src_id,
            'SetBallFree',
            is_free
        )

    def set_ball_interactive(self, src_id, is_interactive):
        """
        Sets the interactive flag on the ball.
        :param src_id: The ball whose interactive flag we want to change
        :type src_id: int
        :param is_interactive: True if ball is associated with a user (like a spaceship for example)
        :type is_interactive: bool
        """
        self._add_to_system_history(
            src_id,
            'SetBallInteractive',
            is_interactive
        )

    def follow_ball(self, src_id, dst_id, range_meters):
        """
        Make src_id ball follow dst_id ball at range_meters meters.
        :param src_id: The follower
        :type src_id: int
        :param dst_id: The followee
        :type dst_id: int
        :param range_meters: The range at which src_id follows dst_id
        :type range_meters: float
        """
        self._add_to_system_history(
            src_id,
            'FollowBall',
            dst_id, range_meters
        )

    def set_max_speed(self, src_id, max_meters_per_second):
        """
        Sets the maximum speed that a ball can reach.
        :param src_id: The ball whose max speed we want to change.
        :type src_id: int
        :param max_meters_per_second: The maximum speed for the ball
        :type max_meters_per_second: float
        """
        self._add_to_system_history(
            src_id,
            'SetMaxSpeed',
            max_meters_per_second
        )

    def set_max_angular_speed(self, src_id, max_radians_per_second):
        """
        Sets the maximum angular speed that a ball can reach.
        :param src_id: The ball whose max angular speed we want to change.
        :type src_id: int
        :param max_radians_per_second: The maximum angular speed for the ball
        :type max_radians_per_second: float
        """
        if not is_dynamical_orientation_enabled():
            raise RuntimeError("Attempting to set angular speed, but Dynamical Orientation is disabled.")
        self._add_to_system_history(
            src_id,
            'SetMaxAngularSpeed',
            max_radians_per_second
        )

    def cloak_ball(self, src_id, cloak_mode, uncloak_range_meters):
        """
        Sets the specified cloak mode on the ball. Cloaked balls are not
        visible to other balls, even if they are in the same bubble.

        If the specified cloak mode is DSTNORMALCLOAK,
        then a proximity sensor is added to the ball with the specified range.
        In Eve code, this proximity sensor is used to forcibly decloak the ship
        if a ball that is not in a special list of non-decloaking groups enters
        the proximity range.

        When a ball is cloaked, any ball following or orbiting the cloaked ball stop.
        The cloaked ball stops being massive, and any missiles following the ball
        fly off without collisions.

        :param src_id: The ball whose cloak mode we want to change:
        :type src_id: int
        :param cloak_mode: One of the three destiny cloak modes (char)
            - DSTNORMALCLOAK = 1
            - DSTRESTORECLOAK = 2
            - DSTGMCLOAK = 3
        :type cloak_mode: int
        :param uncloak_range_meters: The range for the proximity sensor that gets added for normal cloaks.
        :type uncloak_range_meters: float
        """
        ball = self._park.balls.get(src_id, None)
        if ball is None:
            return False
        if ball.newBubbleId != ball.oldBubbleId:
            logger.info("cloak_ball( %s ) found ball changing bubbles this tick. Using old bubbleID for cloak", src_id)
            bubble_id = ball.oldBubbleId
        else:
            bubble_id = ball.newBubbleId
        self._add_to_system_history(
            src_id,
            'CloakBall',
            cloak_mode, uncloak_range_meters,
            local_args=bubble_id
        )
        return True

    def uncloak_ball(self, src_id):
        """
        Make a cloaked ball visible again

        :param src_id: The ball to uncloak
        :type src_id: int
        """
        self._add_to_system_history(
            src_id,
            'UncloakBall',
        )

    def launch_missile(self, src_id, dst_id, owner_id, is_aimed_launch, is_missile_massive):
        """
        Launch src_id at dst_id as a missile owned by owner_id.
        Any explosion related logic needs to be handled by the
        Python ball wrapper's DoCollision method.
        :param src_id: The ball that we want to use as our missile.
        :type src_id: int
        :param dst_id: The target ball at which we are launching the missile.
        :type dst_id: int
        :param owner_id: The ball that owns the missile.
        :type owner_id: int
        :param is_aimed_launch: If True, the missile starts off in the direction of the target.
                     If False, the missile starts off in the direction that the owner ball is facing.
                     Either way, the missile will follow its target.
        :type is_aimed_launch: bool
        :param is_missile_massive: True if missile ball should be massive and thus able to collide with things.
        :type is_missile_massive: bool
        """
        self._add_to_system_history(
            src_id,
            'LaunchMissile',
            dst_id, owner_id, is_aimed_launch, is_missile_massive
        )

    @TimedFunction("destiny.net::actions::clean_up_followers")
    def clean_up_followers(self):
        """
        Stop all balls that are following a
        ball which is located in another bubble.
        """
        for followerID in self._park.GetRemoteFollowers():
            self.stop(followerID)

    @TimedFunction("destiny.net::actions::undo_pending_cloak")
    def undo_pending_cloak(self, src_id):
        removed_count = self._prune_pending_actions(src_id, "CloakBall")
        logger.info(
            "undo_pending_cloak trying to prune pending CloakBall calls from the system history for ship %s, count=%s",
            src_id,
            removed_count
        )

    def _prune_pending_actions(self, src_id, event_name):
        """
        Remove any actions with of the event_name variety for
        the specified ball.
        """
        history_length_before_removal = len(self._system_history)
        self._system_history = [
            event
            for event in self._system_history if
            event[0] != src_id or event[2][0] != event_name
        ]
        removed_count = history_length_before_removal - len(self._system_history)
        return removed_count

    def _get_pending_actions(self, src_id, event_name):
        """
        Dig through upcoming actions to see if something
        is about to happen and return a list of matching events.
        """
        return [
            event
            for ball_id, timestamp, event in self._system_history
            if ball_id == src_id and event[0] == event_name
        ]

    def _has_pending_action(self, src_id, event_name):
        """
        Dig through upcoming actions to see if something
        is about to happen and return True if it is.
        """
        for ball_id, timestamp, event in self._system_history:
            if ball_id == src_id and event[0] == event_name:
                return True
        return False

    def has_pending_cloak(self, src_id):
        return self._has_pending_action(src_id, "CloakBall")

    def has_pending_uncloak(self, src_id):
        return self._has_pending_action(src_id, "UncloakBall")

    def get_pending_cloak_mode(self, src_id):
        events = self._get_pending_actions(src_id, "CloakBall")
        if events:
            return events[-1][1][1]
        if src_id not in self._park.balls:
            return None
        return self._park.balls[src_id].isCloaked

    def has_pending_set_free(self, src_id):
        events = self._get_pending_actions(src_id, "SetBallFree")
        return len(events) and events[-1][1][1] == 1

    def has_pending_set_not_free(self, src_id):
        events = self._get_pending_actions(src_id, "SetBallFree")
        return len(events) and events[-1][1][1] == 0

    def get_pending_interactive_state(self, src_id):
        events = self._get_pending_actions(src_id, "SetBallInteractive")
        if events:
            return bool(events[-1][1][1])
        if src_id not in self._park.balls:
            return None
        return self._park.balls[src_id].isInteractive

    def get_pending_speed_fraction(self, src_id):
        events = self._get_pending_actions(src_id, "SetSpeedFraction")
        if events:
            return events[-1][1][1]
        if src_id not in self._park.balls:
            return None
        return self._park.balls[src_id].speedFraction
