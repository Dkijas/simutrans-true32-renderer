# TRUE32 candidate - what each production file changes

Against SVN trunk r12254. 18 files: 12 modified, 6 new. Patch:
`patches/true32-r12254.diff` (47 hunks, +4424/-771).

## New files (`src/simutrans/display/`)

- `simgraph32.cc` (about 3600 lines) - the ARGB8888 renderer: the full
  `simgraph_t` function table over a 32-bit framebuffer and 32-bit
  per-player image caches (recode from the 16-bit stored images, zoom
  cache, dirty tiles, multi-lane clip state, screenshot, system colours).
  Includes the three lifecycle fixes from REMEDIATION-01: the recode mutex
  with the re-check under the lock, the resize epilogue (clip reset + full
  dirty) and the clamping `set_clip_rect`.
- `simgraph32.h` - its declaration.
- `blit_core.h` - renderer-independent image run-stream traversal, over a
  pixel policy (`Ops::screen_t`, `Ops::copy`, ...).
- `blit_clip_core.h` - the clipped traversal (clip rectangles, lanes).
- `zoom_core.h` - zoom-level image generation shared by both depths.
- `simgraph_palette.h` - palette / special-colour tables shared by both.

## Modified files

- `display/simgraph16.cc` (8 hunks) - refactored to use the shared cores;
  output frozen by the 29-scenario hash, equal to trunk.
- `display/simgraph.h` (1) / `display/simgraph.cc` (2) - the renderer
  function table and selection; the 32-bit renderer registered.
- `simcolor.h` (3) - the pixel-type contracts: `PIXVAL` / `FLAGGED_PIXVAL`
  per depth, `STORED_PIXVAL` (16), `SAVED_PIXVAL` (32), `NETWORK_PIXVAL`
  (16), and the colour helpers for the 32-bit word.
- `sys/simsys.h` (2) - the framebuffer/texture interface typed with
  `PIXVAL`.
- `sys/simsys_w.cc` (5) - GDI: `PIXVAL`-typed DIB, depth-aware bit count,
  `BI_RGB` at 32 bpp (555/565 branches unchanged).
- `sys/simsys_s2.cc` (5) - SDL2: ARGB8888 streaming texture at 32; the
  texture lock is not held across the frame at 32.
- `sys/simsys_s3.cc` (6) - SDL3: the same for SDL3.
- `descriptor/reader/image_reader.cc` (1) and `descriptor/ground_desc.cc`
  (6) - the stored pixel type named explicitly; readers stay 16-bit.
- `dataobj/gameinfo.cc` (1) and `simmesg.cc` (1) - use the screen-colour /
  saved-pixel helpers instead of assuming a 16-bit screen word.

## Build files (v2 patch, CMAKE-MSVC-01; not part of the frozen renderer)

- `CMakeLists.txt` (+27/-7) - cache variable `COLOUR_DEPTH` (16 default,
  32), validated for the graphical backends; the sdl2/sdl3/gdi branches
  compile `src/simutrans/display/simgraph${COLOUR_DEPTH}.cc` and define
  `COLOUR_DEPTH=${COLOUR_DEPTH}`; `none` keeps 0; any other value is a
  configuration error.
- `Simutrans-GDI.vcxproj`, `Simutrans-SDL2.vcxproj`, `Simutrans-SDL3.vcxproj`
  (+11/-4 each) - MSBuild property `COLOUR_DEPTH` (16 unless given), used
  by the preprocessor definitions and the renderer `ClCompile` item; a
  target `SimutransCheckColourDepth` rejects other values before compiling.
- `Simutrans-GDI.vcxproj.filters` (+1/-1) - the renderer item follows.

## Separate auxiliary patch (not in the candidate)

- `patches/gdi-resize-event-delivery-r12254.diff` (`sys/simsys_w.cc` 3 hunks, `simevent.cc`
  1 hunk) - the certified fix for the Windows backend resize-event delivery defect. Independent
  of the renderer; published separately so that v2 stays the canonical TRUE32 patch. Not in SVN.
- `patches/gdi-dpi-windowsize-r12254.diff` (`sys/simsys_w.cc` 1 hunk, +8/-2) - the certified fix
  for the `WindowSize` physical/logical unit contract, the root cause of the DPI-scaling failure
  of forum topic 23805. Certified on top of the resize-event patch, which is a prerequisite of
  that configuration. Also independent of the renderer, also not folded into v2, also not in SVN.

## What is deliberately not in the candidate

- no Makefile change (it already selects `simgraph$(COLOUR_DEPTH).cc`)
- no pak, makeobj, savegame or network format change
- no laboratory instrumentation (the certification hook lives outside the
  candidate, behind `-DSTLAB_CERT_HARNESS`, and is not published)

## History (summary; details in `reports/`)

1. Foundation and core blitters at 32 bits, identity-gated against the
   16-bit renderer (U4, U5).
2. Shared cores extracted so that `simgraph16.cc` and `simgraph32.cc`
   traverse images with one implementation; the 16-bit output frozen
   by hash (SC01-SC03).
3. Coloured/day-night, zoom, blend/alpha/outline, remaining image paths,
   drawing primitives, text, GUI state and surface, image dirty contract,
   backend closure, API residual, threading (U6-U10, UR1).
4. SDL system colours and the 32-bit SDL2/SDL3 backends enabled.
5. First full product certification: three lifecycle defects found
   (CERTIFICATION-01, verdict C).
6. Remediation of exactly those three (REMEDIATION-01, verdict B).
7. Full product recertification of the frozen result
   (RECERTIFICATION-02, verdict B, PRODUCT_CANDIDATE).
