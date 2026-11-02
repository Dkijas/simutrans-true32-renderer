# A4 — visual validation

All results here are for the **frozen candidate binary** (Windows SDL3 `simutrans-rgba32.exe` `52a0e2cb…`) against the
**published testing-3** binary (`69444edc…`), with XR32 on unless stated otherwise. The gates leave the executable
unmodified: gdb only stops in `intr_refresh_display`, calls the game's own drawing functions and reads the framebuffer.

## 1. Tile level: every coast water tile, drawn alone (`evidence/final-gates.txt`)

`tools/coastgate.py` draws each generated water tile alone, on black and on white, at each zoom and light level, and
reads back the pixels the renderer wrote. `tools/coast_model.py` is an independent model: a Python reimplementation of
`create_texture_from_tile` + `create_textured_tile(binary)` from the pak files, read through `pakwalk`. It predicts
each tile twice: once from the sea's 16-bit words (**model16**) and once from its XR32 plane (**model32**).

### Composition oracle at 1/1, light level 0

| Pak | Run | Expected model | Tiles | Geometry / coverage / colour mismatches |
| --- | --- | --- | --- | --- |
| mutant (key colour sea) | candidate, XR32 on | model32 | 195 | 0 / 0 / 0, with 588,414 key-colour px |
| mutant | candidate, XR32 off | model16 | 195 | 0 / 0 / 0, with 0 key-colour px |
| mutant | testing-3 | model16 | 195 | 0 / 0 / 0 |
| makie (real water art) | candidate | model32 | 195 | 0 / 0 / 0 |
| makie | testing-3 | model16 | 195 | 0 / 0 / 0 |
| DAY+NIGHT water (4 stages, night planes) | candidate | model32 | 260 | 0 / 0 / 0 |

The fact that model16 matches the real 16-bit output exactly shows that the model is faithful. Controls, which must
fail:
- a mutant build that drops the four neighbour copies, against model32: colour mismatches in 180 of 195 tiles;
- testing-3 against model32: colour mismatches in 192 of 195 tiles.

### Precision, makie water, 1/1, day

| | px with a channel off the RGB555 grid | distinct colours |
| --- | --- | --- |
| testing-3 | 0 % | 58 |
| candidate | 71.9 % | 1,063 |

### Night, judged relative to the sea image itself

There is no new night formula to check. Each coast pixel that comes from the sea must look exactly like that sea pixel
when the pak sea image is drawn alone at the same light level; testing-2 and testing-3 certified that drawing.

| Pak | Light levels | Sea-sourced px | px that differ |
| --- | --- | --- | --- |
| makie | 0, 2, 4 | 588,414 | 0 |
| DAY+NIGHT, with the pak's own night planes | 0–4 | 784,552 | 0 |
| mutant | 0, 4 | 196,138 | 0 |

Pixels that the light map makes black stay black. The controls fail as they should:
- the sea of another stage: 192 of 195 tiles differ (makie) and 256 of 260 (DAY+NIGHT). The mutant's stages are all
  identical, so this control cannot apply to it.
- the sea at another light level: every tile differs.

### Zoom geometry against testing-3

The drawn rectangle and the opaque pixel set are **identical** at 11/8, 1/1, 3/8 and 1/8, for 195 tiles × 2 light
levels (mutant) and 249 draws × 3 light levels (makie).

At **2/1**, the rectangle is identical but 52,026 edge pixels differ. This is **H1**: XR32 magnification covers edges
differently from the 16-bit scaler. testing-3 already shows the same property for its pak sea images: 13,176 px over
54 images, XR32 on against off, identical in the candidate. What A4 changes is that the coast tiles now go the XR32
path too, so at 2/1 they carry H1 as the open sea already did. The control, comparing testing-3 with itself shifted by
one tile, reports differences.

### Traditional pak

`pak128.german` has no XR32 water. Its 1,560 tile draws (4 zooms × 2 light levels × 195) are **byte-identical** to
testing-3's, and 0 nodes are attached. The control, comparing neighbouring draws, finds 1,552 of 1,552 different.

## 2. World level: the fixed coast map (`evidence/visual-compare.txt`, `evidence/visual-repeat.txt`)

Setup:
- `save/coastworld.sve` is a start-up world saved once, because the start-up world is random. It is 8,374 bytes with
  93 sloped coasts.
- The capture forces day, then dumps the framebuffer at 1920×1080 at world zoom 1/1, 2/1 and back to 1/1, using the
  battery's `gdbgate.py`.

| Pak | Zoom | Differing px, candidate vs testing-3 | Notes |
| --- | --- | --- | --- |
| makie | 1/1 | 10,110 (0.49 %) | colours inside the difference: testing-3 4,192, candidate 5,682 |
| makie | 2/1 | 9,021 | |
| mutant | 1/1 | 13,509 | key colour: **+3,413 px only in the candidate, 0 px only in testing-3** |
| mutant | 2/1 | 13,067 | key colour: +1,365 only in the candidate, 0 only in testing-3 |
| DAY+NIGHT | 1/1 | 10,159 | |

### How much of a single comparison is real

Run-to-run variation was measured with 3 testing-3 captures and 2 candidate captures, using the DAY+NIGHT pak:
- Two testing-3 runs already differ by about 36,000 px. The water animation stage, vehicles and the status-bar clock
  change between runs.
- One candidate capture at 2/1 differed by 551,268 px: the open sea, which A4 does not touch, was dumped at the other
  animation image. Its repetition gives 45,285 px against the same testing-3 run.
- Pixels where a candidate run differs from **all three** testing-3 runs:
  - 10,117 and 29,164 at 1/1, against a floor of 17,680 for testing-3 run 0 against runs 1 and 2;
  - 27,647 at 2/1, against a floor of 17,638.

  The mask `captures/daynight-day-candidate-only-mask.png` shows where they lie: the **triangular strips of the sloped
  coasts**, plus two timing-dependent interface elements (the status-bar text and a message window).

### Captures (`deliver/captures/`)

- `<pak>-<tag>-testing3.png`, `<pak>-<tag>-candidate.png` and `<pak>-<tag>-diffmask.png`, for paks makie, mutant and
  daynight, and tags day (1/1), z2 (2/1) and back (1/1).
- `<pak>-<tag>-crop-t3-cand-mask.png`: the densest 160×90 window of differences, ×3, shown as
  testing-3 | candidate | mask.
  - In **mutant-day**, testing-3 draws the sloped coast with the blue-grey legacy water next to the magenta XR32 sea:
    the seam A4 is about. The candidate fills the strip with the XR32 water, blended by the beach mask, with no seam.
  - In **makie-day**, the difference is the triangular sloped-coast strips. The candidate's water has more tones and no
    RGB555 steps; it is subtle, because makie's legacy and XR32 water are close in colour.

## 3. What was not looked at

- **No human visual review on screen.** Everything above is framebuffer-based. The windowed and fullscreen paths
  were covered by the dirty-region oracle (`A4-REGRESSION-MATRIX.md`), which checks that what reaches the window is
  what the renderer drew.
- **Night at world level.** `gdbgate` has no step that forces night. Night is covered at tile level, above, for every
  light level 0–4, including a pak with real night planes.
- **Mostly one pakset family.** The tile oracles ran on pak128.german and its three water variants. pak64 coast
  tiles (75, in the battery's `pak64.german-mres1` fixture) were only checked by the A6 zoom-route gate
  (`A4-REGRESSION-MATRIX.md`, B3), not by the composition oracle. pak192 and others were not tested. The code fails
  closed for a sea without its own XR32 plane.
