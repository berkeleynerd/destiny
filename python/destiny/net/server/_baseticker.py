import abc
import logging


logger = logging.getLogger(__name__)


class BaseTicker(object):
    """
    A base class for tickers that send out updates
    in synchronization with the ballpark ticks.
    """
    __metaclass__ = abc.ABCMeta

    def __init__(self, park, ball_info):
        self._park = park
        self._ball_info = ball_info
        self.stamp_for_system = self._park.currentTime + 1
        self.current_system_history = []

    def tick(self):
        """
        Call this from the ballparks DoPreTick method.
        """
        self._finalize_tick()
        self._increment_timestamp()
        self._flush_history()
        self._pre_update_bubbles()
        self._update_bubbles()
        self._post_update_bubbles()

    @abc.abstractmethod
    def _finalize_tick(self):
        """
        Gets called on the beginning of DoPretick before we increment the
        timestamp for the solar system.
        """
        pass

    def _increment_timestamp(self):
        """
        Henceforth new system events will happen in the next time step
        """
        self.stamp_for_system = self._park.currentTime + 1

    @abc.abstractmethod
    def _flush_history(self):
        """
        Grab the batch of states to process this tick.
        """
        pass

    @abc.abstractmethod
    def _pre_update_bubbles(self):
        """
        Get anything done that needs to get done before
        update_bubbles is called.
        """
        pass

    @abc.abstractmethod
    def _update_bubbles(self):
        """
        Process bubble additions/deletions.
        Since this can get called by multiple
        tickers in the same tick there is no
        guarantee that the bubbles dict of the
        ballpark has not already been updated
        by the time this gets called.
        """
        pass

    @abc.abstractmethod
    def _post_update_bubbles(self):
        """
        Get anything done that needs to get done after
        update_bubbles is called.
        """
        pass

    def _expand_entry(self, entry):
        event_ball_id, stamp, event = entry
        if stamp != self._park.currentTime and stamp > 0:
            # The first tick is a special case.  Ignore it.
            logger.error(
                "Found systemHistory entry with incorrect stamp %s %s",
                self._park.currentTime,
                entry
            )
        action, args = event[0], event[1]
        local_args = None
        if len(event) > 2:
            local_args = event[2]
            event = event[:-1]
        return action, args, event, event_ball_id, local_args, stamp

    def _find_bubble_id_for_event(self, event_ball_id, action, args):
        if event_ball_id in self._park.balls:
            event_bubble_id = self._park.balls[event_ball_id].newBubbleId
        elif action == 'RemoveBall':
            event_bubble_id = args[1]
        else:
            event_bubble_id = -1
        return event_bubble_id

    def _is_valid_event_bubble(self, event_bubble_id, event_ball_id):
        if event_bubble_id not in self._park.bubbles:
            return False
        if event_bubble_id not in self._park.bubbleInteractives:
            return False  # A dead bubble probably
        if event_ball_id <= 0 or event_ball_id not in self._park.balls:
            return False
        return True
