import logging
import sys

from destiny.net.client._baseticker import BaseTicker
from destiny.net.client._util import merge_state_into_history

import blue

logger = logging.getLogger(__name__)


LOCAL_ACTIONS = {
    'AddBallsToPark',
    'RemoveBall',
    'RemoveBalls',
}


class Ticker(BaseTicker):
    def __init__(self, error_handler, client_ticker_interface):
        """
        :param error_handler: Class containing callbacks for when the ticker fails.
        :type error_handler: destiny.net.client.TickErrorHandlerInterface
        :param client_ticker_interface: Class containing methods and callbacks for
                                        various game specific things.
        :type error_handler: destiny.net.client.TickErrorHandlerInterface
        """
        super(Ticker, self).__init__(error_handler)
        self._should_rebase = False
        self._last_stamp = -1
        self._states = []
        self._state_is_valid = False
        self._client_ticker_interface = client_ticker_interface

    def set_ballpark(self, ballpark):
        """
        Set the ballpark. Ballpark should be set to None
        if the user does not have a presence in a ballpark.
        :param ballpark: The new ballpark
        :type ballpark: destiny.Ballpark or None
        """
        super(Ticker, self).set_ballpark(ballpark)
        self._client_ticker_interface.set_ballpark(ballpark)

    def _flush_state(self, state, wait_for_bubble):
        """
        Merge state into self._history.
        """
        logger.info("Server Update with %s event(s) added to history", len(state))

        if len(state) == 0:
            logger.warn('Empty state received from remote ballpark')
            return

        if state[0][1][0] == 'SetState':
            # As SetState indicates a reset, we should clear any history from time stamps BEFORE the SetState time stamp
            # (but preserve ones for the same time stamp or later)
            # Need to do this because machonet does not guarantee order of deliver (especially under load)
            self._latest_set_state_time = state[0][0] # Time stamp of the incoming SetState
            logger.info("Michelle received a SetState at time %s. Clearing out-of-date entries...", self._latest_set_state_time)
            # This will update self._history to only contain entries equal-to, or later-than the SetState timestamp.
            # Any entries with a timestamp prior to SetState will be dropped
            self._history = [
                history_entry
                for history_entry in self._history
                if history_entry[0][0][0] >= self._latest_set_state_time
            ]
            self._dump_history()
        else:
            # If this update is from before the previous SetState timestamp, we should discard it now, as it is useless (and may break things!)
            entry_time = state[0][0]  # Time stamp of the incoming action
            if entry_time < self._latest_set_state_time:
                # The update is too old, get rid of it
                logger.warn(
                    "Michelle discarded a state that was too old %s < %s",
                    entry_time,
                    self._latest_set_state_time
                )
                self._dump_history()
                return

        merge_state_into_history(state, self._history, wait_for_bubble)

        self._dump_history()

    def do_pre_tick(self):
        """
        If we are not waiting for bubble events, apply
        history to ballpark.
        """
        while len(self._history) > 0:
            state, wait_for_bubble = self._history[0]
            if wait_for_bubble:
                return

            # Get the time stamp of the first event in the state's event list.  We expect
            # all events in the history state entry to be for the same time stamp.
            event_stamp = state[0][0]
            if event_stamp > self._current_time and event_stamp - self._current_time < 3:
                break

            self._real_flush_state(state)
            del self._history[0]

            # This state may clean our stored state out and replace it with a snapshot of
            # the start of the next tick, at the end of this tick.  We want to go a little
            # further and have an up-to-date snapshot of the current tick to continue to work
            # from in case straggling events for this tick come in with the events for the
            # next tick.
            if self._state_is_valid and self._should_rebase:
                # RealFlushState will have synchronised the ballpark to the time stamp for
                # this state, so we know the state dump we do here will have a correct time
                # for the tick it will have been applied in.
                self.store_state(mid_tick=True)

            # If there are more state updates in the queue and the next one is still waiting, then return
            if len(self._history) > 1:
                if self._history[0][1]:
                    return

                self._ballpark._parent_Evolve()

    def _set_state(self, state, ego_ball_id):
        """
        Read the entire state of the ballpark
        from "state" and set the ego ball to "ego_ball_id"
        :param state: the new state
        :type state: str
        :param ego_ball_id: The ball_id of the ego ball
        :type ego_ball_id: int
        """
        self._client_ticker_interface.on_set_state()
        self._ballpark.ClearAll()

        ms = blue.MemStream()
        ms.Write(state)
        # Make a full SetState
        self._ballpark.ReadFullStateFromStream(ms)
        self._ballpark.ego = long(ego_ball_id)  # This is a ballID Long int.
        self._ballpark.Start()
        self._state_is_valid = True

    def _real_flush_state(self, state):
        """
        Apply the history.
        :param state: A list of actions in chronological order.
        :type state: list
        """
        logger.info("Handling Server Update with %s event(s)", len(state))

        if len(state) == 0:
            logger.warn('Empty state received from remote ballpark')
            return

        # Now first of all, check if the first event is a 'SetState' (they always come first)
        event_stamp, event = state[0]
        func_name, args = event

        if func_name == 'SetState':
            # This is a 'SetState' event, which means that we need to replace the existing state with the one received
            # and forget everything else.

            if self._client_ticker_interface.should_log_actions():
                logger.info("Action: %12.12s %s - current: %s %s", func_name, event_stamp, self._current_time, args)

            self._set_state(*args)  # Rebase is implicit in this

        if self._state_is_valid:
            # This is a 'normal' state update. This means that this is a series of state update, all relative to the same
            # time stamp. Just apply them sequentially to the current ballpark and be done with it.

            self._should_rebase = False
            synchronised = False

            for action in state:
                # Cycle over all actions in state
                event_stamp, event = action
                func_name, args = event

                if func_name == 'SetState':
                    continue

                if self._client_ticker_interface.should_log_actions():
                    logger.info("Action: %12.12s %s - current: %s %s", func_name, event_stamp, self._current_time, args)
                try:
                    # We need to know that the simulation is at the correct time for these updates,
                    # once we know that, we can proceed to just go ahead and apply all the updates
                    # in this group.
                    if not synchronised:
                        synchronised = self.synchronize_to_simulation_time(event_stamp)

                    if not synchronised:
                        # We were not able to adjust from the simulations current time to the time stamp
                        # of this group of updates.  The only thing we can do is reset the simulation.
                        # This should be a last recourse as this only resets the simulation state and all
                        # the other game state which adds the gameplay on top of it will no longer be
                        # correct!  Expect errors as that game state has no handling for this whatsoever.
                        logger.warn("Failed to synchronize to %s", event_stamp)
                        self._error_handler.on_recoverable_desync()
                        return

                    self._should_rebase = True
                    # We are now in the correct state to be applied to.
                    if func_name in LOCAL_ACTIONS: # These are functions that are applied to ourself
                        apply(getattr(self, func_name), args)
                    else:
                        # These are functions that are applied directly to the ballpark
                        apply(getattr(self._ballpark, '_parent_' + func_name), args)
                        self._client_ticker_interface.on_ballpark_local_action(func_name, args)
                        if func_name == 'CloakBall':
                            event_ball_id = args[0]
                            if self._ballpark.ego and self._ballpark.ego != event_ball_id:
                                self.RemoveBall(event_ball_id)
                except Exception:
                    message = "{} failed.".format(func_name)
                    logger.error(message, exc_info=1)
                    sys.exc_clear()
                    continue
        else:
            logger.info("Events ignored")

    def do_post_tick(self, stamp):
        """
        Call this in the ballparks PostTick method.
        It updates the state history in case we need
        to rewind to an earlier state at some point.
        :param stamp: The current timestep of the ballpark
        :type stamp: int
        """
        if self._should_rebase:
            self.flush_simulation_history()
            self._should_rebase = False
        elif stamp > self._last_stamp + 10:
            self.store_state()

    def clear_states(self):
        """
        Clear the simulation snapshot history.
        """
        self._states = []

    def store_state(self, mid_tick=False):
        """
        Updates our history of full ballpark states.
        :param mid_tick: True if this is called from the ballparks PreTick method.
        :type mid_tick: bool
        """
        if not self._ballpark.isRunning:
            return

        ms = blue.MemStream()
        self._ballpark.WriteFullStateToStream(ms)

        self._states.append([ms, self._current_time, mid_tick])

        logger.info("store_state: %s mid_tick: %s", self._current_time, mid_tick)

        if len(self._states) > 10:
            self._states = self._states[:1] + self._states[3:3] + self._states[-5:]

        self._last_stamp = self._current_time

    def synchronize_to_simulation_time(self, stamp):
        """
        Rewind or fast-forward if necessary, so that we can apply an update
        at the correct timestamp. Rewinding is done by reading a snapshot saved
        from the destiny simulation if we have one on hand, so we end up
        losing any updates that we have applied at later timestamps.
        """
        logger.info("SynchroniseToSimulationTime looking for: %s - current: %s", stamp, self._current_time)

        if stamp < self._current_time:
            # We are ahead of this state. we must go back

            # Find the last state with a time stamp equal or less than the given stamp
            last_stamp = 0
            last_state = None
            for item in self._states:
                if item[1] <= stamp:
                    last_stamp = item[1]
                    last_state = item[0]
                else:
                    continue

            # Hmm. We didn't find any such state. We basically need a SetState to become in synch again.
            if not last_state:
                logger.warn('SynchroniseToSimulationTime: Did not find any state')
                return 0

            # OK. We have a valid state that is less or equal to the one given. Go there.
            self._ballpark._parent_ReadFullStateFromStream(last_state, 1)
        else:
            # We are behind this state
            last_stamp = self._current_time

        # In all cases now, 'last_stamp' is less or equal to 'stamp', and 'last_state' is the state at 'last_stamp'
        # So now we can evolve ballpark to reach the time 'stamp'
        for i in range(stamp - last_stamp):
            self._ballpark._parent_Evolve()

        # We are now with a valid state for time 'stamp'
        logger.info("SynchroniseToSimulationTime found: %s", self._current_time)
        return 1

    def flush_simulation_history(self, new_base_snapshot=True):
        """
        Clear out simulation history and create a new one with
        the current state of the ballpark, and the mid-state from the
        previous tick if that is the last stored state,
        unless new_base_snapshot is False, in which case there is no
        point creating a new snapshot, since we are releasing the ballpark.
        :param new_base_snapshot: True if we should update our state history with
                                  the latest states, False if we just want to clear
                                  it out
        :type new_base_snapshot: bool
        """
        logger.info(
            "State history rebased at %s newBaseSnapshot %s",
            self._current_time,
            new_base_snapshot,
        )

        last_mid_state = None
        if new_base_snapshot and len(self._states):
            # If the last stored state is the mid-state from the previous tick,
            # then we want to keep it around, in case a straggling update arrives.
            last_mid_state = self._states[-1]
            if not last_mid_state[2] or last_mid_state[1] != self._current_time-1:
                last_mid_state = None

        self.clear_states()

        if new_base_snapshot:
            # If we have a mid-state for the previous tick, use it as well.
            if last_mid_state:
                self._states.append(last_mid_state)

            self.store_state()
            for state in self._states:
                logger.info("State entry %s", state)

    def AddBallsToPark(self, state):
        """
        Make a 'partial' SetState. Balls get added to park,
        but nothing gets cleared out.
        :param state: A binary stream of ball data for the balls to add.
        :type state: str
        """
        ms = blue.MemStream()
        ms.Write(state)
        self._ballpark.ReadFullStateFromStream(ms, 2)

    def RemoveBall(self, ball_id, terminal=False):
        """
        Handler for the RemoveBall action.
        :param ball_id: the ID of the ball.
        :type ball_id: int
        :param terminal: True if ball is being "destroyed" (i.e. ship exploding)
        :type terminal: bool
        """
        blue.statistics.EnterZone("RemoveBall")
        try:
            if terminal:
                logger.info("Removing ball %s (terminal)")
            else:
                logger.info("Removing ball %s")
            ball = self._ballpark.balls.get(ball_id, None)
            delay = self._client_ticker_interface.get_ball_destruction_delay(ball, is_terminal=terminal)
            self._ballpark._parent_RemoveBall(ball_id, delay)
            self._client_ticker_interface.clean_up_after_ball_removal(ball_id, ball, is_terminal=terminal)
        finally:
            blue.statistics.LeaveZone()

    def RemoveBalls(self, ball_ids, is_release=False):
        """
        Handler for the RemoveBalls action. Also called when ballpark is being released.
        :param ball_ids: A list of the IDs of the balls.
        :type ball_id: list
        :param is_release: True if ballpark is being released.
        :type is_release: bool
        """
        blue.statistics.EnterZone("RemoveBalls")
        try:
            self._client_ticker_interface.clean_up_after_multiple_ball_removal(ball_ids, is_release=is_release)
            removal_delays = self._client_ticker_interface.get_ball_destruction_delays(ball_ids, is_release=is_release)

            for ball_id, delay in removal_delays.iteritems():
                self._ballpark._parent_RemoveBall(ball_id, delay)
        finally:
            blue.statistics.LeaveZone()

    def invalidate_state(self):
        """
        Sets a flag that says that our state is currently invalid.
        Let's hope someone has requested a new SetState action from
        the server.
        """
        self._state_is_valid = False

    def release(self):
        """
        Clean up balls from the ballpark and get rid of any
        resources we may be hanging on to. Also mark ourselves
        as not having valid state.
        """
        # Remove all balls
        self.RemoveBalls(self._ballpark.balls.iterkeys(), is_release=True)
        # Clean up the ball park, it would be nice if this would clean up the blue balls efficiently as well.
        self._ballpark.ClearAll()
        self.flush_simulation_history(new_base_snapshot=False)
        self._history = []
        self._latest_set_state_time = 0
        self._state_is_valid = False
