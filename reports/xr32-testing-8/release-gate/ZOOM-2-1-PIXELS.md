# The 8 pixels at zoom 2/1: testing-8 against testing-7

**Result: unchanged and pre-existing.** The same 8 pixels at the same coordinates differ between the real window and the
framebuffer in every run:
- testing-7 and testing-8, SDL3 and SDL2, paks N and NK, two repetitions;
- the coordinates are exactly those the A4 lane recorded on testing-7 (its open finding L4b).

**testing-8 adds no pixel.** Every other zoom step, the night step and the scroll steps show **0 pixels** off.

## Method (`tools/windows_t8.sh`, `tools/windows_t8_check.py`, `logs/windows-t8-check.txt`)

- **Executables:** the **release executables**, not lab twins: T0 = published testing-7 (`fce1ab24` SDL3, `f2901545` SDL2) and
  F3 = the candidate exe in the zips (`6f323264` SDL3, `6f3dbd04` SDL2).
- **Video:** `SDL_VIDEODRIVER=windows`, a real window at 1920×1080. The A4 lane's plan was used unchanged, under `gdbgate.py`.
- **The plan:** `coastworld` at the coast (39,40), world paused, day forced. The steps are 1/1, 2/1, 9/8, 3/4, 1/2, light level
  4, then two scroll positions. At every step:
  - the framebuffer is dumped;
  - the window is captured as the display shows it (`pw_capture.ps1`: raise the window, then `CopyFromScreen`).

  At 2/1 and 1/2 the framebuffer is dumped **again after** the capture (`z0b`, `z10b`).
- **Runs:**
  - N twice and NK once, per exe and backend: 12 runs, 96 steps;
  - 2026-11-12, 14:53–14:59, on an idle machine (0 input samples).
- **What the comparison skips:**
  - the status bar (bottom 16 rows);
  - the known `CopyFromScreen` quirk, which reads 13,11,12 as 0 (33 px per step, set apart).

## Results

| Check | Result |
| --- | --- |
| W1: every step changes the view | 7 of 7 in all 12 runs |
| W2: window = framebuffer at every step except 2/1 | **0 px** off at 1/1, 9/8, 3/4, 1/2, night and both scroll positions, in all 12 runs |
| 2/1: window against the framebuffer dumped before the capture | **8 px**, 1–6 levels, in all 12 runs |
| 2/1: window against the dump taken after the capture | **5 px**, in all 12 runs |
| W3: the 2/1 pixel set, testing-8 against testing-7 | the same 8 coordinates, SDL3 and SDL2, N and NK. **Only in testing-8: 0** |
| the set against the A4 lane's run on testing-7 (`vis-RW-N-t7-s3`) | **identical coordinates**: (940,1111) (942,1115) (944,1119) (946,1123) (965,1143) (967,1139) (1026,1027) (1028,1031) as (row, column) |
| W4: testing-7 against testing-8, framebuffers, pak N | 0 px at every step except **0–2 px at 1/2** |
| W5: each exe against its own repeat | 0 px except **0–2 px at 1/2**, for testing-7 **and** for testing-8 |
| W6 CONTROL: testing-7 against testing-8 on NK (A4 must change the keyed slopes' water) | **differs at every step**, 2,194–6,836 px, identical on SDL3 and SDL2 |

**The 1/2 difference** (W4) is the same size as each version's run-to-run variation (W5). It is not the change, and the A4 lane
saw the same.

## What the 8 pixels are

- **Location:** they lie in pairs along two short diagonals, stepping (+2, +4). That is the slope of a tile edge at 2/1.
- **Size:** 1–6 colour levels; they are not sand-coloured.
- **Timing:** three of them disappear when the window is compared with a framebuffer dumped **after** the capture. The game
  keeps drawing frames while the capture raises the window, so part of the difference is timing between the dump and the
  presented frame.
- **Cause of the other five:** not established. They are the same in testing-7, on a pak without keys, so they do not come from
  any of the three testing-8 changes.

**Status:** an open, pre-existing finding, unchanged by testing-8. It needs its own mission. This mission does not change code.
