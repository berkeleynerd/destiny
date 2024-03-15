from destiny.test import helpers


class TestNotifyOfBubbleTransitions(helpers.BallparkTestCase):

    def setUp(self):
        super(TestNotifyOfBubbleTransitions, self).setUp()
        self.transitions_notified = []

    def _notify_of_transitions(self, transitions):
        self.transitions_notified = transitions

    def test_notify_of_transitions_does_not_notify_if_there_are_no_transitions(self):

        """
        ball, = self.add_balls(1)
        self.park.InitializeBubbles()

        userShips = [ball.id, ]
        additionsPerPlayer = {}
        deletionsPerPlayer = {}
        additionsPerBubble = {}
        deletionsPerBubble = {}

        self.park.AdditionsAndDeletions(
            additionsPerPlayer, deletionsPerPlayer, additionsPerBubble, deletionsPerBubble, userShips,
        )
        """
        transitions = [
            (), # ballID, oldBubbleID, newBubbleID
        ]
        self.park.DoBubbleTransitions()
