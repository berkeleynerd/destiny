# Copyright © 2007 CCP ehf.

import collections


def merge_state_into_history(state, history_list, wait_for_bubble):
    """
    Merge the incoming state with the states we already have
    We maintain history as a sorted list, but the incoming states may not be sorted.

    :param state: A list of states to merge into our history.
    :type state: list
    :param history_list: The current history to merge the new states into
    :type history_list: list
    :param wait_for_bubble: True if we are expecting to get another update
                            for this tick, containing an update sent to everyone
                            in the same bubble.

    This used to be a lot simpler, but apparently there are circumstances in which an
    update can contain entries for different time stamps.  And the later time stamp can
    be first in the update, which would lead to the waitForBubble flag not being
    cleared and the bad mixed time stamp update sitting at the head of the queue
    indefinitely blocking subsequent updates.

    67881   2007.07.14 19:03:46:122 state waiting: no
    67882   2007.07.14 19:03:46:122 [ 22168 ] TerminalPlayDestructionEffect
    67883   2007.07.14 19:03:46:122 [ 22168 ] OnSpecialFX (x 2)
    67884   2007.07.14 19:03:46:122 [ 22168 ] FollowBall
    67885   2007.07.14 19:03:46:122 [ 22168 ] GotoDirection
    67886   2007.07.14 19:03:46:122 [ 22168 ] OnSpecialFX
    67887   2007.07.14 19:03:46:122
    67888   2007.07.14 19:03:46:122 state waiting: yes
    67889   2007.07.14 19:03:46:122 [ 22169 ] AddBalls
    67890   2007.07.14 19:03:46:122 [ 22168 ] OnFleetDamageStateChange

    The last update listed is the one which is applied next.  The one before it
    should have been included in it, which would change the update from waiting
    to not waiting.  But because somehow the AddBalls from a later time had slipped
    in first, this did not happen.. and the ballpark synchronisation became blocked.
    """
    entries_by_time = collections.defaultdict(list)
    for entry in state:
        entries_by_time[entry[0]].append(entry)
    time_list = sorted(entries_by_time.keys())
    # Insert events happening earlier than (some of) our current history
    time_list_idx = 0
    for history_idx, history_time_entries in enumerate(history_list):
        history_time = history_time_entries[0][0][0]
        entry_time = time_list[time_list_idx]
        if history_time < entry_time:  # new action happens later
            wait_for_bubble = False  # Because if we need to wait for bubble, then surely we must wait for it in an earlier timestep
            continue
        if history_time == entry_time:  # merge the two and don't wait for bubble
            history_time_entries[0].extend(entries_by_time[entry_time])
            history_time_entries[1] = False
        else:  # history_time > entry_time
            history_list.insert(history_idx, [entries_by_time[entry_time], wait_for_bubble])
            wait_for_bubble = False  # Max 1 wait for bubble
        time_list_idx += 1
        if time_list_idx >= len(time_list):
            break  # time_list (i.e. the new actions) has now been exhausted
    # Insert entries after current history. This loop will be 0 iterations if all of time_list got processed in the
    # loop above.  If that's not the case, there are remaining entries in time_list, which implies that we ran out
    # of history entries before we ran out of new actions in the previous loop, which in turn implies that all the
    # remaining new actions have a timestamp larger than the largest history timestamp, and can thus be appended to
    # the current history.
    for tl_idx in range(time_list_idx, len(time_list)):
        history_list.append([entries_by_time[time_list[tl_idx]], wait_for_bubble])
