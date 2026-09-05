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
