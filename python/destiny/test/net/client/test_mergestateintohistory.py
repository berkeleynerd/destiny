import unittest

from destiny.net.client import merge_state_into_history


SHIP_ID_1 = 1000000176143
GOTO_DIRECTION = ('GotoDirection', (SHIP_ID_1, 1.0, 0.0, 0.0))
STOP = ('Stop', (SHIP_ID_1,))


class TestMergeStateIntoHistory(unittest.TestCase):
    def test_merging_empty_state_list_has_no_effect(self):
        state = []
        history_list = []
        merge_state_into_history(state, history_list, True)
        merge_state_into_history(state, history_list, False)
        self.assertListEqual([], history_list)

    def test_merging_empty_state_list_with_wait_for_bubble(self):
        state = [(101, GOTO_DIRECTION)]
        history_list = []
        merge_state_into_history(state, history_list, True)
        self.assertListEqual(
            [[state, True]],
            history_list
        )

    def test_merging_empty_state_list_without_wait_for_bubble(self):
        state = [(101, GOTO_DIRECTION)]
        history_list = []
        merge_state_into_history(state, history_list, False)
        self.assertListEqual(
            [[state, False]],
            history_list
        )

    def test_merge_two_states_same_tick_wait_for_bubble_should_clear(self):
        state_one = [(101, GOTO_DIRECTION)]
        state_two = [(101, STOP)]
        history_list = []
        merge_state_into_history(state_one, history_list, True)
        merge_state_into_history(state_two, history_list, False)
        self.assertListEqual(
            [[state_one + state_two, False]],
            history_list
        )

    def test_merge_two_states_same_tick_wait_for_bubble_should_clear_if_wait_in_second_state(self):
        state_one = [(101, GOTO_DIRECTION)]
        state_two = [(101, STOP)]
        history_list = []
        merge_state_into_history(state_one, history_list, False)
        merge_state_into_history(state_two, history_list, True)
        self.assertListEqual(
            [[state_one + state_two, False]],
            history_list
        )

    def test_merge_two_states_same_tick_no_wait_for_bubble(self):
        state_one = [(101, GOTO_DIRECTION)]
        state_two = [(101, STOP)]
        history_list = []
        merge_state_into_history(state_one, history_list, False)
        merge_state_into_history(state_two, history_list, False)
        self.assertListEqual(
            [[state_one + state_two, False]],
            history_list
        )

    def test_merge_newer_state_wait_for_bubble_persists_for_older_state(self):
        state_one = [(101, GOTO_DIRECTION)]
        state_two = [(102, STOP)]
        history_list = []
        merge_state_into_history(state_one, history_list, True)
        merge_state_into_history(state_two, history_list, False)
        self.assertListEqual(
            [
                [state_one, True],
                [state_two, False]],
            history_list
        )

    def test_merge_newer_state_wait_for_bubble_persists_for_newer_state(self):
        state_one = [(101, GOTO_DIRECTION)]
        state_two = [(102, STOP)]
        history_list = []
        merge_state_into_history(state_one, history_list, True)
        merge_state_into_history(state_two, history_list, False)
        self.assertListEqual(
            [
                [state_one, True],
                [state_two, False]],
            history_list
        )

    def test_merge_older_state(self):
        state_newer = [(101, GOTO_DIRECTION)]
        state_older = [(100, STOP)]
        history_list = []
        merge_state_into_history(state_newer, history_list, False)
        merge_state_into_history(state_older, history_list, False)
        self.assertListEqual(
            [
                [state_older, False],
                [state_newer, False]],
            history_list
        )

    def test_merge_older_state_older_wait_for_bubble(self):
        state_newer = [(101, GOTO_DIRECTION)]
        state_older = [(100, STOP)]
        history_list = []
        merge_state_into_history(state_newer, history_list, False)
        merge_state_into_history(state_older, history_list, True)
        self.assertListEqual(
            [
                [state_older, True],
                [state_newer, False]],
            history_list
        )

    def test_merge_older_state_newer_wait_for_bubble(self):
        state_newer = [(101, GOTO_DIRECTION)]
        state_older = [(100, STOP)]
        history_list = []
        merge_state_into_history(state_newer, history_list, True)
        merge_state_into_history(state_older, history_list, False)
        self.assertListEqual(
            [
                [state_older, False],
                [state_newer, True]],
            history_list
        )
