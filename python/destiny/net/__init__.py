"""
This package encapsulates the performing of actions such as GotoPoint, Orbit, etc.

The Actions class exposes methods for performing the actions, which get added to
a "system history". The server-side Ticker class flushes these actions each PreTick
(The PreTick gets processed right before the ballpark "ticks"). Once the Ticker has
the actions for the tick, it goes through them one by one and applies them to the ballpark,
keeping lists for updates that need to be sent to clients that have a presence in particular
bubbles, or (in the case of cloaked balls) to individual clients. Some actions performed are
not applied to the ballpark per se, but have to do with managing which balls are visible to which clients.

The client side ticker can receive 0-2 updates for the same tick. Potentially one for the bubble
and potentially one for the user. The updates get processed by the tickers update() method.
User events get sent out first, so when the user is about to receive a bubble event as well,
the wait_for_bubble flag is set to true, then during the client parks PreTick the accumulated
history gets applied.

This package contains interfaces to try to abstract away specific details. One of these is networking
so this package does not specify how the updates get sent to the clients, but it is very important that
the client ticker should always receive the user updates before the bubble ones.
"""
