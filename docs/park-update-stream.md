# The Park-Update Stream (D-04) — DRAFT

Wire format of destiny's state-synchronization stream, recovered from the
reference implementation (`Ballpark::WriteBallToStream` /
`ReadFullStateFromStream`, `src/Thunkers.cpp`) and pinned by the Python
oracle `python/destiny/test/ballpark/test_stream_read_write.py`. All values
little-endian, packed in write order with no alignment padding (raw
`ICcpStream::Write` of each field). Status: packet layouts below are
source-verified; sections marked OPEN are the remainder of the D-04 unit.

## Packet header

| Field | Type | Notes |
| --- | --- | --- |
| packet | u8 | `DESTINY_FULLSTATE = 0`, `DESTINY_BALLS = 1` (IDstConstants.h) |
| timestamp | i32 | `Ballpark::mCurrentTime` truncated to 32 bits; the reader adopts it as its current time |

Golden vector (oracle): an empty `DESTINY_BALLS` stream is exactly
`01 00 00 00 00`.

Then zero or more ball records until end of stream. `DESTINY_FULLSTATE`
writes every ball with `mId >= DSTLOCALBALLS` (= -1073741824); on the
master with a source ball given, it is bubble-limited (skips non-global
balls in other bubbles and cloaked balls other than the source). Moribund
balls are never written.

## Ball record

Fixed prefix:

| Field | Type | Source |
| --- | --- | --- |
| id | i64 | `mId` |
| mode | u8 | `DSTBALLMODE` |
| radius | f32 | `mRadius` |
| position | 3 × f64 | `mNewPos` |
| flags | u8 | see below |

Flags (`BALLFLAGS`): `ISFREE 0x01`, `ISGLOBAL 0x02`, `ISMASSIVE 0x04`,
`ISINTERACTIVE 0x08`, `ISSPACEJUNK 0x10`, `HASMINIBOXES 0x20`,
`HASMINIBALLS 0x40`, `HASMINICAPSULES 0x80`. The mini-shape flags are
derived from the lists at write time, not stored state.

If `mode != DSTBALL_RIGID`:

| Field | Type | Source |
| --- | --- | --- |
| mass | f64 | `mMass` |
| isCloaked | i8 | |
| harmonic | i64 | `mHarmonic` |
| corporationID | i32 | truncated from ID |
| allianceID | i32 | truncated from ID |

If `flags & ISFREE`:

| Field | Type | Source |
| --- | --- | --- |
| maxVel | f32 | |
| velocity | 3 × f64 | `mNewVel` |
| agility | f32 | |
| speedFraction | f32 | |

**Format variability:** if (and only if) `g_useDynamicalOrientation` is
enabled at write time, the free-ball block continues with rotation
(4 × f32 quaternion), maxAngVel (f32), angularVelocity (3 × f32), and
rotationalAgility (f32). Reader and writer must agree on the setting —
the flag is not encoded in the stream. An independent consumer must treat
this as a stream-profile parameter.

Then always: formationID (i8).

Mode-specific block:

| Mode | Fields |
| --- | --- |
| FOLLOW | followId i64, followRange f32 |
| FORMATION | followId i64, followRange f32, effectStamp i32 |
| MISSILE | followId i64, followRange f32, ownerId i64, effectStamp i32, goto 3 × f64 |
| ORBIT | followId i64, followRange f32 |
| GOTO | goto 3 × f64 |
| WARP | goto 3 × f64 (destination), effectStamp i32, totalWarpLength f64 (shanghaied `mLastCollision`), minimumRange i64-slot (shanghaied `mFollowId`, semantically a double per the D-07 unpunning), warpFactor i64-slot (shanghaied `mOwnerId`) |
| MUSHROOM | followRange f32, span f64 (shanghaied `mGoto.x`), effectStamp i32, ownerId i64 |
| TROLL | effectStamp i32 |
| STOP / FIELD / RIGID | none |

Mini-shape blocks, each present only when its flag is set, each with a
u16 count prefix:

- miniballs: count × (pos 3 × f64, radius f32)
- minicapsules: count × (hemisphereA 3 × f64, hemisphereB 3 × f64, radius f32)
- miniboxes: count × (corner, localX, localY, localZ — each 3 × f64)

Write order is balls → miniballs → minicapsules → miniboxes (note the
capsule/box order differs from the flag-bit order).

## Reader semantics (source-verified)

`ReadBallFromStream(stream, partial)` — verified prefix (Thunkers.cpp:2899):

- **End of stream** is a short read of the 8-byte id (`Read > 0` fails):
  the record loop terminates and the packet is complete; there is no
  count field or terminator byte.
- Fields are read symmetrically to the writer through the free-ball
  block. The dynamical-orientation quaternion block is read only when the
  READER's `g_useDynamicalOrientation` is set, confirming the setting is
  a stream-profile parameter that both ends must share.
- Balls are **created, not updated**: `AddDynamicallyOrientedBall` under
  dynamical orientation, `AddOldStyleOrientedBall` otherwise — the
  reader's setting also selects the creation path. Defaults for fields
  absent from RIGID/non-free records: mass sentinel `1.0e34`,
  harmonic -1, corporation/alliance -1, identity rotation.
- `partial != 1` and the ball is not free: existing mini shapes are
  destroyed (mini balls individually `RemoveBall`'d, capsules and boxes
  likewise, lists cleared) before the stream's shapes are re-read —
  non-partial reads REPLACE compound collision geometry on non-free
  balls; `partial = 1` preserves it.
- The mode-specific block reads mirror the writer field-for-field
  (verified through the full mode table); `mEffectStamp` widens from the
  wire's i32. The mode byte is applied directly after creation, before
  follow-topology resolution.
- **Robustness caveat:** an unknown mode byte aborts the record
  (`return 0`), which the packet loop treats as end of stream — a
  corrupt record silently truncates the packet rather than failing it.
  The successor codec should reject, not truncate.
- Mini-shape blocks read as written: flag-gated, u16 count, fields per
  shape.

`ReadFullStateFromStream(stream, partial)` adopts the stream timestamp as
`mCurrentTime`, reads ball records to end of stream, then resolves follow
topology: for FOLLOW/MISSILE/ORBIT/FORMATION balls, `followId` is resolved
to a live ball, the follower registers in the target's `mFollowers` set,
and **a missing follow target degrades the ball to DSTBALL_GOTO** rather
than failing the stream.

## Additions and deletions (source-verified)

`AdditionsAndDeletions` is **not a wire packet** — it is the host-side
delta builder. From the bubble transition journal (the +1/0/-1 per-ball
actions `AddToBubble` records and `CopyBubbles` prunes), it clears and
fills four Python dicts: additions/deletions per bubble (for users who
stayed in their bubble this tick — the common case the source comment
calls out for fleet fights) and additions/deletions per player (for users
who crossed bubbles, computed against their old/new bubble pair). Only
bubbles containing interactive balls are materialized. Filters: journal
action +1 appends to the add list, -1 to the delete list; cloaked-ball
removals are suppressed; adds skip missing, global, and cloaked balls and
the observer itself. The values are ball-ID lists — the wire bytes for an
addition are produced separately by `WriteBallsToStream` over those IDs.
Framing of the pair (ID deltas + ball streams) belongs to the
`python/destiny/net` layer (next OPEN item).

## OPEN — remainder of the D-04 unit

1. The `python/destiny/net` layer: server ticker cadence, per-client
   bubble batching, tick numbering as seen by the client ticker — the
   framing that carries these packets.
2. RNG inventory: where randomness enters evolution (proximity shuffle
   et al.) and what seed the recorder must journal.
3. The recorder itself: the four wire functions currently live in the
   D-03-gated `Thunkers.cpp` although their cores are Python-free
   (IBlueStream only) — relocate the cores un-gated, expose a
   scripted-scenario recorder on the embedded seam, and pin determinism
   (same scenario → byte-identical stream, tick numbers + RNG seed in the
   header of the recording, not the wire packets).
