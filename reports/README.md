# Reports

These are the canonical laboratory reports of the TRUE32 programme,
copied verbatim except that local machine paths were replaced by
`<lab>/`, `<home>/` and similar placeholders. They were written for an
internal certification process, so they are long and use its vocabulary
(cuts, gates, lanes, verdict letters A-F); the summaries in `docs/` are
the readable entry point.

## Authoritative

- **RGBA32-FULL-RUNTIME-RECERTIFICATION-02.md** - the final product
  certification of the frozen candidate. Verdict B: TRUE32 PRODUCT
  CERTIFIED WITH DOCUMENTED ENVIRONMENT LIMITATION;
  `RGBA32_RENDERER_STATUS = PRODUCT_CANDIDATE`. Everything in
  `docs/CERTIFICATION.md`, `docs/PERFORMANCE.md` and
  `docs/LIMITATIONS.md` comes from this report.
- **RECERTIFICATION-02-provenance.txt** - candidate identity, every
  laboratory binary's hash, paksets and toolchains of that certification.

## The progression that led to it

- **U4-RGBA32-FOUNDATION-REBASE-01.md** - the TRUE ARGB8888 foundation
  (pixel-type contracts, framebuffer, screen colours) rebased on the
  sanitised trunk.
- **U5-RGBA32-BLITTER-REBASE-01.md** - the core blitter family at 32 bits.
- **SC03-RGBA32-SHARED-CORE.md** - the shared traversal/clipping/zoom
  cores used by both renderers, with the 16-bit output frozen by hash.
- **U9-RGBA32-BACKEND-CLOSURE-01.md** - the backend interface closed
  (system colours, screenshot, texture contract).
- **SDL32-BACKEND-ENABLEMENT-01.md** - SDL2 and SDL3 presenting a 32-bit
  framebuffer.
- **RGBA32-FULL-RUNTIME-CERTIFICATION-01.md** - the first full product
  certification (verdict C): three lifecycle defects found and narrowed,
  deliberately not fixed there.
- **RGBA32-RUNTIME-REMEDIATION-01.md** - the three defects closed with
  before/after evidence and negative controls (verdict B); its output is
  the candidate.

Not included: the other laboratory cuts (flags, persistence, coloured /
day-night, zoom, blend, primitives, text, GUI, dirty contract, threading,
API residual, SDL system colours) - their gates are re-run and reported
as "internal identity" inside the recertification, which is sufficient
for review. They can be provided on request.

## Build-system integration (after the renderer certification)

- **RGBA32-CMAKE-MSVC-01.md** - CMake and Visual Studio renderer selection
  by `COLOUR_DEPTH`, certified as build-file changes only (v2 patch); the
  18 renderer files unchanged.

## Android build path (after the build-system integration)

- **RGBA32-ANDROID-BUILD-DEPTH-01.md** - the Android build chain is
  Gradle -> CMake -> Simutrans' root CMakeLists (so the v2 `COLOUR_DEPTH`
  rule applies: default 16, opt-in 32); the old `AndroidBuild.sh` is not
  used; a TRUE32 x86_64 APK built, installed and rendered on the emulator;
  internal framebuffer precision left UNVERIFIED there.
- **RGBA32-ANDROID-FRAMEBUFFER-CERT-01.md** - closes that: the raw
  framebuffer captured at the SDL3 present boundary on the emulator is
  genuine ARGB8888 (99.90 % of the pixels outside the RGB565 grid, the
  decisive colour 0xFF123456 present) while the 16-bit build captured the
  same way is 0.00 % outside; a quantised negative control is rejected by
  the oracle. Physical devices and ARM ABIs not run.

## Separate: Windows GDI resize-event delivery

- **GDI-RESIZE-EVENT-DELIVERY-CERT-01.md** - a pre-existing Windows backend / shared
  event-queue defect (newest client size lost in the single GDI event slot, or reverted by a
  re-queued obsolete resize), its CURRENT_STATE contract, the certified two-file fix
  (`patches/gdi-resize-event-delivery-r12254.diff`), mutants, SDL2/SDL3 controls and the
  302 / 302 suite. Not part of TRUE32 v2; not in SVN; does not fix the DPI defect of topic 23805.
- **GDI-DPI-WINDOWSIZE-CERT-01.md** - the DPI half: `WindowSize` must hold physical client
  pixels, `dr_textur_resize` wrote logical ones, and at scaling other than 100 % the next repaint
  shrinks the DIB height so the bottom third stops being painted (forum topic 23805). Certified
  one-hunk fix on top of the resize-event patch, with the contract oracle, the visible 150 %
  runs, the negative control, SDL2, 302 / 302 and the three-way composition with TRUE32 v2.

## XR32 DAY+NIGHT, r12377 testing-2

- **XR32-CONSOLIDATION-01.md** - the consolidation of the r12377 testing-1 build that became
  testing-2: five defects of the 32-bit renderer reproduced, fixed and certified against
  testing-1 on Windows SDL2/SDL3 and Linux SDL2/SDL3, each with a control that fails (shared
  images A3, interface images at night A5, zoom geometry A1/A2, base-size interface draw N1,
  non-zoomable placement N5), A6 not reproduced, Linux SDL3 in a WSLg session, the performance
  and memory comparison, and the architecture review. A dated publication note on top records
  the certification of the published packages themselves.
- **XR32-CONSOLIDATION-01-INCIDENT-MATRIX.md** - the incident matrix (A1-A8, N1-N5) with
  status, root cause, fix and evidence.

## XR32 DAY+NIGHT, r12377 testing-3

- **XR32-DIRTY-REGIONS-CERTIFICATION-01.md** - the certification of testing-3: testing-2 plus a screen update that
  sends only the changed 16x16 tiles (simgraph16's walk), draws the screen texture without blending, and reports light /
  company-colour changes and filled rectangles as changed (a missing dirty mark in `draw_rect` was found by the oracle).
  Verdict B, certified with limitations (WSLg is not a native Linux desktop). A dated publication note on top.
- **xr32-dirty-regions-cert-01/** - its companion reports: `FRAMEBUFFER-INTEGRITY-REPORT.md` (the read-back oracle on
  Windows and Linux, SDL2 and SDL3, with its controls and the trunk's theme-switch staleness T1),
  `XR32-REGRESSION-MATRIX.md` (testing-2's battery, identical values), `PATCH-REVIEW.md` (the four changes),
  `PLATFORM-VALIDATION-MATRIX.csv` and `BENCHMARK-COMPARISON.csv`.

## XR32 DAY+NIGHT, r12377 testing-4

- **xr32-a4/** - the two certifications of testing-4's only change, A4: the water of sloped coasts, which the game
  generates from the sea picture, now uses the pakset's 32-bit sea.
  - `a4/`: the first candidate, with the root cause, the design and its alternatives, the oracles, the regression
    matrix and the benchmarks.
  - `a4c/`: the testing-4 code, with a per-stage texture cache that gives byte-identical pictures at a 4–5 times lower
    rebuild cost, and the zoom 2/1 sand-gap finding (known issue A11).

## XR32 DAY+NIGHT, r12377 testing-5

- **xr32-testing-5/** - the Windows x64 SDL2 + SDL3 certification of testing-5 (translucent company colours + the zoom
  2/1 coast fix A11): the A11 root cause and fix, the integration of the certified company-colour patch, the test matrix
  with its controls, and performance. Verdict B, certified with limitations. The Linux builds of testing-5 are
  experimental and not certified.
- **xr32-tcc/** - the certification of the company-colour fix (TCC-01 and its consolidation).

## XR32 DAY+NIGHT, r12377 testing-6

- **xr32-testing-6/** - the Windows x64 SDL2 + SDL3 certification of testing-6 (A12: the generated terrain at 8-bit
  precision): the integration and provenance of the code, the visual comparison and brightness analysis, the test
  matrix with its controls, performance (the light-change and zoom-step cost), and the 32-level transition masks.
  Verdict B, certified with limitations. The Linux builds of testing-6 are experimental and not certified.
- **xr32-a12/** - the A12 work: the root cause (the terrain tiles were composed from 16-bit copies), the architecture
  options and the implementation.
