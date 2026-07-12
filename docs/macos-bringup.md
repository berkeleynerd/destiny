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
