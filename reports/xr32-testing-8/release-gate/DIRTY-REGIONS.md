# Dirty regions on the final code (screen oracle, real windows)

**Result: PASS.** testing-8 (B3) matches testing-7 (B0) on every scenario and on both backends. One 3-pixel mismatch appeared,
in one SDL2 resize run of B3. It did not recur in 20 more runs of the same configuration (`SDL2-WINDOW-EVENTS.md`). The control
was caught in every scenario where a dropped rectangle can show.

## Method

- **The oracle:** the lab twins' screen oracle, `STLAB_VERIFY=10`. Every 10th frame, the screen as presented is compared with
  what the game drew. *mismatch* = differing pixels.
- **Executables:** **B0** = testing-7 twin (`7ab6a437` SDL3, `bbc33f5e` SDL2) and **B3** = testing-8 twin (`8b46d0c9` SDL3,
  `f63d83a2` SDL2). Their rendering code is the release code, plus probes.
- **Windows:** real windows, 1920×1080, `SDL_VIDEODRIVER=windows`; SDL3 3.4.12 on direct3d11, SDL2 2.32.10 on direct3d. Every
  run's CSV header says `video=windows`, and the analyser checks it.
- **Order:** the configurations rotate every repetition.
- **When:** 2026-11-12, 13:11–14:59, after more than 10 minutes without input. 215 activity samples, **none with user input**
  (`logs/activity2.csv`, `logs/activity3.csv`).
- **Runs:** 222 in total, all rc = 0 (`evidence/bench-manifest.csv`). Analysis: `tools/verify_analyze.py`,
  `evidence/verify-analysis*.txt`.

## Results, B0 against B3

| Phase | World | Scenarios | Per configuration | Verified frames (B0 / B3, each backend) | Mismatching frames |
| --- | --- | --- | --- | --- | --- |
| VC | coastworld, animated water | `coastscroll`, `coastzoom`, `coastnight`, `windows` (dialogs opened and closed), `playercolor` (company colour scheme every 20 frames), `fullscreen` (window ↔ fullscreen every 90 frames), `resize` | 2 reps × 7 | 840 / 840 | SDL3: **0 / 0**. SDL2: **0 / 2** (one `resize` run of B3, 3 px in 2 frames) |
| VT | pyramid | `zoomx` (every zoom level), `daynight`, `scroll` | 2 × 3 | 360 / 360 | **0** everywhere |
| VF | flat world | `vehicles` (moving vehicles), `zoom` | 2 × 2 | 240 / 240 | **0** everywhere |
| WE | coastworld, N and NK | `windows`, interleaved rate series | 10 per pak | SDL2 1,200 / 1,200; SDL3 (NK) 600 / 600 | **0** everywhere |
| WR | coastworld | `resize`, interleaved rate series | 10 | 600 / 600 per backend | **0** everywhere |

**Totals:**

| | Verified frames per configuration | Runs with a mismatch |
| --- | ---: | --- |
| SDL2 | 3,240 | B0 **0 of 54**, B3 **1 of 54** |
| SDL3 | 2,640 | B0 **0 of 44**, B3 **0 of 44** |

The WE and WR series were added to measure these rates (`SDL2-WINDOW-EVENTS.md`).

## Negative control

**The control:** `STLAB_MUTANT=droprect` drops every second dirty rectangle, on B3, both backends, one run per scenario.

| Scenario | SDL3 frames caught (of 60) | SDL2 | Max px |
| --- | ---: | ---: | ---: |
| `windows` | 59 | 59 | 117,951 |
| `fullscreen` | 40 | 44 | 577,275 |
| `resize` | 25 | 25 | 122,668 |
| `playercolor` | 19 | 16 | 118,839 |
| `coastnight` | 17 | 13 | 156,064 |
| `coastzoom` | 15 | 14 | 156,134 |
| `zoomx` (pyramid) | 6 | 5 | 304,754 |
| `vehicles` | 60 | 60 | 242 |
| `coastscroll` | 0 | 0 | — |
| `scroll` (pyramid) | 0 | 0 | — |
| `zoom` (flat world) | 0 | 0 | — |

**Not caught, and why.** These are measured from the frame data (`rects`, `bytes`, `dirty_frac`):
- **`coastscroll` and `scroll`:** every frame is a single full-screen rectangle. There is nothing to drop, as the A12-P1 lane
  found (its L13).
- **`zoom` on the flat world:** the 120 zoom-step frames are full-screen single rectangles. The other 480 frames carry about
  25 KB of small rectangles. That nothing visible changes in them is inferred from the result.

**The zero mismatches of these three scenarios are therefore not backed by a failing control.** `daynight` was not given a
control here; in the A12-P1 lane it was also uncatchable (0/0).

## Limitations

- **Display:** one display (3440×1440); the game window was 1920×1080. Multi-monitor behaviour is not tested.
- **Foreground:** for most samples the game window was not in the foreground (the editor was, `logs/activity*.csv`). The
  pre-existing SDL2 behaviour is sensitive to that (`SDL2-WINDOW-EVENTS.md`).
- **Executables:** the oracle runs in the lab twins. The release executables were checked with the screen capture in
  `ZOOM-2-1-PIXELS.md`.
