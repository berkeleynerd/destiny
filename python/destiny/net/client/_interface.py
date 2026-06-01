# Copyright © 2023 CCP ehf.

import abc


class ClientTickerInterface:
    __metaclass__ = abc.ABCMeta

    def set_ballpark(self, ballpark):
        self._ballpark = ballpark

    @abc.abstractmethod
    def on_set_state(self):
        """
        Existing ballpark is about to have its balls removed.
        Client should clean up existing ball-related data.
        """
        pass

    @abc.abstractmethod
    def on_ballpark_local_action(self, func_name, args):
        """
        called when we apply an action to the ballpark
        using the parent_xxx methods, except _parent_RemoveBall.
        :param func_name: The name of the function, so if we have called
                          _parent_RemoveBall func_name is RemoveBall.
        :type func_name: str
        :param args: The arguments sent to the function
        :type args: tuple
        """
        pass

    @abc.abstractmethod
    def should_log_actions(self):
        """
        :return: True if actions should be logged out as
        they are applied, False otherwise.
        :rtype: bool
        """
        pass

    @abc.abstractmethod
    def get_ball_destruction_delay(self, ball, is_terminal=False):
        """
        :param ball: The ball for which to get the destruction delay.
        :type ball: destiny.Ball
        :param is_terminal: True if ball currently exploding
        :type is_terminal: bool
        :return:  How many ticks to wait before removing
                  the ball after destruction.
        :rtype: int
        """
        pass

    @abc.abstractmethod
    def get_ball_destruction_delays(self, ball_ids, is_release=False):
        """
        :param ball_ids: A list of ball_ids to get destruction delays for.
        :type ball_ids: list
        :param is_release: True if the ballpark is being released
                           (in which case all the destruction delays should be 0)
        :type is_release: bool
        :return: A dict mapping from ball_id to how many ticks to wait
        before removing each ball after destruction.
        :rtype: dict
        """
        pass

    @abc.abstractmethod
    def clean_up_after_ball_removal(self, ball_id, ball, is_terminal=False):
        """
        Called after a ball gets removed from the park.
        This is a good time to clean up after the removed ball.

        :param ball_id: The id of the ball that got removed from the park.
        :type ball_id: int
        :param ball: The ball that got removed from the park:
        :type ball: destiny.Ball
        :param is_terminal: True if the ball exploded
        :type is_terminal: bool
        """
        pass

    @abc.abstractmethod
    def clean_up_after_multiple_ball_removal(self, ball_ids, is_release=False):
        """
        Called after multiple balls get removed from the park.
        This is a good time to clean up after the removed balls.

        :param ball_ids: The IDs of the balls that got removed from the park.
        :type ball_ids: list
        :param is_release: True if the ballpark is being released
        :type is_release: bool
        """
        pass


class TickErrorHandlerInterface:
    __metaclass__ = abc.ABCMeta

    def set_ballpark(self, ballpark):
        self._ballpark = ballpark

    @abc.abstractmethod
    def on_fatal_desync(self):
        """
        We somehow got a state update from the server containing
        updates for multiple timestamps.
        """
        pass

    @abc.abstractmethod
    def on_recoverable_desync(self):
        """
        A recoverable desync has occurred. This presents an opportunity
        to request a state reset from the server.
        """
        pass
