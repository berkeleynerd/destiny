# Destiny macOS Bring-Up Journal

This journal records the first macOS arm64 build of this destiny checkout and
the doctrine discovered while establishing it. It follows the conventions of
the trinity bring-up journal (`trinity/docs/macos-metal-bringup.md` in the
sibling checkout): exact commands, exact evidence, and explicit notes for
anything deferred or approximated. Doctrine here means facts and decisions
that future integration work builds on rather than re-derives.

Snapshot: 2026-07-11, commit `4136fde` (merge of keepalive-balls leak fix),
Apple Silicon host, AppleClang 21.0.0, CMake 4.4.0, Ninja 1.13.2.

## Build doctrine

1. **Destiny builds on arm64 macOS with zero source changes.** The repository
   already carries `arm64-osx-*` CMake presets, `if(APPLE)` warning guards,
   and tests that link CoreFoundation. The upstream mac pass is real; this was
   a configuration exercise, not a port.

2. **The dependency closure is small and trinity-free.**
   Direct: `carbon-blue` >= 5.1.2 (PUBLIC), `carbon-core` >= 2.6.0 (PUBLIC),
   `carbon-math` >= 1.2.1 (PRIVATE), plus `carbon-exefile-interpreter`
   (host tool, tests only) and `gtest` (tests only).
   Transitive: `carbon-blue` -> `carbon-core` + `carbon-blueexposure`;
   `carbon-blueexposure` -> `Python3 (Development)`; `carbon-core` -> `Tracy`.
   The manifest's protobuf/openssl entries are version pins for the closure,
   not direct uses.

3. **There is no build dependency on trinity, by design.** The two shared
   scene interfaces (`IEveBallpark`, `IEveReferencePoint`) are duplicated
   local headers on both sides, and the one trinity interface destiny
   implements is declared locally: `src/destiny.cpp` line 12 defines
   `BLUE_DEFINE_INTERFACE( ITriVectorFunction )` under the comment "Define
   the interfaces we use from Trinity". Blue interface identity is the whole
   contract; destiny and trinity only meet at runtime, inside whatever
   process composes them. Destiny is therefore equally usable headless
   (simulation worker) or beside a renderer.

4. **Compiler selection must be pinned on hosts with a GNAT toolchain.** The
   carbon registry toolchain (`toolchains/arm64-osx-carbon.cmake`) declares
   `CCP_TOOLSET "AppleClang"` but never sets a compiler path. A GNAT
   installation earlier on `PATH` (here: Alire `gnat_native_15.1.2`) shadows
   `/usr/bin/c++`, and its GNU `ld` fails immediately on macOS
   (`ld: library not found for -lSystem`, first observed in the vcpkg tracy
   port). Doctrine: configure with `CC=/usr/bin/cc CXX=/usr/bin/c++`; the
   environment propagates into vcpkg port builds. A local (gitignored)
   `CMakeUserPresets.json` preset `arm64-osx-debug-local` records the pin.

5. **Exact commands.**

   ```sh
   git submodule update --init --recursive
   ./vendor/github.com/microsoft/vcpkg/bootstrap-vcpkg.sh
   CC=/usr/bin/cc CXX=/usr/bin/c++ CMAKE_GENERATOR=Ninja \
     cmake --preset arm64-osx-debug
   cmake --build .cmake-build-arm64-osx-debug
   ctest --test-dir .cmake-build-arm64-osx-debug
   ```

## Evidence

- Configure completes with vcpkg building the full closure from the carbon
  registry (baseline `2027e0c9`), including the registry's patched CPython
  3.12.9 and the exefile interpreter host tool.
- Build: 78/78 targets; artifacts `_destiny_debug.so` (Blue module),
  `libdestinyStatic`, `tests/DestinyTest_debug`.
- Tests: **74/74 pass** — 73 C++ gtest cases (AABB, collision, box shapes,
  planes, triangles, Vector3d) plus `PythonTests` (6.7 s), which runs the
  `python/destiny` package under the exefile interpreter. The Python thunkers,
  blueexposure Python linkage, and interpreter hosting are all exercised on
  this host, not merely compiled.
- Warnings: 130, all `-Wformat` (`%d` given `long`) at `CCP_LOG*` call sites,
  concentrated in `src/Ballpark.cpp`. Cosmetic on LP64; fix opportunistically
  when the affected files are first touched.
- Unused-on-macOS artifacts: `destiny.rc`, `destinyI.dll` (Windows resource
  and interface DLL; not referenced by the CMake sources).

## Python doctrine

6. **Python linkage enters through `carbon-blueexposure`, not through
   destiny's own choices.** `blueexposure` hard-requires
   `Python3 COMPONENTS Development`; any Blue consumer (trinity included)
   already links the Python development libraries today.

7. **`src/Thunkers.cpp` compiles raw CPython thunkers into the module**
   (`Ballpark::Py__init__`, `PyErr_Format`, `ITaskletTimer` timing hooks). It
   is listed unconditionally in the target sources. The simulation core
   (`Ball`, `Ballpark`, collision, partition, motion evaluation) does not
   otherwise need Python.

8. **The `python/destiny/net` replication layer is pure Python and optional**
   (client/server tickers, bubble updater, park-update batching). Nothing in
   the C++ core depends on it.

9. **Posture.** PL-10 corrected the near-term assumption: Trinity already
   initializes CPython, and embedded Ballpark construction still creates
   legacy Python containers. The accepted policy is therefore
   `initialized-required`, while forbidding a Destiny module, Python methods,
   scheduler ticks, tasklets, and callbacks. Long term, excise the remaining
   containers and guard `Thunkers.cpp` behind a `DESTINY_WITH_PYTHON`-style
   seam, then provide a no-Python blueexposure variant under D-03/PL-13.

## Determinism doctrine

10. **The carbon toolchain applies `-ffast-math` (with
    `-fhonor-infinities -fhonor-nans`, and `-ffp-model=fast` on newer
    compilers) globally.** Any future bit-exact replay or third-party
    verification contract over destiny motion is incompatible with this flag
    as-is: fast-math licenses value-changing reassociation that varies across
    compilers and versions. This is a registry-toolchain setting, not a
    destiny setting. Flagged now; to be addressed only when a replay contract
    becomes real (options: per-target override for the simulation core, or a
    dedicated deterministic triplet).

11. **The motion model is otherwise determinism-friendly:** per-ball
    closed-form evaluation (see `Equations.mws`) with no cross-ball
    floating-point accumulation. `src/Random.cpp` is seeded state and must be
    part of any snapshot/journal design.

## Integration doctrine (the trinity seam)

12. Recorded here so the renderer-side and simulation-side work can proceed
    independently. In the sibling trinity checkout:
    `EveEffectRoot2`/`EveRootTransform`/`AudioGameObject` expose
    `m_ballPosition`/`m_ballRotation` Blue slots documented as "Vector
    function slot for attaching a destiny ball"; `EveSpaceScene` owns an
    `IEveBallparkPtr` and rebases its update-context origin from it each
    frame; the sun is identified by matching a planet's translation curve
    against `m_sunBall` (the authentic lens-flare linkage). Direct source
    inspection and PL-10 instrumentation establish that `UpdateOrigin`
    natively consumes only `GetReferencePoint`. `Delta`, `DeltaVel`, and
    `GetUnitBase` are separate interface methods queried by the probe for
    diagnostics.

13. **First integration contract accepted:** `destinyEmbedded` excludes the
    module entry, thunkers, `Partition_Blue.cpp`, and Python-facing exposure
    files. A minimal exposure unit registers exactly ten `_destiny` classes,
    idempotently, while the C API owns one Ballpark and one borrowed
    `ClientBall` position/rotation pair. The normal `destiny` and
    `destinyStatic` targets remain unchanged and all 74 existing tests pass.

## PL-10 first Ballpark evidence (2026-07-12)

Promised Land now owns the common dependency closure and execution gate:

```sh
cd /Users/rebecca/src/github.com/berkeleynerd/promised-land
cmake --preset arm64-osx-debug
cmake --build --preset arm64-osx-debug --target pl10_validate
```

The superbuild configures Destiny first with Trinity's manifest closure,
installs only `destinyEmbedded`, then configures Trinity against that exact
package and shared `vcpkg_installed` tree. Apple Objective-C++ does not support
CMake's `WHOLE_ARCHIVE` link feature, so Trinity uses `-force_load` to retain
the ten registrations.

The embedded contract reports ten classes, one free STOP `ClientBall` in
system `30005286`, and exactly two direct one-second evolves over frames
`0..179`. `Start`, `OnTick`, scheduler registration, tasklets, and Python
callbacks remain zero. CPython is initialized by Trinity, but `_destiny` and
`_destiny_debug` are absent from `sys.modules`.

The static fixture uses unit base `1`, maximum velocity `250`, the Astero's
CMF-derived radius, zero position/velocity, and its authored quaternion.
Native free-ball evolve applies roll-upright behavior, so PL-10 restores that
quaternion after each evolve to keep the null visual contract exact. PL-11
will remove the pin while accepting authentic orientation and motion.

## PL-11A native STOP and linear GOTO (2026-07-12)

`Destiny_CreateEmbeddedSessionWithOptions` now selects native orientation and
either the primary-ego or fixed-observer reference frame. Commands are accepted
only for the next native tick; the fixture applies one `GotoPoint` at frame 180
and reaches 19 direct evolves over 1,200 render frames without `Start`,
`OnTick`, scheduler, tasklet, or Python callback activity.

The Astero fixture uses mass `975000 kg`, radius `35 m`, maximum velocity
`312 m/s`, agility `2.87`, and starts at `(0,0,-1000)`. The committed STOP and
17-tick GOTO corpora are checked against the `Equations.mws` drag/thrust closed
form. Their provenance ledger records the exact local SharedCache paths,
sizes, and SHA-256 values for `types.fsdbinary` and `typedogma.fsdbinary`; no
client bytes are committed. Native motion overshoots the target, turns, and
reverses velocity. PL-11B native orbit is the next D-02 gate.

Evidence lives outside source control:

```text
/Users/rebecca/src/github.com/berkeleynerd/promised-land/.cmake-build-arm64-osx-debug/pl10/reports/PL10Validation.txt
/Users/rebecca/src/github.com/berkeleynerd/promised-land/.cmake-build-arm64-osx-debug/pl10/reports/ballpark-a.csv
/Users/rebecca/src/github.com/berkeleynerd/promised-land/.cmake-build-arm64-osx-debug/pl10/reports/ballpark-b.csv
```

Both CSVs have SHA-256
`1b8398e9d987732353c265432335fedb75cd8c1561af15e18ddf0735958682a8`.
Frozen off/static matrices, raw color/depth hashes, and encoded PNG pairs are
byte-identical. Origin updates equal 180; reference point, origin, shift,
deltas, and velocity remain zero. The integrated image loads exactly one
`blue_debug.so` and Trinity's existing one Python dylib, with no Destiny
module initializer, timer, thunker, Granny, or tracked EVE payload.

## PL-11B Frontier orbit contract (2026-07-12)

The embedded session now owns one fixed navigation target and queues native
`Orbit` through the existing next-tick command seam. PL-11B explicitly selects
the opt-in Frontier solver, preserves two STOP evolves, and checks 60 ORBIT
evolves against the independently generated Python-Ballpark corpus
`tests/data/pl11b-orbit-new.csv`. Ego, repeated ego, and fixed-observer runs
are bit-identical. The target remains stationary and nonrendered, and all
scheduler/Python callback counters remain zero.

Installed `code.ccp` has SHA-256
`232a2c1552cd00d030e7b9f6bf1d4956673e3c1be85f07f4b19ebe19131fa67f`.
It contains `enable_new_orbit` but no observed caller; Frontier orbit is an
explicit Promised Land choice rather than claimed installed-client policy.

## PL-12 celestial ball exposure (2026-07-12)

The embedded session now owns up to four celestial balls through
`Destiny_AddEmbeddedCelestial`, `Destiny_GetEmbeddedCelestialPosition`, and
`Destiny_GetEmbeddedCelestialState`. Celestials are created with
`AddOldStyleOrientedBall` as fixed, global, non-massive, non-interactive,
zero-velocity balls and set to `DSTBALL_RIGID` through `SetBallRigid`;
additions are accepted only before the first advance or command, with unique
nonzero identifiers distinct from the primary, ego, and navigation-target
balls. Mass is pinned to `1.0` because mass has no effect on non-free,
non-massive balls; celestial collision remains out of scope.

Because a client-mode park creates every ball as an `OClientBall`, the
celestial ball is itself the `ITriVectorFunction` position curve: a fixed
global ball returns `(mNewPos - referencePoint) / unitBase` in floats, which
at the embedded unit base `1.0` is observer-relative meters — exactly the
units Trinity's planet pass and lens flare consume. `DestinyEmbedded.h` also
mirrors `DSTBALL_STOP`/`DSTBALL_RIGID` as `DestinyEmbeddedBallMode` for
consumers that must not include destiny internals; static asserts pin the
mirror to the real enum.

`DestinyEmbeddedCelestialContractTest` proves the addition constraints, the
RIGID/global/fixed state, bit-exact positions and radii across all evolves,
curve outputs against the ego and observer reference points, and — by
running the complete PL-11B orbit fixture with and without celestials —
that celestial balls leave the accepted trajectory bit-identical. The New
Eden fixture uses authored item identifiers `40334263` (star, radius
`158,400,000 m`) and `40334264` (planet, radius `2,630,000 m`) at their
stargate-anchored solarSystemContent positions.

## D-07 embedded warp command (2026-07-12)

Warp enters the embedded seam the same way STOP, GOTO, and ORBIT did:
`Destiny_CommandEmbeddedWarp` validates against the exact next-tick time,
monotonic command ordering, and finite arguments, then queues a single
pending command that `Destiny_AdvanceEmbeddedSession` dispatches to the
recovered `Ballpark::WarpTo` on its effective tick. One contract
tightening is deliberate: `WarpTo` silently downgrades sub-100 km warps
to `GotoPoint`, and the embedded command rejects that zone outright with
a 200 km floor — a warp command always means a warp. Diagnostics expose
the warp state without leaking the sim's member reuse: the effect stamp
(negative while aligning, the warp-start tick afterward), the warp
factor and minimum range unpunned from the shanghaied `mOwnerId` and
`mFollowId` members, the total leg length from the repurposed
`mLastCollision`, and the ball's collision/sensor participation. The
three warp lifecycle events compile to no-ops in the scheduler-free
build (`DESTINY_PY_POST_EVENT` is `true` under the embedded seam), so
nothing is absorbed at runtime because nothing is emitted.

`DestinyEmbeddedWarpContractTest` proves the contract on the PL-11C
fixture leg — the stargate anchor to the New Eden planet, 9.058774 AU,
at `warpFactor 5000` (5.0 AU/s from the Astero's `warpSpeedMultiplier
5.0` already recorded in the pl11a provenance): eight align ticks to
the 75 %-of-max-velocity/8.1° gate, activation at evolve 11, dropout at
evolve 31 below 100 m/s with collision and sensor participation
restored, settling ~9,997 km short of the planet center under drag.
Every warp-phase sample is re-derived in-test from the recovered closed
forms (`SetupWarpConstants`/`WarpDistance` replicated verbatim) against
the previous sample; the 42-sample corpus (`pl11c-warp.csv` +
provenance) matches at `max(1e-5 m, 1e-12 relative)` per axis; and the
trajectory is bit-exact across two ego runs, a fixed-observer run, and
a run with the full PL-12 celestial set present.

The unit's precision finding extends the D1 doctrine: warp positions
are computed destination-relative at the ~1.4e12 m leg scale
(`p = goto − remaining·dir`), so every sample inherits that scale's
~2.4e-4 m double ULP regardless of its own magnitude — near-origin
early-warp samples quantize at ~1e-4 m granularity, and any
independent re-derivation agrees to a few ULP of the LEG scale, not of
the sample. The closed-form gate therefore uses a 5e-3 m floor
(~20 ULP at leg scale) with 1e-9 relative; absolute PL-11A-style 5e-10
gates are structurally impossible at warp magnitudes and are not
claimed. Run-to-run determinism remains bit-exact, which is the strong
contract.

### CP-37b consumer addendum (2026-07-12)

Trinity's warp integration surfaced one API-surface gap: embedded
consumers gate warp-phase behavior on the ball mode, and the
`DestinyEmbeddedBallMode` mirror only carried `STOP`/`RIGID`. The
mirror gained `DESTINY_EMBEDDED_BALL_MODE_WARP = 3` so consumers read
`DSTBALL_WARP` without including destiny internals. No behavioral
change; the trinity probe's three PL-11C lanes (ego, repeat ego,
fixed observer) validate against this header with bit-exact corpus
scoring and a shared trajectory hash.

## D-08 embedded route commands and celestial orbit (2026-07-13)

The EVE Gate round-trip consumer needed two existing Ballpark operations
without including destiny internals. `Destiny_CommandEmbeddedGotoDirection`
now validates a finite nonzero direction on the exact next tick and dispatches
it to `Ballpark::GotoDirection`; public ball-mode mirrors now include GOTO and
ORBIT as well as STOP, WARP, and RIGID. The motion contract rejects zero and
out-of-order direction commands.

Orbit targeting is no longer restricted to the session's synthetic fixed
navigation ball. Any registered celestial may be selected, and the session
tracks the actual orbit target for relative-state evolution and diagnostics.
`DestinyEmbeddedCelestialContractTest` commands a native orbit around the Sun
ball and observes ORBIT mode with the correct target ID. Trinity uses these
seams to align to the EVE Gate, warp there, align back to the Sun, warp back,
and enter a 2,500 m surface-range Sun orbit; warp execution remains the D-07
`Destiny_CommandEmbeddedWarp` to `Ballpark::WarpTo` path.

## D-03 Python seam (2026-07-14)

`DESTINY_WITH_PYTHON` now separates the simulation core from the CPython
thunker surface (doctrine: `docs/destiny-python-seam.md`). The flag defaults
to 1 in `src/StdAfx.h`; `destinyEmbedded` builds with it at 0 and its import
surface is pinned by the executable `DestinyEmbeddedSymbolGate` to a
15-symbol whitelist whose every entry is Blue-ABI interop, not
destiny-authored CPython calls (`BLUE_WITH_PYTHON` must stay 1 for
`Be::ClassInfo` layout parity with `blue.so`).

The excision gates `Thunkers.cpp` whole, every `MAP_METHOD` table, and the
in-file thunkers of Ball, Ballpark, the mini shapes, Partition, Box, and
GlobalSettings. Two subsystems were simulation-coupled and received
statement-mirrored C++ twins instead of stubs: the bubble interactive-count
bookkeeping (`InDeadBubble` gates Evolve; `GetInteractiveCnt` feeds
`Partition.cpp`) now runs on `std::unordered_map` under the seam, and the
keep-alive/journal/subscription surfaces reduce to their count updates
because their only writers were Python.

Evidence, both ways: seam-off, all seven embedded contract tests green with
corpus pins byte-identical (warp align 8 / activation 11 / dropout 31 at
9.058774 AU; approach min-center 37.360883 m, final 568.299963 m) and the
PL-11A/B/C/D lanes re-derive their pinned trajectory hashes; seam-on, the
classic bringup tree rebuilds and passes 74/74 tests including the
interpreter-hosted Python suite. `test_ball`, `test_cleanup`, and
`test_configure` are ported to `tests/DestinyEmbeddedSeamTest.cpp`
(mini-shape counts, Blue live-count hygiene across session create/destroy,
settings-global projection/restore); `test_using_correct_module` is retired.
Embedded test translation units now compile with `DESTINY_WITH_PYTHON=0` and
`DESTINY_EMBEDDED=1` so they see the archive's exact ABI (`MapOfBalls` and
the Ballpark bubble members differ across the seam).
