# TCC-R in real windows (translucent company colours)

**Result: PASS on the final executables, SDL3 and SDL2, by day and at night. The real-window pixels are identical to the
earlier headless captures. The control fails as it must.**

## What ran

`tools/tcc_runs.sh` under `gdbgate.py`. These are the unmodified release executables, not lab twins:

| | testing-7 (T0) | testing-8 candidate (F3) |
| --- | --- | --- |
| SDL3 | `T0-s3`, `fce1ab24…` (published testing-7) | `F3-s3`, `6f323264…` (= the exe in the SDL3 zip) |
| SDL2 | `T0-s2`, `f2901545…` (published testing-7) | `F3-s2`, `6f3dbd04…` (= the exe in the SDL2 zip) |

- **Video:** `SDL_VIDEODRIVER=windows`, so each game ran in a **real desktop window**, 1920×1080. The earlier gate used the dummy
  driver.
- **When:** 2026-11-12, 09:18–10:20 (`logs/gate.txt`, step 3).
- **Fixture:** `building.stlab_tcc_fixture2.pak` (image 17137). It mixes company-colour keys with translucent pixels. It is drawn
  for **players 0, 3 and 5** at **1/1, 2/1, 9/8, 3/4 and 1/2**, then again after a round trip (1/1, 9/8, 1/2).
  - **By day:** `dayforce`.
  - **At night:** light level 4 (`n4`).
- **Real paks (`D-*`):** every XR32 image is drawn on its own. This covers PAK128.German 2.5 (50,823 images, glass-roof platforms
  among them) and pak192.comic with the TCC fixtures.

## Results

### The fixture against the corrected model (`tools/tcc_engine_oracle.py`, `logs/tcc/oracle-*.txt`)

| Backend | Light | Checks | Failures |
| --- | --- | ---: | ---: |
| SDL3 | day | 68 | **0** |
| SDL3 | night (n4) | 36 | **0** |
| SDL2 | day | 68 | **0** |

What the checks show:
- **R2, the corrected model:** at every zoom and for every player, T0 equals testing-7's model and F3 equals the corrected model
  within ±1.
- **R3, the ideal model:** against the ideal, F3's error is at most testing-7's at 1/1 and 2/1, and smaller at 9/8, 3/4 and 1/2.
- **R4 and N2, confinement:** F3 equals T0 outside the boxes that hold both a key and a translucent pixel, with 0 such pixels
  differing.
- **R5, round trip:** going back to a zoom gives the same pixels as the first visit, for T0 and for F3.
- **N1, night:** the night capture differs from the day capture, so it really is a night frame.

### Every image drawn alone (`tools/tcc_drawall_cmp.py`, `logs/tcc/drawall-*.txt`)

| Pak | Zoom | Images | Changed | Changed outside the candidate sizes |
| --- | --- | ---: | ---: | ---: |
| PAK128.German 2.5 | 1/1 | 50,823 | **0** | 0 |
| PAK128.German 2.5 | 9/8 | 50,823 | 68 | **0** |
| PAK128.German 2.5 | 3/4 | 50,823 | 60 | **0** |
| PAK128.German 2.5 | 1/2 | 50,823 | 47 | **0** |
| pak192.comic + fixtures | 1/1, 2/1 | 4 | 0 | 0 |
| pak192.comic + fixtures | 9/8, 3/4, 1/2 | 4 | 1 (the fixture) | 0 |

- **Images without specials, and opaque company colours,** are unchanged at every zoom. Every changed image has the size of a
  node with both key runs and translucent pixels (271 such nodes, 130 sizes).
- **The ids and geometry** of the drawn images are identical between T0 and F3.

### Real window against headless

| Comparison | Files | Differ |
| --- | ---: | ---: |
| `tcw-T0-s3-C-day` against the previous lab's dummy `tcc-T0-s3-C-day` | 8 raw captures | **0** |
| `tcw-T0-s3-C-n4` against `tcc-T0-s3-C-n4` | 8 | **0** |
| `tcw-F3-s3-C-day` against the previous lab's `tcc-T1-s3-C-day` | 8 | **0** |
| `tcw-F3-s3-C-n4` against `tcc-T1-s3-C-n4` | 8 | **0** |
| `tcw-F3-s3-D-german` and `-D-comic` against `tcc-T1-s3-D-*` | 9 drawall tables | **0** |

The previous lab's T1 executable (`5dbaca02…`) was a development build of the same source, not the packaged exe. The pixels are
identical, so the window system does not change what the renderer draws.

## Negative control

The control feeds T0's captures in as F3's (`--swap`, `logs/tcc/oracle-*-swap.txt`):

| Backend | Light | Expected | Result |
| --- | --- | --- | --- |
| SDL3 | day | R2/R3 fail at 9/8, 3/4, 1/2 | **18 failures** |
| SDL2 | day | R2/R3 fail at 9/8, 3/4, 1/2 | **18 failures** |
| SDL3 | night | not a control (see below) | 0 failures |

**The night checks are not a colour model.** They check that night differs from day (N1), confinement (N2) and the round
trips. All three are symmetric between T0 and F3, so swapping cannot make them fail. This is the known limitation T1 of TCC-R:
**night colours are not modelled.** Night evidence here means:
- the change stays inside the translucent-key boxes;
- the pixels are identical to the previous lab's night captures.

## Company-colour changes and players while playing

These are covered by the dirty-region scenario `playercolor` (`DIRTY-REGIONS.md`), which switches the company colour scheme
every 20 frames in a real window, and by the 40-minute memory session `N-playercolor` (`MEMORY-STRESS.md`).

## Limitations

- **Full scenes:** the fixture and drawall captures read the framebuffer an image at a time. How the full scene reaches the
  window is checked separately by the screen oracle in `DIRTY-REGIONS.md`.
- **What a player sees:** 47–68 PAK128.German images change when zoomed out, among them glass-roof platforms. This is the
  intended fix (company colour under glass at its correct share), and players will notice it.
- **Night colours:** not modelled (see the negative control above).
- **Platform:** Windows only.
