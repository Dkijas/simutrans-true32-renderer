# A4 — platform validation

Machine: one Windows 11 Pro PC, a single 3440×1440 display. Linux runs on the same machine under WSL 2 (Debian 13,
GCC 14.2) with WSLg. **WSLg is not a native Linux desktop.**

## Lanes that ran

| Platform | Backend / video / renderer | Build | Gates that ran | Result |
| --- | --- | --- | --- | --- |
| Windows 11 | SDL3 3.4.12, windows, direct3d11 | MSYS2 GCC, product `52a0e2cb` + lab `w-co-s3` | coast gates (gdb), dirty-region oracle (6 coast scenarios + first frames + DAY+NIGHT pak), battery, K/M, world captures, re-init probe, counters | PASS |
| Windows 11 | SDL2 2.32.10, windows, direct3d | product `b8b2c4c9` + lab `w-co-s2` | dirty-region oracle (same set), battery, K/M | PASS |
| Windows 11 | 16-bit (SDL2 / SDL3) | product `20cbf7da` / `8feaaf49` | byte identity with the base 16-bit, the same program as testing-2's and testing-3's 16-bit | PASS |
| Linux (WSL 2) | SDL3, x11 (WSLg), opengl | lab `l-co-s3`, product `4d8efce6` | dirty-region oracle, battery, ASan/UBSan/LSan | PASS, except switch frames (see below) |
| Linux (WSL 2) | SDL3, wayland (WSLg), opengl | lab | dirty-region oracle | PASS |
| Linux (WSL 2) | SDL3, dummy, software | lab | dirty-region oracle | PASS |
| Linux (WSL 2) | SDL2 2.32.4, x11 / wayland, opengl | lab `l-co-s2`, product `f2cbc892` | dirty-region oracle, battery, ASan/UBSan/LSan | PASS |
| Linux (WSL 2) | 16-bit (SDL2 / SDL3) | product `3e7cd80e` / `9822aee1` | byte-identical to testing-3's 16-bit | PASS |

Notes on the Linux lanes:
- Under SDL3 + X11, `coastfull` and `coastresize` cannot be verified on the window-size switch frames: 3 and 8
  frames. testing-3 mismatches exactly the same frames, and on all other frames the candidate shows 0 px
  (`evidence/switch-frames.txt`).
- SDL2 + dummy is a VOID lane of the oracle, a property of SDL2's dummy display (certified with testing-3), so it was
  not run. SDL3 + dummy was run instead.

Sanitizers (Linux, both backends):

| Build | Runs | Result |
| --- | --- | --- |
| testing-3 and candidate, product and lab | world on `pak128.german-cat`, world on the coast map, 6 coast scenarios with the oracle on, DAY+NIGHT night scenario | see `evidence/asan-compare-a4.txt`: ASan errors, and UBSan / LSan report locations new in the candidate, must all be 0 |

## Not tested here (stated, not converted to PASS)

- **Native Linux desktops.** X11 and Wayland were tested only under WSLg.
- **Multi-monitor, HiDPI scaling, other GPUs and drivers.** There is one display here.
- **MSVC builds.** Only MinGW (GCC) on Windows, and GCC on Linux. The patch uses standard C++. `std::map`, `std::vector` and
  `<chrono>` are already used by the XR32 files of testing-3. **`std::atomic` is new to the whole tree** (`git grep`:
  0 files in `df2a1b0`), and only the measurement counters use it.
- **macOS, Android, ARM, 32-bit builds.**
- **Most paksets.** The composition, night and zoom oracles ran on pak128.german and its three water variants
  (makie, mutant, DAY+NIGHT).
  - The one other family touched is the battery's `pak64.german-mres1` fixture, which has an XR32 sea, so the
    candidate derives 75 pak64 coast tiles there. Only the A6 zoom-route gate saw them: identical pictures on both
    routes, and every other image byte-identical to testing-3 (`evidence/a6-extra-ids.txt`).
  - pak192, pak128 (Britain) and the others were not run.
  - The code fails closed for any sea without its own XR32 plane.
- **Network games.** The change is display-only and is not part of the game state.
