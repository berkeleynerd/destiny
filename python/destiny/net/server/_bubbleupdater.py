# Copyright © 2023 CCP ehf.

import itertools


class BubbleUpdater(object):
    """
    Updates ball bubble presence in the "bubbles" dict
    on the park.
    """
    def __init__(self, park):
        self._park = park
        self._tick = None
        self.additions_per_player = {}
        self.deletions_per_player = {}
        self.additions_per_bubble = {}
        self.deletions_per_bubble = {}

    def additions_and_deletions(self):
        """
        Updates ball bubble presence in the "bubbles" dict
        on the park if it hasn't already been done this tick.
        Keeps track of additions and deletions by updating the
        member additions and deletions dicts.
        """
        if self._tick != self._park.currentTime:
            self._process_additions_and_deletions_for_tick()
            self._park.CopyBubbles()  # Update the parks "bubbles" dict.
            self._tick = self._park.currentTime

    def _process_additions_and_deletions_for_tick(self):
        # Find those interactives that belong to this solar system
        self.user_balls = list(itertools.chain(*self._park.bubbleInteractives.values()))
        # For users who have changed bubbles since the last tick, we'll get a user-specific set of adds/deletes here
        # key   : ball_id
        # value : A single bubble_id OR a list of ballIDs
        # When value is a bubble_id, it should be used to look up the actual list of ballIDs from the per-bubble dicts
        self.additions_per_player = {}
        self.deletions_per_player = {}
        # For users who have not changes bubbles since the last tick, we'll get a bubble-specific set of adds/deletes here,
        # This allows us to make major savings by not serialising the same data lots of times.
        # key   : bubble_id
        # value : A list of ballIDs
        self.additions_per_bubble = {}
        self.deletions_per_bubble = {}

        # Populate addition/deletion dictionaries
        self._park.AdditionsAndDeletions(
            self.additions_per_player,
            self.deletions_per_player,
            self.additions_per_bubble,
            self.deletions_per_bubble,
            self.user_balls
        )
