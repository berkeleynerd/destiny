import sys
from signals import Signal
import logging

import blue
import bluepy

from destiny.net.server._baseticker import BaseTicker

logger = logging.getLogger(__name__)


PARK_METHODS_THAT_GET_CALLED_DIRECTLY = {
    "RemoveBall",  # Eve has been calling this directly when removing items and fake balls
                   # from the park since time immemorial. Not sure if it's a good idea, but
                   # I'm not going to mess with this today.
}


class Ticker(BaseTicker):
    """
    Applies history from destiny actions and updates bubble
    and user histories on the update batcher, which should
    be sent out in the ballparks DoPostTick method.
    """
    _parent_methods = None  # Lazy init.

    ACTION_HANDLERS = {
        "CloakBall": "_handle_cloak_ball_action",
        "UncloakBall": "_handle_uncloak_action",
        "SetBallPosition": "_handle_set_ball_position_action",
        "AddBallsToPark": "_handle_add_balls_to_park_action",
        "BallNotGlobal": "_handle_ball_not_global_action",
        "RemoveGlobalBall": "_handle_remove_global_ball_action",
    }

    def __init__(self, park, ball_info, actions, update_batcher):
        super(Ticker, self).__init__(park, ball_info)
        self._actions = actions
        self._update_batcher = update_batcher
        self.on_ball_cloaking = Signal()
        self.on_ball_uncloaking = Signal()
        self.on_set_ball_position = Signal()

    def _get_parent_methods(self):
        if Ticker._parent_methods is None:
            Ticker._parent_methods = [
                "_parent_" + x
                for x in dir(self._park)
                if not x.startswith("__") and callable(getattr(self._park, "_parent_" + x, None))
            ]
        return Ticker._parent_methods

    def _clean_up_followers(self):
        # Clean up balls following other balls if that arrangement is no longer tenable
        with bluepy.Timer("Ticker::_clean_up_followers"):
            self._actions.clean_up_followers()

    def _finalize_tick(self):
        self._clean_up_followers()

    def _increment_timestamp(self):
        super(Ticker, self)._increment_timestamp()
        self._actions.set_stamp_for_system(self.stamp_for_system)

    def _flush_history(self):
        self.current_system_history = self._actions.flush_history()

    def _pre_update_bubbles(self):
        self._apply_park_parent_methods_except_cloak_ball()

    def _update_bubbles(self):
        with bluepy.Timer("Ticker::_update_bubbles"):
            self._update_batcher.update_bubbles()

    def _post_update_bubbles(self):
        self._apply_history()

    def _apply_history(self):
        with bluepy.Timer("Ticker::_apply_history"):
            for entry in self.current_system_history:
                # Only apply ballpark methods to ballpark. Note that the 'RemoveBall' commands
                # are handled specially, because they have already been applied to the ballpark
                action, args, event, event_ball_id, local_args, stamp = self._expand_entry(entry)

                handler_method_name = Ticker.ACTION_HANDLERS.get(action, None)
                if handler_method_name is not None:
                    handler = getattr(self, handler_method_name)
                    handled = handler(args, event, event_ball_id, local_args, stamp)
                    if handled:
                        continue

                event_bubble_id = self._find_bubble_id_for_event(event_ball_id, action, args)
                with bluepy.Timer("Ticker::_apply_history::distribute_to_bubbles"):   
                    if not self._is_valid_event_bubble(event_bubble_id, event_ball_id):
                        continue
                    if not self._park.balls[event_ball_id].isCloaked:
                        self._update_batcher.add_to_bubble_history(event_bubble_id, (stamp, event))
                    else:  # Don't send any events about cloaked balls to anyone else
                        charID = self._ball_info.get_character_for_ball(event_ball_id)
                        if charID is not None:
                            self._update_batcher.add_to_character_history(charID, (stamp, event))

    def _apply_park_parent_methods_except_cloak_ball(self):
        with bluepy.Timer("Ticker::_ApplyParkParentMethodsExceptCloakBall"):
            for entry in self.current_system_history:
                event_ball_id, stamp, event = entry
                action, args = event[0], event[1]
                if action == 'CloakBall':
                    continue  # Skip the cloaking bit in destiny until after we update bubbles
                elif action in PARK_METHODS_THAT_GET_CALLED_DIRECTLY:
                    continue  # RemoveBall gets called directly by gameplay server park.
                parent_action_name = '_parent_' + action
                if parent_action_name in self._get_parent_methods():
                    fn = getattr(self._park, parent_action_name)
                    try:
                        fn(*args)
                    except Exception:
                        logger.exception("Bummer in park, fn %s args %s", action, args)
                        continue

    def _handle_cloak_ball_action(self, args, event, event_ball_id, local_args, stamp):
        ball_id = args[0]
        if ball_id in self._park.balls:
            self.on_ball_cloaking(ball_id)
        event_bubble_id = local_args
        self._update_batcher.add_to_bubble_history(event_bubble_id, (stamp, event))
        self._park.CloakBall(*args)
        return True

    def generate_add_balls_update(self, ball_id):
        dump = blue.MemStream()
        self._park.WriteBallsToStream([ball_id], dump)
        ball_stream = dump.Read()
        return ('AddBallsToPark', (ball_stream,))

    def _handle_uncloak_action(self, args, event, event_ball_id, local_args, stamp):
        if args[0] != event_ball_id:
            # Been seeing a few KeyErrors where eventBallID is absolutely not a ball,
            # like 96, 515 and such.  Would like more info on that, hence this.
            logger.error("Malformed 'UncloakBall' entry in system history: %s", str(event))
            event_ball_id = args[0]
        if event_ball_id not in self._park.balls:
            logger.info("'UncloakBall' event for a ball not in park: %s", str(event))
            return True
        event_ball = self._park.balls[event_ball_id]
        self.on_ball_uncloaking(event_ball_id)
        event_bubble_id = event_ball.newBubbleId
        bubble_interactives = self._park.bubbleInteractives.get(event_bubble_id, [])
        add_balls_update = None
        for ball_id in bubble_interactives:
            char_id = self._ball_info.get_character_for_ball(event_ball_id)
            if ball_id == event_ball_id:
                self._update_batcher.add_to_character_history(char_id, (stamp, event))
            else:
                if add_balls_update is None:
                    add_balls_update = self.generate_add_balls_update(event_ball_id)
                char_ids = self._ball_info.get_characters_for_ball(ball_id)
                for charID in filter(None, char_ids):
                    self._update_batcher.add_to_character_history(charID, (stamp, add_balls_update), in_front=True)
        return True

    def _handle_set_ball_position_action(self, args, event, event_ball_id, local_args, stamp):
        is_local_ball = local_args is True
        self.on_set_ball_position(event_ball_id, is_local_ball)
        if is_local_ball:
            logger.info("Local ball moved, notifying noone. %s", event)
            return True
        return False

    def _handle_add_balls_to_park_action(self, args, event, event_ball_id, local_args, stamp):
        for bubble_id in self._park.bubbleInteractives:
            self._update_batcher.add_to_bubble_history(bubble_id, (stamp, event))
        return True

    def _handle_ball_not_global_action(self, args, event, event_ball_id, local_args, stamp):
        #  This is the case where a global ball has been made local.
        #  Thus, remove the ball from all bubbles except the local bubble
        for bubble_id in self._park.bubbleInteractives:
            if bubble_id != args[0]:
                self._update_batcher.add_to_bubble_history(
                    bubble_id,
                    (stamp, ('RemoveBall', (event_ball_id,)))
                )
        return True

    def _handle_remove_global_ball_action(self, args, event, event_ball_id, local_args, stamp):
        # This is the special case when global balls get explicitly deleted
        for bubble_id in self._park.bubbleInteractives:
            self._update_batcher.add_to_bubble_history(
                bubble_id,
                (stamp, ('RemoveBall', (event_ball_id,)))
            )
        return True
