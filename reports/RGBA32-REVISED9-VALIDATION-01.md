# SIMUTRANS-RGBA32-REVISED9-VALIDATION-01

    MISSION:      SIMUTRANS-RGBA32-REVISED9-VALIDATION-01
    LANE:         C:\Users\Victor\dev\_stlab-rgba32-revised9-validation-01
    REFERENCE:    RGBA32-CANDIDATE-REVISED-8 (published), on trunk r12261
                  = git ecca332dca25025833f5ce6641491f8886a3ef9f
    CANDIDATE:    RGBA32-CANDIDATE-REVISED-9 = revised-8 + FIX-6
    PUBLICATION:  NONE - nothing pushed, released, integrated or posted
    DATE:         2026-09-23

---

## Verdict

**LISTO PARA REVISIÓN / PUBLICACIÓN** (technically ready; publishing still needs
its own authorisation).

| question | answer |
|---|---|
| does revised-8 show the defect here? | **yes, reproduced**: on a frozen scene, 14 234 px at the north edge drift away from the correct image within ~9 frames and stay there (32 bit); also west, and east when the earth border is off |
| is it the defect makie reported? | **yes, by reproduction**: same geometry, same ramp-and-reset signature, same look (near-black shadow, over-saturated crowns over the grey) |
| does revised-9 remove it? | **yes**: 0 px change over 100 frames, and equal to revised-8's own full redraw pixel for pixel |
| does revised-9 erase anything near the edge? | **no**: its frame equals revised-8's full redraw exactly (0 px), terrain, trees and shadows included |
| 16 bit | **no regression**; revised-8 16 bit has the same defect at lower amplitude, revised-9 16 bit removes it the same way |
| patch changed in this mission? | **no** - revised-9 is delivered exactly as built in the previous mission |

---

## 1. Identities

| | |
|---|---|
| FIX-6 | `_stlab-rgba32-map-edge-alpha-01/patches/FIX-6-simview-repaint-outside-every-frame.diff`, sha256 `a3a22a7b1d95901910be6d9050c8da72af975782c062df2a78ee1b56b53870a3`, 1 file +27/-2 |
| full candidate | `_stlab-rgba32-map-edge-alpha-01/patches/RGBA32-CANDIDATE-REVISED-9.diff`, sha256 `484a46351a43810f3b4f3ed341f23c105d566188b0cce73864e6deb97bc83243` |
| revised-8 source | `_stlab-rgba32-minimap-icons-01/src-r8` (2164 files), `simview.cc` `3b7aed0171a64d69…` |
| revised-9 source | the same + FIX-6 (applied rc 0, 0 rejects, 0 fuzz), `simview.cc` `16eb6cda6e2b8b5c…`; **only `simview.cc` differs** |
| candidate ↔ tested tree | the full candidate applied to a clean r12261 base reproduces the tested revised-9 tree **exactly** (0 differences); control: the same comparison against revised-8 reports `simview.cc` (`logs/verify_candidate.txt`) |

Builds: WSL Ubuntu 26.04, gcc 15.2.0, cmake 4.2.3, SDL2 2.32.10; Release, `sdl2`,
multithreaded, `SIMUTRANS_USE_REVISION=12261`, each in a fresh build directory,
from copies of the source (CMake writes three generated files into `-S`).

| build | sha256 | errors | warnings |
|---|---|---|---|
| revised-8 32 bit | `363546f32399732b51e1e2d749e6449eb5323f0671934323da9085b763fdc439` | 0 | 7 |
| revised-9 32 bit | `89ecc44bb818857cafc59363ed3cd976a4f298a36fde59e4926996943ffba26e` | 0 | 7 |
| revised-8 16 bit | `62f4cb2e1394e120…` | 0 | 7 |
| revised-9 16 bit | `e5465d02158a74ed…` | 0 | 7 |

Warning **sets** identical per depth (6 distinct), none in `simview.cc`, all
pre-existing trunk warnings (`logs/build-*.log`). revised-8 and revised-9 were
both rebuilt here with one toolchain; the published revised-8 binaries (Debian 13
/ GCC 14) would not have been an equivalent pair.

## 2. The scene

A 64x64 **pak128.german** world (its trees carry 623 057 semi-transparent pixel
words; pak64's 25 946), created through the new-game dialog with the sea level
below the terrain (`groundwater = -14`), so land runs into the map boundary on
all four sides, and dense forest. Six savegames of **the same world**, differing
only in the saved camera, placed by clicking the non-isometric minimap:

| savegame | camera tile | sha256 |
|---|---|---|
| `scene/edge-north.sve` | (32,0) | `24fd80e5ac3d75cd…` |
| `scene/edge-west.sve` | (0,32) | `c357badc12fd7705…` |
| `scene/edge-east.sve` | (63,32) | `d620f891bc2c3045…` |
| `scene/edge-south.sve` | (32,63) | `8365f63fa7457add…` |
| `scene/corner-se.sve` | (63,63) | `8c2bf4afaed597e0…` |
| `scene/interior.sve` | (32,32) | `6b427b680b4ffbf0…` |
| `scene/settings.xml` | day/night cycle **off** | `af71230f1e3927b2…` |

Made by `tools/mkscenes.sh` (the save dialog is opened from the toolbar and
checked on screen before anything is typed; xdotool's Ctrl+S is unreliable).

**Where the defect can appear.** The brief asked for south and east edges.
With the default settings those are not where it shows: south and east of the
map the engine draws the **earth-border wall**, repainted every frame, and
sprites rise *upward* from the border tiles, so nothing reaches the stale
background there (`out/visual/east-south-r8-settled.png`). The stale area touches
the terrain along the **north and west** edges, where tree crowns and their
right-falling shadows cross the boundary; that is also makie's geometry. With
`draw_earth_border = 0` the wall is gone and the **east** edge shows the defect
too (section 7). All six scenes were measured.

## 3. Procedure

`tools/run_scene.sh`, one arm per (build, scene, option):

* fresh run dir from the same runtime bundle, the same pakset copy (tree hash
  `71859fa3bc8aba95`), the same savegame and `settings.xml`; `simuconf.tab`
  differs only in the option under test; provenance per arm in
  `out/arms/<arm>/provenance.txt`;
* Xvfb 1024x768x24, mouse parked on the status bar, world **paused**
  (`-pause`) - a paused world keeps drawing (probe: 1045 frames drawn while
  paused, 1004 running), so the scene is frozen while the renderer runs;
* `settled`: 20 frames of the steady state; `ramp`: 100 frames at 25 fps, with
  **one scroll step and straight back** after 1 s. Any viewport move sets
  `welt->set_dirty()` (`viewport.cc:96`), which `display()` turns into a full
  repaint; the probe build confirmed repaints exactly at the two scroll frames.
  (Closing a window was tried first: it does **not** repaint the background in
  revised-8.) The camera returns to where it was, so the ramp contains the
  clean frame and everything after it.

Oracle, `tools/seq.py`: every frame against **revised-9's first frame of the
same scene** and against the previous frame; status bar excluded (clock);
background colour taken from the reference (64,64,64 in 32 bit, 65,64,65 in 16
bit).

Negative control, as the workbench requires: **revised-8 is the control**
(revised-9 with the fix reverted). The same harness reports it FAIL and the
candidate PASS on the same scene - it is able to fail.

## 4. 32 bit

Paused, pixel count differing from revised-9's frame (threshold 12 per channel):

| scene | revised-8 settled | revised-8 right after full repaint | revised-8 70 frames later | revised-9, all 100 frames |
|---|---|---|---|---|
| **north** | **14 234** (mean ΔL 30.7) | **0** | 14 234 (ΔL 30.7) | **0** |
| **west** | **10 799** (ΔL 25.5) | **0** | 10 799 | **0** |
| east | 0 | 0 | 0 | 0 |
| south | 0 | 0 | 0 | 0 |
| SE corner | 0 | - (*) | 0 | 0 |
| interior | 0 | 0 | 0 | 0 |
| north, `draw_outside_tile=1` | 0 | 0 | 0 | 0 |

(*) at a corner the scroll step is clamped in one direction, so the camera does
not come back; both builds are identical in every frame anyway.
Frames of the scroll itself are left out of the table.

**The ramp, north, revised-8** (px off / mean ΔL, capture frames at 25 fps
after the full repaint):
`0 → 7 699/12.9 → 11 368/16.9 → 13 177/18.8 → … → 14 234/25.5 (+9) → … → 14 234/30.7 (+70)`
- monotone, then saturated. revised-9 over the same frames: 0 throughout.

**Where** (`out/visual/north32-frozen-diffmap.png`): every differing pixel lies
on the overhang along the north boundary; nothing differs inside the map.
**What it looks like** (`out/visual/north32-ramp-strip.png`, same crop, same
frames, both builds): after the full redraw both draw the same translucent
shadow on the grey; revised-8's darkens frame by frame until it is solid black,
and the crowns over the grey become over-saturated; revised-9's never changes.

**Incremental vs full redraw.** revised-8's frame right after a forced full
repaint and revised-9's ordinary frame are **identical, 0 px**, in north and
west. revised-9's every-frame image is revised-8's correct image; it neither
erases terrain, trees or shadows near the edge nor adds anything.

**Running, not paused** (`north-run`): over 4 s nothing in this scene triggers a
repaint on its own (no ticker, no vehicles near the edge, mouse still), so
revised-8 sits saturated until the forced repaint, then shows the same ramp.
revised-9: 0 in every frame.

**Correspondence to makie's report**, established by reproduction, not by
assumption: makie's 32-bit frames (`_stlab-rgba32-map-edge-alpha-01/out/p56-triple.png`)
show a north edge, a tree on the border tile, its shadow over the grey nearly
black in one frame and translucent in the next, with the same monotone
climb-and-reset measured on the recording. The scene here reproduces the same
two states and the same transition. The **rate** of resets in makie's clip
depends on what repaints the background in their game (unknown); it is not
claimed here.

## 5. 16 bit

| scene | revised-8 settled | revised-8 after full repaint | revised-9, all frames |
|---|---|---|---|
| **north** | **8 276 → 9 034** (ΔL 12.6) | **0** | **0** |
| **west** | **6 027** (ΔL 8.9) | **0** | **0** |
| east / south / SE corner / interior | 0 | 0 / - / 0 | 0 |
| north, `draw_outside_tile=1` | 0 | 0 | 0 |

revised-8 16 bit **has the same defect**, at lower amplitude: the shadow
darkens after one frame and stops at a dark grey rather than black
(`out/visual/north16-ramp-strip.png`). This agrees with makie ("the bug itself
is old") and with the earlier measurement of their 16-bit recording. FIX-6 is
the first fix of the chain that changes the 16-bit binary; here the only
16-bit difference between revised-8 and revised-9 is the removal of that
accumulation - **no regression**.

## 6. Camera: scroll and zoom (both depths)

`tools/run_cam.sh`: record **while** scrolling 8 steps right and 8 down; zoom
out and back in; settle; force a full repaint at the same camera; then zoom out
two steps and repeat.

| check | 32 bit | 16 bit |
|---|---|---|
| revised-9 settled frame vs forced full repaint at the same camera (remnants) | **0** / **0** (zoomed out) | **0** / **0** |
| settled frames stable (frame 1 vs frame 30) | 0 | 0 |
| revised-8 vs revised-9, same camera | 4 417 / 5 648 px | 2 963 / 3 389 px |

The revised-8/revised-9 differences are, again, only on the overhang along the
edge (`out/visual/zoomout32-diffmap.png`, `moved32-diffmap.png`). During the
scroll, revised-9 shows a uniform background and translucent shadows in every
frame, with no trails (`out/visual/scrolling-r9-32.png`, `-16.png`).

An earlier camera arm used opening and closing the minimap as the repaint; the
window stayed open, so those numbers measured the window and are **discarded**.

## 7. Background options

`display_background()` is a no-op when `draw_earth_border && draw_outside_tile`.

| option | result |
|---|---|
| `draw_outside_tile=0`, border on (default) | sections 4-6 |
| `draw_outside_tile=1`, border on | both builds identical and static, both depths |
| `draw_earth_border=0`, `draw_outside_tile=0`, north | same as border on: revised-8 14 234 px (32) / 8 276 px (16), revised-9 **0**, full redraw **0** |
| `draw_earth_border=0`, `draw_outside_tile=0`, **east** | no wall: revised-8 **1 017** px (32) / **669** px (16) of shadows accumulated on the grey below the edge (`out/visual/border-off-r8.png`); revised-9 **0** |
| `draw_earth_border=0`, `draw_outside_tile=1`, north | both builds identical and static, both depths |

At the east edge the scroll step is clamped, so the camera does not return; at
the new position revised-8 again accumulates (301 px in 32 bit, 232 in 16 bit)
and revised-9 does not (`logs/summaries-border-off.txt`).

Every combination of the two options that decides what `display_background()`
does has been run, at both depths.

## 8. Reused evidence

* cause and mechanism, makie's recordings measured:
  `STLAB-RGBA32-MAP-EDGE-ALPHA-REDRAW-FIX-01.md`;
* cost: 24.6 vs 24.7 frames/s in one identical scene, background repainted 0/246
  vs 247/247 frames (`FIX-6-APPLIED.md`, probe builds) - a bounded measurement,
  not a benchmark; **not re-measured here**.

## 9. Limits

* Linux, SDL2, Xvfb, one machine. Windows (SDL2/SDL3/GDI) was not run in this
  mission. The change is in shared, platform-independent code (`simview.cc`)
  and does not touch a backend, but that is a reading of the code, not a test.
* Frame captures come from the X server, not from the game's framebuffer; the
  capture rate (25 fps) and the game's frame rate are not locked.
* One pakset (pak128.german) and one world. Other paks have fewer
  semi-transparent pixels, so less to accumulate.
* makie's savegame was not available; the correspondence rests on geometry and
  signature (section 4).

## 10. Files

`tools/` (all scripts), `scene/` (savegames, settings.xml), `logs/`
(`build-*`, `summaries.txt`, `cam2.txt`, `verify_candidate.txt`, `batch*.log`),
`out/arms/<arm>/` (frames, `seq-ramp.txt`, `provenance.txt`), `out/visual/`
(the comparable pictures).
