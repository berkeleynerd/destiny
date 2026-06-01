# Copyright © 2023 CCP ehf.

import abc
import logging
import sys

import blue

logger = logging.getLogger(__name__)


class BaseTicker(object):
    """
    A base class for client-side PreTick handlers.
    Provides some basic functionality, such as
    logging out the history and checking for
    updates that have data for multiple ticks.
    """
    __metaclass__ = abc.ABCMeta

    def __init__(self, error_handler):
        """
        :param error_handler: Class containing callbacks for when the ticker fails.
        :type error_handler: destiny.net.client.TickErrorHandlerInterface
        """
        self._ballpark = None
        self._history = []  # list of state updates
        self._latest_set_state_time = 0
        self._error_handler = error_handler

    def set_ballpark(self, ballpark):
        """
        Call this when you switch the active ballpark for the user.
        Set the ballpark to None when no ballpark is active.
        """
        self._ballpark = ballpark
        self._error_handler.set_ballpark(ballpark)

    @abc.abstractmethod
    def do_pre_tick(self):
        """
        Call this from a ballparks DoPretick method.
        """
        raise NotImplemented

    def update(self, state, wait_for_bubble):
        """
        Call this when you get state from the server.
        wait_for_bubble means we just got state for
        this user only, but have an upcoming state
        for the bubble we are in.
        """
        if self._ballpark is None:
            return

        state = _expand_states(state)
        self._report_update_if_bad(state)
        self._flush_state(state, wait_for_bubble)

    @abc.abstractmethod
    def _flush_state(self, state, wait_for_bubble):
        """
        Merge the state into history.
        """
        raise NotImplemented

    @property
    def _current_time(self):
        """
        The current tick of the ballpark.
        :rtype: int
        """
        return self._ballpark.currentTime

    def _dump_history(self):
        """
        Log out the contents of self._history.
        """
        if logger.level > logging.INFO:
            return
        logger.info("------ %s History Dump %s -------", self.__class__.__name__, self._current_time)
        rev = self._history[:]
        rev.reverse()
        for state, wait_for_bubble in rev:
            logger.info('state waiting: %s', "yes" if wait_for_bubble else "no")
            last_state = None
            last_state_count = 0
            for entry in state:
                event_stamp, event = entry
                func_name, args = event
                next_state = ["[", event_stamp, "]", func_name]
                if next_state != last_state:
                    if last_state is not None:
                        if last_state_count != 1:
                            last_state.append("(x %d)" % last_state_count)
                        logger.info(" ".join([str(s) for s in last_state]))
                    last_state = next_state
                    last_state_count = 1
                else:
                    last_state_count += 1
            if last_state is not None:
                if last_state_count != 1:
                    last_state.append("(x %d)" % last_state_count)
                logger.info(" ".join([str(s) for s in last_state]))
            logger.info(" ")

    def _report_update_if_bad(self, state):
        """
        Identify and warn about bad updates from the server.
        """
        timestamps = {action[0] for action in state}
        if len(timestamps) > 1:
            logger.error("Found update batch with %s items and %s timestamps", len(state), len(timestamps))
            for action in state:
                logger.error("Action: %s", action)

            self._error_handler.on_fatal_desync()


def _expand_states(state):
    """
    Expand any packaged updates back into their full state
    (A PackagedAction is a pre-serialised stream containing one ore more regular actions.
    There is currently an assumption that a PackagedAction does not recursively contain
    another PackagedAction, but this could be accounted for by refactoring this loop slightly)
    """
    expanded_states = []
    for action in state:
        if action[1][0] == 'PackagedAction':
            try:
                unpackaged_actions = blue.marshal.Load(action[1][1])
                expanded_states.extend(unpackaged_actions)
            except StandardError:
                # Not much we can do here other than log it and try to carry on
                logger.exception("Exception whilst expanding a PackagedAction")
        else:
            # This is not a packaged update, so just use as-is
            expanded_states.append(action)
    state = expanded_states
    return state
