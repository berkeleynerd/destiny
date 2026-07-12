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
| D-02 | Motion-state reference corpus: per-state expected trajectories (stop, linear, orbit, warp, approach, align) derived from the closed forms (`Equations.mws`) as machine-readable expectation logs | PL-11 | Active | D-01 | Corpus committed (or staged per asset rules); probe-rendered motion matches expectations within recorded tolerance. |
| D-03 | Python-free build: `DESTINY_WITH_PYTHON`-style seam guarding `src/Thunkers.cpp` (3,531 lines); Python-hosted tests ported to gtest or retired | PL-13 | Active (independent) | — | Build with the seam off links no libpython (`otool -L` clean); 73 C++ tests green; journal updated. |
| D-04 | Wire-format recovery: document the park-update stream (from `python/destiny/net` semantics and the C++ `WriteBallsToStream` path) and provide a scripted-scenario recorder | PL-33 | Blocked | D-01 | Format writeup sufficient for an independent consumer; recorder emits a deterministic stream with tick numbers and RNG seed. |
| D-05 | Oracle role for kernel succession: replay-equivalence corpora (motion, collision predicates) and exported RNG state for journaling; destiny retires to reference implementation | PL-50 | Blocked | D-02, D-04 | SPARK kernel matches destiny bit-for-bit over the corpora on same-binary replay terms; divergences adjudicated against the closed forms. |

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
