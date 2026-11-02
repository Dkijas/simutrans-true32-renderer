# Framebuffer integrity — XR32 dirty-region upload (testing-3 candidate)

**Question.** With the candidate (testing-2 + C, B, D, A: only the dirty tiles are uploaded), does the window always show
exactly what the renderer drew?

**Answer.** Yes, on every lane the oracle can see, with two exceptions that are not caused by the candidate: two
toolbar elements after a live theme switch (trunk behaviour: official r12377 16-bit leaves **the same pixel positions**
stale, T1) and the switch frame itself of a resize / fullscreen change under SDL3 + X11 (testing-2 mismatches the same
frames). The observation O1 found on the way is resolved as part of T1.

## Method

The lab builds (`-DSTLAB_PERF`, identical lab code on every side) read the presented picture back from the renderer
right before `SDL_RenderPresent` (`SDL_RenderReadPixels`) and compare it with the renderer's own framebuffer at RGB565
precision, every 5th logged frame (phases WV, WM, LV, LVM, LGX, LGX2, LGXM, LGW, LGW2, LGWM), or **every** frame (WI2, WR, WT2, WTD, LI, LR, LRX, LT2). A scenario
starts when the game clock moves; 120 warm-up frames, 600 logged frames (40 in WI2 / LI), then the game quits through
its own quit path. Every run's executable hash is checked against the installed set (`evidence/bench-exes.sha256`);
runs with a wrong hash, wrong mode, missing rows or a non-zero exit are rejected, never counted.

| Side | What it is |
| --- | --- |
| `t2` | testing-2 (whole-screen upload every frame): the known-good reference for "what reaches the window" |
| `cand` | the candidate, `dr/candidate` + lab code |
| `mut` | the candidate with `STLAB_MUTANT=droprect`: every 2nd dirty rectangle of a frame is not uploaded — the control |
| `noC` | the candidate without C (light / company-colour invalidation) — the control for C |
| `off16` | official r12377, 16-bit renderer (`lab/official16`) — the reference for trunk behaviour |

Scenarios: `static` (paused), `vehicles` (traffic, game running), `scroll`, `zoom`, `zoomx` (zoom to the extremes),
`windows` (dialogs opened, moved, closed), `daynight` (scripted light levels), `daynight_raw` (light level set
directly), `playercolor` (company colour changed), `edges` (rectangles drawn with `dirty = true` at the framebuffer
borders, overlapping and adjacent), `theme` (live theme switch), `fullredraw`, `resize`, `fullscreen`. MRES and
alpha: pak64.german-mres1 / -alpha1 (WM, LVM). Map: pak128.german-cat with its 1950 save, 1920×1080.

Platforms: Windows 11 (one display, 3440×1440), SDL2 2.32.10 (Direct3D 9) and SDL3 3.4.12 (Direct3D 11); Debian 13
under WSL 2, SDL2 2.32.4 and SDL3 3.4.12, with SDL's dummy driver (software renderer) and in the WSLg session
(X11 through XWayland, and Wayland; OpenGL). **WSLg is not a native Linux desktop.**

## Results (mismatching pixels over the verified frames; full tables `evidence/oracle-matrix.txt`)

### Windows, every 5th frame (WV, 120 verified frames per cell)

| scenario | s2-t2 | s2-cand | s2-mut | s2-noC | s3-t2 | s3-cand | s3-mut | s3-noC |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| static | 0 | **0** | 3,744,960 | 0 | 1,920 | **0** | 3,744,960 | 0 |
| vehicles | 0 | **0** | 4,621,147 | 120,877,055 | 1,867 | **0** | 5,227,931 | 72,054,131 |
| scroll | 0 | **0** | 0 | 0 | 1,865 | **0** | 0 | 0 |
| zoom | 0 | **0** | 1,332,594 | 3,946,347 | 1,960 | **0** | 898,515 | 1,972,321 |
| zoomx | 0 | **0** | 1,339,286 | 3,944,662 | 2,838 | **0** | 381,180 | 3,921,643 |
| windows | 0 | **0** | 4,658,100 | 114,276,197 | 30,654 | **0** | 5,249,700 | 66,588,515 |
| daynight | 0 | **0** | 0 | 0 | 1,920 | **0** | 0 | 0 |
| daynight_raw | 0 | **0** | 949,886 | 110,059,146 | 1,845 | **0** | 1,455,834 | 72,350,586 |
| playercolor | 0 | **0** | 2,202,411 | 121,064,347 | 1,848 | **0** | 2,275,809 | 64,783,244 |
| edges | 0 | **0** | 4,822,499 | 118,735,491 | 54,970 | **0** | 4,818,475 | 73,849,300 |
| theme | 0 | 41,650 ¹ | 5,464,829 | 109,730,252 | 1,967 | 45,900 ¹ | 6,596,191 | 66,873,530 |
| fullredraw | 0 | **0** | 0 | 0 | 1,847 | **0** | 0 | 0 |
| resize | 0 | **0** | 1,771,557 | 13,943,896 | 1,022 | **0** | 1,895,772 | 17,496,767 |
| fullscreen | 0 | **0** | 14,107,271 | 70,383,013 | 1,636 | **0** | 15,332,838 | 70,025,508 |

¹ T1 (two toolbar elements, the same pixels as official r12377 16-bit). s3-t2's small counts are testing-2's SDL3 blending defect (fixed by B, see below).

- **Mutant:** fails in 11 of 14 scenarios on both backends. It passes in `scroll`, `fullredraw` and `daynight`, where
  every frame is one full-screen rectangle (there is no 2nd rectangle to drop) — in those three the oracle shows that
  the full-screen path is right, not that partial updates are.
- **noC:** fails wherever the light level changes while the world runs (`vehicles`, `windows`, `zoom`… : the save's
  clock crosses a light step during the run) and in `playercolor`; passes where every frame is a full upload or the
  game is paused. Without C, a light or colour change stays on screen unrefreshed: C is required by A.
- MRES / alpha (WM, 120 frames each, static / zoom / theme): candidate 0 in all 12 cells; s3-t2 600–695 (blending).

### Windows, every frame (WI2: 40 frames from the first frame of the running world; WR: 600 frames)

| phase / scenario | s2-t2 | s2-cand | s3-t2 | s3-cand | s3-mut |
| --- | ---: | ---: | ---: | ---: | ---: |
| WI2 vehicles (start-up) | 0 | **0** | 378 | **0** | — |
| WR edges | 0 | **0** | 277,080 | **0** | 24,654,857 |
| WR resize | 0 | **0** | 6,433 | **0** | 11,506,601 |
| WR fullscreen | 0 | **0** | 7,792 | **0** | 77,365,179 |
| WR theme | 0 | 113,070 ² | 10,503 | 221,850 ¹ | 31,631,215 |

In WI2, frame 0 is one rectangle covering the whole screen (the dirty map starts all dirty), then partial updates;
every frame matches. In WR the first frame after each resize and each fullscreen switch is verified, and matches.
² 850 px × 120 frames **plus 82 px × 135 frames**: both T1 (see O1).

### Linux (Debian 13, WSL 2)

| phase (driver) | SDL3 cand | SDL2 cand | controls |
| --- | --- | --- | --- |
| LV (dummy, 13 scenarios) | **0** in 12, theme 35,700 ¹ | VOID ³ | mutant fails 10/13, noC fails 8/13 (SDL3) |
| LVM (dummy, MRES + alpha) | **0** in all 6 | VOID ³ | — |
| LI (dummy, start-up, every frame) | **0** / 40 frames | VOID ³ | — |
| LR (dummy, every frame) | **0** in edges / resize / fullscreen, theme 196,350 ¹ | VOID ³ | mutant fails 4/4 |
| LGX + LGX2 (WSLg X11, all 14 scenarios) | **0** in 11, theme 32,300 ¹, resize / fullscreen: switch frames ⁴ | **0** in 13, theme 37,400 ¹ | SDL3 mutant fails 13 of 16 cells, SDL2 mutant 5 of 8 (LGX2) |
| LGXM (WSLg X11, MRES + alpha) | **0** in all 6 | **0** in all 6 | — |
| LGW + LGW2 (WSLg Wayland, all 14 scenarios) | **0** in 13, theme 45,050 ¹ | **0** in 13, theme 39,950 ¹ | SDL3 mutant fails 13 of 16 cells, SDL2 mutant 5 of 8 (LGW2) |
| LGWM (WSLg Wayland, MRES + alpha) | **0** in all 6 | **0** in all 6 | — |
| LRX (WSLg X11, every frame) | resize / fullscreen: switch frames ⁴, **0** elsewhere | **0** | — |

³ **VOID lane (Linux SDL2 + dummy driver).** SDL2's dummy display is fixed at 1024×768: the game's framebuffer stays
1024×768 (3.0 MB per full upload = 1024×768×4) and is drawn stretched into a 1920×1080 window, so the read-back can
never equal the framebuffer — testing-2 fails by the same ~773,000 px per frame. No SDL2 + dummy result counts, PASS or
FAIL. Linux SDL2 is certified in the WSLg X11 and Wayland sessions instead. (SDL3's dummy framebuffer does follow
`-screensize`: 7.91 MB per full upload = 1920×1080×4; the csv header's `tex=` is captured at the first flush, before the
game's own resize.)

⁴ **Switch frames under SDL3 + X11 (XWayland).** In `resize` and `fullscreen`, the frame on which the window size
changes (frames 0, 30, 60 … and 15, 105, 195 …) mismatches as a whole (> 100,000 px): X11 resizes asynchronously, so
on that frame the window and the framebuffer have different sizes. **testing-2 mismatches exactly the same frames**
(it uploads everything, so this cannot be a stale tile), SDL3 on Wayland and SDL2 on X11 do not show it, and every
other frame of the candidate is 0 px, with every frame verified in LRX. These frames are therefore UNVERIFIABLE on
that session for both builds (`evidence/switch-frames.txt`; the mutant mismatches 93–107 other frames there).

## Specific checks of the brief

| Check | Where | Result |
| --- | --- | --- |
| overlapping, adjacent and border rectangles | `edges` (WV, WR, LV, LR, LGX, LGX2, LGW, LGW2) | candidate 0 everywhere; the starting patch failed it (5,605 px) → fix D |
| fast lighting changes | `daynight` (scripted, every frame a new level), `daynight_raw` | 0; `noC` fails `daynight_raw` |
| company colours | `playercolor` | 0; `noC` fails (64–121 M px) |
| camera, zoom, extreme zoom | `scroll`, `zoom`, `zoomx` | 0 |
| windows / GUI open | `windows` | 0 |
| resize, fullscreen, first frame after each | `resize`, `fullscreen`, every frame in WR / LR / LRX | 0 (except ⁴) |
| first frame after start-up | WI2 / LI, every frame from the first frame of the running world | 0; frame 0 = full upload |
| theme change | `theme`; references WT / LT / WT2 / LT2 / WTD (official 16-bit) | T1 only: the same pixel positions as official 16-bit |
| MRES, alpha | WM, LVM | 0 |
| negative control | mutant, noC, testing-2's SDL3 blending, the SDL2-dummy and X11 cases above | every check that is counted failed at least once on a control |

Not verified by the oracle: the loading-screen frames before the world clock moves (the scenario starts with the
running world; the dirty map starts all dirty, so the first frame of the world is a full upload).

## T1 — two toolbar elements stay stale after a live theme switch (trunk behaviour)

After a theme switch, up to 850 px stay stale for the rest of that theme state (60 frames in the scenario, until the
next switch), in two pieces:

- **768 px at x 1872–1919, y 32–47** (top right, where the compass is drawn; `gui/simwin.cc:2232` draws it with
  `dirty = false`);
- **82 px at x 10–38, y 96–116** (left; the element was not identified).

Sometimes only the 82-px piece stays stale (the 768-px piece is redrawn by something else in time); which of the two
happens varies from run to run, **for official r12377 16-bit exactly as for the candidate**:

| Reference run (same scenario, same frames) | official r12377 16-bit | candidate | testing-2 |
| --- | --- | --- | --- |
| Windows SDL3, WT r1 (every 5th frame) | 39,950 px, 47 frames, box x10..1919 y32..116 | **39,950 px, 47 frames, same box** | 1,883 (blending) |
| Windows SDL3, WT r2 | 39,100 px, 46 frames | 37,400 px, 44 frames, same box | 1,896 |
| Linux SDL3 dummy, LT r1 / r2 | 49,300 / 48,450 px, 58 / 57 frames | 45,900 / 45,900 px, 54 / 54 frames | 3,069 / 2,988 |
| Windows SDL2 / SDL3, WT2 (every frame) | 850 × 254 / 850 × 249 | 850 × 260 / 82 × 60 + 850 × 212 | 0 / blending only |
| Linux X11 SDL2 / SDL3, LT2 (every frame) | 82 × 27 + 850 × 240 / 850 × 269 | 82 × 60 + 850 × 187 / 82 × 60 + 850 × 213 | 0 / blending only |
| Windows SDL3, WTD: 8 runs each ending inside the first theme block, with picture dumps | 82 px in 3 of 8, 850 px in 5 | 82 px in 2 of 8, 850 px in 6 | — |

**Same pixels, not just the same count** (`tools/theme_masks.py`, `evidence/theme-masks.txt`): from the WTD dumps, the
mismatching positions at RGB565 precision are identical in all 11 runs with 850 px (6 candidate, 5 official 16-bit) and
in all 5 runs with 82 px (2 candidate, 3 official); every 82-px set lies inside the 850-px set; the difference is the
768-px piece. Every mismatching candidate frame of every theme run in this mission is 850 or 82 px. testing-2 does not
show T1 only because it uploads the whole screen every frame. Not changed here: the fix belongs in trunk (draw those
elements with `dirty = true`, or mark the screen dirty on a theme change).

## O1 — resolved: the 82 px after a theme switch are T1

Found in WR s2-cand (every frame verified: 82 px × 135 frames next to 850 px × 120), first only on SDL2, then in WT2 on
SDL3. It looked like a stale area the candidate leaves and official 16-bit does not, because the first official
references (WT, every 5th frame) had shown only 850-px blocks. The every-frame and dump references above show official
16-bit producing the same 82-px blocks at the same positions (WT2 / LT2: Linux SDL2 82 × 27; WTD: 3 of 8). Not a
candidate defect; counted as T1.

## B — the SDL3 blending defect of testing-2

testing-2's SDL3 builds show 1,022–4,038 px per run in every scenario (up to 277,080 in `edges` with every frame
verified): framebuffer pixels with alpha ≠ 0xFF, composited by SDL3's default `SDL_BLENDMODE_BLEND` with the render
target. testing-2's SDL2 builds show 0 on every SDL2 lane; why SDL2 does not show it was not investigated (B sets
`SDL_BLENDMODE_NONE` on SDL2 as well, which makes the result independent of what the render target holds). With B: 0
on every SDL3 lane. The SDL3 oracle counts those pixels (`framebuffer alpha!=FF`, 33–189 per verified frame) and how
many of them mismatch: for testing-2 most or all of them, for the candidate none.
