# Thunker Contract Audit (D-03)

Complete inventory of the Python-exposed surface excised behind
`DESTINY_WITH_PYTHON` (seam mechanics and symbol gate:
[destiny-python-seam.md](destiny-python-seam.md)), classified by where each
entry's semantics survive.
This page is the input document for the successor host seam
(Ravenscar/Jorvik): everything marked `[UNIQUE]` encodes simulation
semantics reachable only through this surface today, and must be consciously
adopted, corpus'd, or rejected by the successor — nothing on this page may
die silently.

Legend: `[EMBEDDED]` covered by the embedded C API (D-01+),
`[CORPUS]` behavior pinned by D-02 corpora, `[TRIVIAL]` plain state
accessor already visible through embedded diagnostics/config,
`[SUPERSEDED]` replaced by a deliberately different embedded mechanism,
`[BRIDGE]` Python-mechanism-only, `[UNIQUE]` semantics preserved nowhere
else, `[D-04]` chartered to the wire-format unit.

## Ballpark (75 methods)

### Motion commands
| Entry | Class | Disposition |
| --- | --- | --- |
| GotoPoint | command | [EMBEDDED][CORPUS] Destiny_CommandEmbeddedGoto, PL-11A |
| Stop | command | [EMBEDDED][CORPUS] Destiny_CommandEmbeddedStop |
| Orbit | command | [EMBEDDED][CORPUS] Destiny_CommandEmbeddedOrbit, PL-11B |
| WarpTo | command | [EMBEDDED][CORPUS] Destiny_CommandEmbeddedWarp, PL-11C |
| FollowBall | command | [EMBEDDED][CORPUS] Destiny_CommandEmbeddedFollow, PL-11D |
| GotoDirection | command | [EMBEDDED] Destiny_CommandEmbeddedGotoDirection (warp-journey route commands); trajectory uncorpus'd |
| MissileFollow, LaunchMissile | commands | [UNIQUE] DSTBALL_MISSILE entry points — lead-pursuit homing; charter of the future combat slice (PL-C0) |
| EntityWarpIn | command | [UNIQUE] NPC warp-in (`Ballpark::EntityWarpIn`, the RealWarp variant at src/Ballpark.cpp:4416) — spawn-in-warp semantics |
| FormationFollow, SetBallFormation, LoadFormations | commands | [UNIQUE] DSTBALL_FORMATION — fleet formation flight |
| SetBoid, SetBallTroll, SetBallRigid, SetBallHarmonic | mode setters | [UNIQUE] mode entry for BOID/TROLL/RIGID/harmonic oscillation; RIGID creation itself is [EMBEDDED] (Destiny_AddEmbeddedCelestial), the rest are inert-mode dressing |

### Ball lifecycle and state setters
| Entry | Class | Disposition |
| --- | --- | --- |
| __init__, AddBall, (AddDynamicallyOrientedBall / AddOldStyleOrientedBall / AddCapsule / AddOrientedBox via Thunkers) | creation | [EMBEDDED] session config + fixed target + celestials cover the used paths; capsule/box collision shapes [UNIQUE] to collision fidelity (D-04/D-05 territory) |
| SetBallMass/Agility/Radius/MaxSpeed/MaxAngularSpeed/AngularAgility | setters | [TRIVIAL] creation-time config in embedded; runtime mutation uncorpus'd |
| SetBallGlobal/Massive/Interactive/Free | flag setters | [TRIVIAL] embedded config; runtime flips appear in sim (warp suspension) and are corpus-gated there |
| SetBallPosition/Velocity/Rotation/AngularVelocity | teleport setters | [UNIQUE] out-of-band state injection (server corrections); deliberately absent from the embedded seam — successor must decide whether to admit them |
| SetSpeedFraction | throttle | [UNIQUE] scales terminal velocity + thrust mid-flight; the client speed slider; uncorpus'd (all corpora run 1.0) |
| SetTargetTracking, GetAccuracy | tracking | [UNIQUE] turret-tracking accuracy surface — combat-slice charter |
| CloakBall / UncloakBall | visibility | [UNIQUE] cloak drops followers/targeting (StopAllFollowers path) — interaction semantics uncorpus'd |
| ClearAll | lifecycle | [TRIVIAL] session teardown |

### Clock
| Entry | Disposition |
| --- | --- |
| Start, Pause, Evolve, AdjustTimes | [SUPERSEDED] the embedded seam owns a deterministic tick clock (Destiny_AdvanceEmbeddedSession); Python-driven wall-clock scheduling is deliberately excluded |

### Spatial queries and proximity
| Entry | Disposition |
| --- | --- |
| GetBallIdsInRange / AndDistInRange / InCone / InCapsule / InRangeOfTriangle, ScanCone | [UNIQUE] scan/d-scan spatial query family |
| AddProximitySensor / RemoveProximitySensor / SetNotificationRange | [UNIQUE] proximity event registration (bombs, gates, aggro ranges) |
| CheckVisibility | [UNIQUE-dormant] sphere-ray occlusion; zero client callers — preserved as documentation, deliberately unused |
| GetSurfaceDist / GetCenterDist / GetCurrentEgoPos / GetFollowers / GetRemoteFollowers | [TRIVIAL] diagnostics expose equivalents for the corpus'd fixtures |

### Bubble / partition management
| Entry | Disposition |
| --- | --- |
| InitializeBubbles, CopyBubbles, GetBubbleAtCoordinates, GetBallBox, GetBallBoxKeys, GetBoxKey, GetBoxChildren, GetBoxCenter, GetBoxObject, SetBoxObject, GetActiveBoxes, GetStaticCollidableBox | [UNIQUE] bubble/octree partition surface — server-side space management; the embedded single-bubble fixture never touches it; successor scope decision required |

### Wire format
| Entry | Disposition |
| --- | --- |
| WriteFullStateToStream, WriteBallsToStream, ReadFullStateFromStream, AdditionsAndDeletions | [D-04] the park-update stream — explicitly chartered to the wire-format unit; do not lose |

### Bridge-only
| Entry | Disposition |
| --- | --- |
| SetBallNotInParkCallback | [BRIDGE] Python error-callback plumbing |

## Ball (9 methods)
| Entry | Disposition |
| --- | --- |
| __init__ | [BRIDGE] |
| AddMiniBall / AddMiniCapsule / AddMiniBox | [UNIQUE] compound collision sub-shapes — collision-fidelity charter (D-04/D-05) |
| GetRotatedVector | [TRIVIAL] |
| AddProximitySensor | [UNIQUE] see proximity family |
| ReserveFormationSlot / FreeFormationSlot | [UNIQUE] formation slots |
| GetPartitionBoxes | [UNIQUE] partition surface |

## MiniBall / MiniBox / MiniCapsule (1 each)
`__init__` only — [BRIDGE].

## GlobalSettings (4 methods)
Apply/Get/GetDefault/Reset — [UNIQUE-config] the tunable-constants surface
(`g_useNewOrbit` et al.). The embedded options struct admits exactly the two
knobs the corpora use (orbit policy, orientation policy); the full settings
inventory transfers to the successor as configuration schema, not API.

## Outbound events (destiny → host)
Exactly three semantic events exist, preserved in executable form by the
D-03 embedded warp-event callback (DestinyEmbedded.h) and pinned by the warp
contract test: a completed warp fires `OnActivatingWarp` at the RealWarp
tick (mCurrentTime 10 in the PL-11C fixture), `OnDeactivatingWarp` at
dropout (tick 30), and `OnExitWarp` from Ball::SetMode's WARP-exit hook with
its historical literal time 0. The WarpTo sub-100 km downgrade path posts an
additional OnExitWarp(t=1) that the embedded seam makes unreachable (it
rejects sub-200 km warps by contract). The collision/proximity SendEvent
family stays macro-silenced in embedded builds — chartered to the combat and
proximity contracts. The 38 `Destiny::*` timer labels are profiling
instrumentation, not contract.

## Summary counts
91 exposed methods: 8 [EMBEDDED+CORPUS], ~20 [TRIVIAL], 4 [SUPERSEDED],
4 [D-04], 5 [BRIDGE], ~50 [UNIQUE] across seven families — manual flight,
missiles/combat, formations/inert modes, out-of-band state injection,
spatial/proximity queries, bubble management, settings schema. Each [UNIQUE]
family is a successor-seam scope decision, not an accidental loss.
