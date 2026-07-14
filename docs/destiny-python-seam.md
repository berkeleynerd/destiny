# The DESTINY_WITH_PYTHON Seam (D-03)

`DESTINY_WITH_PYTHON` separates destiny's simulation core from its CPython
thunker surface. It defaults to `1` (classic builds are unchanged); the
`destinyEmbedded` target builds with `DESTINY_WITH_PYTHON=0` and imports no
CPython symbols beyond a pinned interop whitelist. The seam is the staging
ground for the successor host boundary (Ravenscar/Jorvik): what the flag
excises is exactly what the successor must consciously adopt, re-shape, or
reject — inventoried in [thunker-contract-audit.md](thunker-contract-audit.md).

## Flag semantics

- `DESTINY_WITH_PYTHON=1` (default, set in `src/StdAfx.h`): the full Python
  thunker surface compiles — classic `_destiny` module behavior, bit for bit.
- `DESTINY_WITH_PYTHON=0` (the `destinyEmbedded` archive): every
  destiny-authored CPython *call* is compiled out. Types, pointer members,
  and declarations stay — only calls emit `_Py*` imports.

## The ABI ruling

destiny must keep **`BLUE_WITH_PYTHON=1`**: `Be::ClassInfo` carries a
conditional `PyTypeObject* mTypeObject` member (BlueTypes.h), so flipping
Blue's flag would desynchronize struct layouts against `blue.so`, which is
compiled with Python and *is* the embedded interpreter provider. The seam
therefore gates destiny's calls, never Blue's headers. Three consequences,
visible as the sanctioned import whitelist
(`tests/data/embedded-python-symbol-whitelist.txt`):

1. `_Py_IsInitialized` — the D-01 host contract: sessions refuse to start
   until the host has initialized CPython for Blue
   (`DestinyEmbedded.cpp`, under `#if BLUE_WITH_PYTHON`).
2. `_PyType_Type` — Blue's `EXPOSURE_BEGIN_IMP` instantiates a static
   `PyTypeObject` headed by `&PyType_Type`; this is the class-registration
   ABI itself (`DestinyEmbeddedBlue.cpp` and the `*_Blue.cpp` exposure files).
3. The BlueList template set (12 symbols) —
   `BlueList_Impl<MiniBall/MiniBox/MiniCapsule>` instantiations emit weak
   copies of Blue's `PyRepr`/`PyDebugExpand`/`PyDebugCollapse` helpers,
   guarded by Blue's own `BLUE_WITH_PYTHON`. Debug/repr plumbing only; never
   reachable on the simulation path.

The executable gate (`DestinyEmbeddedSymbolGate`,
`tests/check-embedded-python-symbols.sh`) diffs `nm -u` output against the
whitelist — any new `_Py*` import fails the build's test run.

## Simulation-coupled twins

Most gated code is bridge-only (argument marshaling, event formatting). Two
subsystems feed the simulation and required C++ twins under seam-off, with
logic mirrored statement-for-statement so the corpora stay bit-neutral:

- **Interactive-count bookkeeping** (`Ballpark.cpp`): `InDeadBubble` gates
  Evolve, `GetInteractiveCnt` feeds `Partition.cpp`. Seam-off these run on
  `std::unordered_map<long, std::unordered_set<long long>>
  bubbleInteractives` (`Ballpark.h`). The transition journal (`bubbles`),
  bubble subscriptions, and keep-alive registries are host-facing surfaces
  with no Python writer under the seam — they stay empty and their functions
  reduce to the count updates.
- **Mini-shape lists** (`Ball.h`): untouched — they were always C++
  (`BlueList` members); only the Py marshaling wrappers gated.

Layout note: `Ballpark` (bubble members) and `MapOfBalls` differ between
seam-on and seam-off. Every test translation unit that links
`destinyEmbedded` compiles with `DESTINY_WITH_PYTHON=0` and
`DESTINY_EMBEDDED=1` so it sees the archive's exact ABI (enforced in
`CMakeLists.txt`).

## Python test triage

- `test_ball`, `test_cleanup`, `test_configure` → ported to
  `tests/DestinyEmbeddedSeamTest.cpp` (mini-shape add/count/cleanup, Blue
  live-count hygiene across session create/destroy, settings-global
  projection and restore). The Python originals remain for classic seam-on
  runs until the ON-mode sunset.
- `test_using_correct_module` → retired (module-path hygiene for the Python
  package layout; meaningless at the embedded seam).
- The Python-attribute default-value assertions in `test_ball` are exposure
  contract, not simulation contract — inventoried as `[TRIVIAL]` in the
  thunker audit for the successor's exposure layer.

## Sunset

Classic seam-on mode (`DESTINY_WITH_PYTHON=1`, the `_destiny` module and its
thunkers) is transitional scaffolding: it exists so the corpus-pinned classic
behavior stays runnable while the embedded seam and its successor
(Ravenscar/Jorvik) take over host duties. New host-facing capability lands on
the embedded C API, not the thunker surface. When the successor seam is
accepted, seam-on mode and the remaining Python tests retire together.
