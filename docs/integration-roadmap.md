# Destiny Integration Roadmap

This tracker is authoritative for destiny-side work in the wider program.
Cross-cutting gates (PL-numbers) live in the program repository
(`promised-land`, a sibling checkout, currently local-only); the trinity
domain tracker is `trinity/docs/eve-runtime-contract-roadmap.md` in the
sibling trinity checkout. Where this file and the program tracker disagree
about destiny facts, this file wins and the program tracker gets corrected.

Bring-up history and standing doctrine live in
[`macos-bringup.md`](macos-bringup.md); this file tracks what destiny must
do and provide, gate by gate.

## Status notation

| Mark | Meaning |
| --- | --- |
| Accepted | Contract and evidence gate passed. |
| Partial | Some real inputs are present, but the contract is incomplete. |
| Active | Current direct-path work. |
| Blocked | A named prerequisite has not been accepted. |
| Capability only | Machinery executes, but its output is not acceptance evidence. |

## Gates

| ID | Destiny-side contract | Program gate | Status | Depends on | Evidence required |
| --- | --- | --- | --- | --- | --- |
| D-00 | macOS arm64 build and test acceptance | PL-02 | Accepted | — | 78/78 targets, 74/74 tests; `macos-bringup.md`. |
| D-01 | Consumable embedded package: curated `destinyEmbedded` static target, ten-class module-free Blue registration, real Ballpark/ClientBall C API, explicit scheduler-free advance, and install/export closure for `carbon-blue`, `carbon-core`, and `carbon-math`. | PL-10 | Accepted | D-00 | Promised Land consumes the installed package through `find_package`, force-loads it into Trinity, and passes two 180-frame null tests with one Blue runtime, no Destiny Python module/timer/thunker symbols, exactly two evolves, and byte-identical off/static outputs. |
| D-02 | Motion-state reference corpus: per-state expected trajectories (stop, linear, orbit, warp, approach, align) derived from the closed forms (`Equations.mws`) as machine-readable expectation logs | PL-11 | Accepted | D-01 | PL-11A accepts STOP/GOTO. PL-11B accepts explicitly selected Frontier orbit against a 60-tick Python-Ballpark corpus. D-07 accepts warp and the align-for-warp phase against a 44-tick embedded corpus with in-test closed-form re-derivation, and PL-11C accepts the same corpus cross-repository in the trinity render seam (four lanes plus the celestial control, trajectory `56c3973a912c8b26`), completing the PL-11 motion-state set. D-08 accepts approach (the client's Approach/Keep-at-Range verb, `FollowBall` → `DSTBALL_FOLLOW`) against a 60-tick embedded corpus with in-test closed-form re-derivation and a station-keeping contract (mode never leaves FOLLOW), validated cross-repository under PL-11D (trajectory `4b2ab48907c53458`). Every D-02 motion state — stop, linear, orbit, warp, approach, and the align-for-warp phase — is now accepted. |
| D-03 | Python-free simulation core: `DESTINY_WITH_PYTHON` seam gating `src/Thunkers.cpp` (3,531 lines) and every in-file thunker; Python-hosted tests ported to gtest or retired | PL-13 | Accepted | — | `destinyEmbedded` builds seam-off with its CPython import surface pinned to a 15-symbol Blue-ABI whitelist by the executable `DestinyEmbeddedSymbolGate` (`BLUE_WITH_PYTHON` stays 1 for `Be::ClassInfo` layout parity with `blue.so`, so process-level libpython removal is PL-42's charter, not D-03's). Simulation-coupled bubble bookkeeping runs on statement-mirrored C++ twins; all seven embedded contract tests and the PL-11A–D lanes hold their corpus pins seam-off; the classic seam-on tree stays 74/74 green. `test_ball`/`test_cleanup`/`test_configure` ported to `DestinyEmbeddedSeamTest`; `test_using_correct_module` retired. Doctrine: `docs/destiny-python-seam.md`. |
| D-04 | Wire-format recovery: document the park-update stream (from `python/destiny/net` semantics and the C++ `WriteBallsToStream` path) and provide a scripted-scenario recorder | PL-33 | Blocked | D-01 | Format writeup sufficient for an independent consumer; recorder emits a deterministic stream with tick numbers and RNG seed. |
| D-05 | Oracle role for kernel succession: replay-equivalence corpora (motion, collision predicates) and exported RNG state for journaling; destiny retires to reference implementation | PL-50 | Blocked | D-02, D-04 | SPARK kernel matches destiny bit-for-bit over the corpora on same-binary replay terms; divergences adjudicated against the closed forms. |
| D-07 | Embedded warp command: `Destiny_CommandEmbeddedWarp` queues `Ballpark::WarpTo` through the accepted next-tick seam with warp-state diagnostics (effect stamp, factor, minimum range, total distance, collision/sensor participation) unpunned from the shanghaied ball members | PL-11C | Accepted | D-01, D-06 | `DestinyEmbeddedWarpContractTest`: preconditions reject wrong-tick, degenerate-factor, and sub-200 km (silent-downgrade-zone) commands; the 9.06 AU stargate-to-planet leg at warpFactor 5000 (Astero `warpSpeedMultiplier 5.0`, pl11a provenance) aligns in 8 ticks, activates at evolve 11, and drops out at evolve 31 below 100 m/s with collision and sensor participation restored; every warp-phase sample re-derives from the recovered closed forms within 20 ULP of the leg scale; the 42-sample corpus matches at `max(1e-5 m, 1e-12 rel)`; trajectories are bit-exact across ego/ego/observer runs and with the PL-12 celestial set present; warp events compile to no-ops in the scheduler-free build. |
| D-08 | Embedded route commands and celestial orbit targets: next-tick `GotoDirection`, public GOTO/ORBIT mode mirrors, and native `Orbit` against any registered celestial | PL-11C/PL-12 | Accepted | D-06, D-07 | Motion tests reject zero/out-of-order directions and prove a valid direction enters GOTO with motion on the requested axis; celestial tests command ORBIT around the registered Sun and verify target diagnostics. Trinity's finite route aligns and warps to the EVE Gate, aligns and warps back to the Sun, then enters native Sun orbit. |
| D-06 | Celestial ball exposure: fixed/global RIGID celestial balls with per-ball position curves and state queries in `destinyEmbedded` | PL-12 | Accepted | D-01 | `DestinyEmbeddedCelestialContractTest` proves addition constraints, RIGID/global/fixed state, bit-exact placement across evolves, ego/observer curve outputs, a bit-identical PL-11B orbit trajectory with celestials present, and native ORBIT targeting of the registered Sun. Journal: `macos-bringup.md` PL-12 and D-08 sections. |

## Scope and succession notes

- **D-01's Python policy is deliberately narrow.** Trinity continues to
  initialize CPython because Ballpark still creates legacy Python containers.
  The embedded target creates no Destiny module, exposes no Python methods,
  and performs no scheduler, tasklet, `OnTick`, or callback work. Removing the
  remaining container dependency belongs to D-03/PL-13.
- **The PL-10 STOP quaternion is a fixture constraint.** Native free-ball
  evolve applies roll-upright behavior even with zero angular velocity, so the
  embedded null test restores the authored quaternion after each evolve.
  D-02 removes that pin as each authentic motion/orientation state is accepted.
- **PL-11A removes the pin for STOP and GOTO.** The committed 17-tick GOTO
  corpus reproduces the analytic drag/thrust integration, including native
  overshoot, turn-gated thrust, and velocity reversal. Both reference frames
  produce trajectory hash `8ee43851a6c2f115`; raw errors remain below
  `5e-10`. Native roll decreases from `0.785398` to `0.562532` radians.
- **PL-11B explicitly selects Frontier orbit.** The checkout defaults to
  legacy orbit and installed `code.ccp` contains the opt-in helper but no
  observed caller. The 60-tick corpus records PL policy, not a recovered
  current-client setting.

- **No gameplay-scope trim exists here, by design.** Of 21,310 C++ lines:
  core motion (Ball/Ballpark/IBall) 11,203 is entirely 1.0-vintage;
  collision+shapes 3,247 mostly stays (stations/gates needed docking
  geometry from launch); infrastructure 1,663 stays. The strippable ~25%
  is D-03's thunkers (3,531) plus Blue exposure boilerplate (1,838) —
  architectural removals, not scope. Post-strip: ~15.9k lines.
- **The succession target is smaller than the codebase:** ~8–12k lines of
  semantics (motion evaluation, tick orchestration, collision predicates).
  Container/partition infrastructure is reimplemented idiomatically in the
  successor, and the motion math ports from the `Equations.mws` closed
  forms rather than from code. `python/destiny/net` (9,470 lines) is
  superseded by the program's journal/exporter design, not ported.
- **Determinism caveats carried from doctrine** (`macos-bringup.md` items
  10–11): the carbon toolchain's global fast-math is acceptable for the
  C++ oracle on same-binary replay terms but must not leak into successor
  builds; `src/Random.cpp` state is part of any snapshot/journal contract.

## Evidence policy

As in the journal: exact commands, finite runs with status 0, A/B against
the previously accepted state, explicit notes for anything approximated or
deferred. Consumer-side captures (the probe's byte-identical placement
test for D-01) live in the trinity tree under its conventions; this
tracker records the destiny-side artifacts and links the acceptance.
