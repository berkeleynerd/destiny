# Copyright © 2023 CCP ehf.

"""
Since we are basically dumping this package into each monolith and therefore
cannot rely on any packages inside the particular monolith that it gets
published into, we need to set up super clear boundaries to help make it
easier for multiple product teams to pull in changes.

Sure, things will break, but at least it will be because the interfaces have
changed, but not because we were making assumptions about some product specific
packages existing across all products.
"""
import abc


class NetworkInterface(object):
    __metaclass__ = abc.ABCMeta

    @abc.abstractmethod
    def narrowcast(self, updates):
        """
        Calls remote methods on remote clients.
        Multiple clients can receive the same update.
        Updates is a list of tuples of (client_ids, method, *args)
        where client_ids is a list of integers, method is the name of the
        remote method and *args are the parameters for that method.

        :param updates: A list of updates to be narrowcast to the clients.
                        [(client_ids, method, *args), ...]
        :type updates: list
        """
        pass

    @abc.abstractmethod
    def singlecast(self, updates):
        """
        Calls remote methods on remote clients.
        Exactly one client receives each update.
        Updates is a list of tuples of (client_id, method, *args)
        where client_id is an integer, method is the name of the
        remote method and *args are the parameters for that method.

        :param updates: A list of updates to be singlecast to the clients.
                        [(clientid, method, *args), ...]
        :type updates: list
        """
        pass


class ClientInterestsInterface(object):
    """
    Keeps track of which clients are interested in ballpark updates from the perspective
    of the specified ball. These interested clients may belong to many players docked inside
    a single structure for example, or just a single player in their ship.
    """
    __metaclass__ = abc.ABCMeta

    @abc.abstractmethod
    def add_client_interest(self, ball_id, client_id):
        """
        Register a client_id as having an interest in this ball
        :param ball_id: The ID of the ball in which the client is interested.
        :type ball_id: int
        :param client_id: The ID of the client which is interested in the ball.
        :type client_id: int
        """
        pass

    @abc.abstractmethod
    def get_all_interested_ball_ids(self):
        """
        Returns a list of all balls that have a client interest
        :return: a list of all balls that have a client interest
        :rtype: list
        """
        pass

    @abc.abstractmethod
    def get_interested_client_ids_for_ball(self, ball_id):
        """
        Returns a list of clients interested in this ball
        or an empty list.
        :return: A list of clients interested in this ball
        or an empty list.
        :rtype: list
        """
        pass

    @abc.abstractmethod
    def remove_client_interest(self, ball_id, client_id):
        """
        Removes the client from this balls interested list.
        :param ball_id: The ID of the ball in which the client
                        is no longer interested.
        :type ball_id: int
        :param client_id: The ID of the client which is no longer
                          interested in the ball.
        :type client_id: int
        """
        pass

    @abc.abstractmethod
    def has_client_interest(self, ball_id):
        """
        Does this ball have a client interested in it.
        :return: True if any client has a registered interest
                 in this ball, False otherwise.
        :rtype: bool
        """
        pass


class CharacterInterestsInterface(object):
    """
    Keeps track of which characters are interested in ballpark updates from the perspective
    of the specified ball. These interested characters may belong to many players docked inside
    a single structure for example, or just a single player in their ship.
    """
    __metaclass__ = abc.ABCMeta
    @abc.abstractmethod
    def add_interest(self, ball_id, char_id):
        """
        Register a character as having an interest in this ball
        :param ball_id: The ID of the ball in which the client is interested.
        :type ball_id: int
        :param char_id: The ID of the character which is interested in the ball.
        :type char_id: int
        """
        pass

    @abc.abstractmethod
    def get_interested_character_ids_for_ball(self, ball_id):
        """
        Returns a list of characters interested in this ball
        or an empty list.
        :return: A list of character_ids of characters interested
                 in this ball or an empty list.
        :rtype: list
        """
        pass

    @abc.abstractmethod
    def remove_character_interest(self, ball_id, character_id):
        """
        Removes the client from this balls interested list.
        :param ball_id: The ID of the ball in which the client
                        is no longer interested.
        :type ball_id: int
        :param client_id: The ID of the client which is no longer
                          interested in the ball.
        :type client_id: int
        """
        pass

    @abc.abstractmethod
    def get_client_id_for_character(self, character_id):
        """
        :param character_id: The ID of the character whose client_id
                             we need.
        :type character_id: int
        :return: The client_id if any for the provided character_id.
        :rtype: int or None
        """
        pass


class BallInfoInterface(object):
    """
    An interface to get gameplay dependent information about a ball,
    such as which character it belongs to or whether or not it should
    be considered a "dynamic global object."
    """
    __metaclass__ = abc.ABCMeta

    @abc.abstractmethod
    def get_character_for_ball(self, ball_id):
        """
        Gets the pilot character for a ball.
        :param ball_id: The ID of the ball whose associated character we want to get.
        :type ball_id: int
        :return: The characterID of the player character associated with the ball.
        :rtype: int or None
        """
        pass

    @abc.abstractmethod
    def get_characters_for_ball(self, ball_id):
        """
        Gets a list of all pilot characters for a ball.
        :param ball_id: The ID of the ball whose associated characters we want to get.
        :type ball_id: int
        :return: The characterID of the player character associated with the ball.
        :rtype: int or None
        """
        pass
