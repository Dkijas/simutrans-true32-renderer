# Patch review — XR32 dirty-region upload (testing-3 candidate)

Starting patch: `<texture-perf-lab>/deliver/patches/01-XR32-DIRTY-TILE-UPLOAD-ON-TESTING-2.diff`
(SHA-256 `1cc0a6fb…`, 3 files, +106 / −1). Split here into three commits (together exactly the starting patch's tree
`3bac7fa9`), then **a fourth change, D, found by this mission's oracle**. Branch `dr/candidate` (from testing-2's source
`fb5e8a5` = tree `3a194065`), final tree **`e0c48441`**, 3 files, +123 / −14.

| Order | Commit | Change | Files | Lines |
| --- | --- | --- | --- | --- |
| 1 | `2d9905d` | **C** — invalidation on light and company-colour changes | `simgraph32.cc` | +2 |
| 2 | `0de51bc` | **B** — screen texture unblended | `simsys_s2.cc`, `simsys_s3.cc` | +8 |
| 3 | `0671565` | **D** — `draw_rect` marks the tiles it fills (new in this mission) | `simgraph32.cc` | +5 / −13 |
| 4 | `df2a1b0` | **A** — partial texture update (dirty tiles) | `simgraph32.cc` | +108 / −1 |

No file outside the XR32 32-bit renderer and the two SDL backends' 32-bit branches is touched: `simgraph16.cc`, makeobj,
the 16-bit build paths and the trunk patches B/C/D/E/G/H are unchanged; B is inside `#if COLOUR_DEPTH == 32`.

## C — invalidation on lighting / colour changes

`simgraph32_set_daynight_level` now calls `simgraph32_mark_screen_dirty()` when the night level changes, and
`simgraph32_set_player_color_scheme` when a company colour scheme changes — **at the same places as
`simgraph16_set_daynight_level` / `simgraph16_set_player_color_scheme`** (the colour mark in the outer block, after
the day/night player handling, as simgraph16 has it after `recode()`).

- Why it is needed: every picture changes with the night level or a company colour, but nothing in the world marks it
  dirty — the renderer is expected to. testing-2 never showed the omission because it uploads the whole screen every
  frame. With A alone, the first natural light change stays on screen unrefreshed (measured in the previous mission:
  ~1.9 M px; control `lab/noC` repeats it here, `FRAMEBUFFER-INTEGRITY-REPORT.md`).
- On its own (without A) C has no visible effect: a full-screen dirty mark costs nothing when everything is uploaded.

## B — screen texture unblended

`SDL_SetTextureBlendMode(screen_tx, SDL_BLENDMODE_NONE)` right after the screen texture is created, 32-bit only, in both
backends (the texture is re-created on resize through the same function, so the mode follows it).

- Why: the screen texture is ARGB8888 and SDL alpha-blends a texture with an alpha channel by default. The framebuffer
  is opaque by intent — simgraph32 resolves every image's transparency (alpha images, semi-transparent icons, night
  lights) into the framebuffer's colours — but a few framebuffer pixels keep an alpha byte ≠ 0xFF (~55 per frame at
  1080p with traffic); on SDL3 they reached the screen mixed with whatever the render target held. Measured on testing-2
  in this mission: SDL3 1,022–2,838 px per 120 verified frames on Windows (D3D11) and 1,752–4,038 on Linux (software,
  OpenGL), up to 103,820 in scenarios that draw such pixels on purpose (`windows`, `edges`); SDL2 (D3D9 on Windows,
  OpenGL on Linux X11 and Wayland) showed 0. With B: 0 everywhere.
- Transparency semantics are preserved: the blend mode only governs how the finished framebuffer is composited onto the
  window; image alpha never reaches SDL. Checked by the alpha / MRES / sea gates (`XR32-REGRESSION-MATRIX.md`).
- Not done: tracing which drawing path leaves alpha ≠ 0xFF. It has no other consumer than this composition; a
  follow-up, not needed for correctness.
- Independent of A and C.

## D — `draw_rect` honours its dirty flag (found here)

`simgraph32_draw_rect` declared its dirty parameter without a name and filled the rectangle itself, so **nothing it
drew was ever marked dirty**. `simgraph16_draw_rect` hands the rectangle to `display_fb_internal`, which marks it; D
makes simgraph32 do the same with `display_fb_internal32` (simgraph32's existing transposition), with simgraph16's
limits `(0, disp_width, 0, disp_height)`; the null-framebuffer guard stays. Same fill, same colour; the right clip moves
from the visible width to the pitch, as in simgraph16 (the extra columns are padding, never presented).

- How it was found: the oracle's `edges` scenario (rectangles drawn with `dirty = true` just before the flush) showed
  none of them on screen with the starting patch (5,605 px); testing-2 (whole-screen upload) did not. With D: 0 px
  (`FRAMEBUFFER-INTEGRITY-REPORT.md`).
- Not fixed by D, and not to be fixed here: after a live theme switch two toolbar elements stay stale (up to 850 px:
  768 px where the compass is drawn, which `gui/simwin.cc:2232` draws with `dirty = false`, and an 82-px element at
  x 10–38, y 96–116). The official r12377 16-bit renderer leaves the identical pixel positions stale in the identical
  run (picture dumps: 11 of 11 850-px sets and 5 of 5 82-px sets identical across both builds). Trunk behaviour,
  documented as known issue T1 of testing-3.
- Audit: of the 19 simgraph16 functions that take a `dirty` flag, `draw_rect` is the only one whose simgraph32
  counterpart dropped it (`tools/audit_dirty_params.py`, `evidence/audit-dirty-params.txt`). The circle function's
  commented-out mark is identical in simgraph16 (trunk behaviour) and is left as it is.
- On its own D changes nothing visible (whole-screen upload); it is required by A.

## A — partial texture update

`simgraph32_flush_framebuffer` calls `flush_dirty_tiles()`: simgraph16's dirty walk, **taken textually** from
`simgraph16_flush_framebuffer` (helper renamed `dirty_lowest_set_bit`, `ALLOCA` replaced by a `std::vector`, the
`DEBUG_FLUSH_BUFFER` drawing dropped). Current and previous frame's dirty tiles are merged into rectangles and handed to
`dr_textur` one by one. Falls back to the whole screen when the dirty map does not exist.

| Concern (brief §8) | Review |
| --- | --- |
| Pitch and geometry | the walk works in tiles of the dirty map, whose extent is `disp_width` = the backend's pitch (multiple of 16); rectangles reaching past the visible width are clipped by `dr_textur` to the texture; the backend computes the source pointer from its own pitch — no pitch assumption in the renderer |
| Resize | `simgraph32_on_window_resized`: `dr_textur_resize` returns the new pitch and framebuffer, `dirty_state_alloc()` builds a new map **all dirty**, then `mark_screen_dirty()` → the first frame after a resize is a full upload. The walk reads only the bitmaps; no pointer into the old framebuffer survives. Measured: phase WR/LR verify **every** frame of the resize and fullscreen scenarios, the first frame after each switch included: 0 px |
| First frame after init | `dirty_state_alloc()` sets every tile dirty → full upload. Measured: phase WI2/LI, every frame verified from the first frame of the running world: frame 0 = 1 rectangle, the whole screen; 0 px over 40 frames |
| Full-screen updates when needed | `mark_screen_dirty()` (world `set_dirty`, zoom, resize, C) marks every tile |
| Previous frame's tiles | kept (`tile_dirty_old \|= tile_dirty`): measured as necessary in trunk 16-bit (oracle fails without them while vehicles move) |
| SDL guarantees relied on | only `SDL_UpdateTexture` of a sub-rectangle of a streaming texture, which SDL documents; no lock, no persistence of locked memory, no driver pitch |
| Threads | the walk runs in `flush_framebuffer`, on the main thread after drawing, as simgraph16's |
| Memory | one `std::vector<uint32>` of `tiles_per_line / 32 + 1` words per flush (≤ 9 words at 3440 px) |

Dependency: **A requires C and D** (otherwise a light / colour change, and anything drawn by `draw_rect`, is never
uploaded); B is independent. They ship as four patches in the order C, B, D, A; A must not be applied without C and D.

## Discarded and not included

The locked-texture / `set_texture_ptr` experiments, `STLAB_SINGLE` (dropping the previous frame's tiles) and the hybrid
strategy from SIMUTRANS-SDL3-TEXTURE-PERFORMANCE-01 are not in the candidate; neither is any lab instrumentation
(0 `STLAB_PERF` occurrences, no `stlab_perf*` file, and the diff against testing-2 adds no `stlab` line; the
`STLAB_XR32_*` / `stlab_xr32_*` names in the tree are XR32's own product identifiers, unchanged since testing-2).
