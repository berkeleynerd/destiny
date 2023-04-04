from collections import defaultdict
from destiny.net.server import (
    BallInfoInterface,
    CharacterInterestsInterface,
    ClientInterestsInterface,
    NetworkInterface
)

import blue


class BallInfo(BallInfoInterface):
    def __init__(self):
        self._characters_for_ball = defaultdict(list)

    def get_character_for_ball(self, ball_id):
        return self._characters_for_ball[ball_id][0]

    def get_characters_for_ball(self, ball_id):
        return self._characters_for_ball[ball_id]

    def set_characters_for_ball(self, ball_id, characters):
        self._characters_for_ball[ball_id] = characters


class Network(NetworkInterface):
    def __init__(self):
        self._singlecasts = []
        self._narrowcasts = []
        self._narrowcasts_bytes = []
        self._singlecast_bytes = []

    def narrowcast(self, updates):
        self._narrowcasts += updates
        for each in updates:
            client_ids, method, args = each[0], each[1], each[2:]
            payload = (method, args)
            marshalled_payload = blue.marshal.Save(payload)
            client_count = len(client_ids)
            # We count the payload once per client, since we'll have to
            # send it out to all of them.
            total_narrowcast_bytes = len(marshalled_payload) * client_count
            self.narrowcast_bytes.append(total_narrowcast_bytes)

    def singlecast(self, updates):
        self._singlecasts += updates
        for each in updates:
            client_id, method, args = each[0], each[1], each[2:]
            payload = (method, args)
            marshalled_payload = blue.marshal.Save(payload)
            singlecast_bytes = len(marshalled_payload)
            self._singlecast_bytes.append(singlecast_bytes)

    def clear(self):
        self._singlecasts = []
        self._narrowcasts = []

    @property
    def singlecast_bytes(self):
        return self._singlecast_bytes

    @property
    def narrowcast_bytes(self):
        return self._narrowcasts_bytes


class CharacterInterests(CharacterInterestsInterface):
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


class ClientInterests(ClientInterestsInterface):
    def __init__(self):
        self._ball_id_to_client_ids = defaultdict(list)

    def add_client_interest(self, ball_id, client_id):
        self._ball_id_to_client_ids[ball_id].append(client_id)

    def get_all_interested_ball_ids(self):
        self._ball_id_to_client_ids.keys()

    def get_interested_client_ids_for_ball(self, ball_id):
        return self._ball_id_to_client_ids[ball_id]

    def remove_client_interest(self, ball_id, client_id):
        self._ball_id_to_client_ids[ball_id].remove(client_id)

    def has_client_interest(self, ball_id):
        return bool(self._ball_id_to_client_ids[ball_id])
