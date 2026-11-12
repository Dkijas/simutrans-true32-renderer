# Windows x64 certification — testing-8 candidate, SDL3 and SDL2

**Machine:** Windows 11 Pro 10.0.26200, one display (3440×1440), MSYS2 MinGW-w64 GCC 16.1.

**Real-window runs:** every one started after 10 minutes without user input. The activity logs (`logs/activity*.csv`) show
**no input** during any of them.

**Executables certified:**

| | XR32 (`simutrans-rgba32-candidate.exe`) | 16-bit (`simutrans-16bit-candidate.exe`) |
| --- | --- | --- |
| SDL3 | `6f32326414e6a06c4130503a4e8dc838dd314015c374837e59f5b101b5efe7ef` | `8feaaf49317cf1d7…` = testing-7's |
| SDL2 | `6f3dbd0451094db4ea5acbf12a7a1cc2…` | `20cbf7da7e183f52…` = testing-7's |

The XR32 and 16-bit executables are fresh, non-incremental builds of `3200e8e`. The XR32 SDL3 build was made twice, with
identical results. Each is byte-identical to its exe in the zips (`logs/package-check.txt`).

## Matrix

| Check | SDL3 | SDL2 | Where |
| --- | --- | --- | --- |
| Build: compile, link | PASS, warning set = testing-7's | PASS, warning set = testing-7's | `logs/builds.txt` |
| Reproducible build | identical twice; identical to the previous lab's | identical to the previous lab's | the same |
| 16-bit renderer | byte-identical to testing-7 | byte-identical to testing-7 | the same |
| Automated suite (pak64) | **309/309** | **309/309** | `logs/suites.txt`; control FAIL [1/310] |
| TCC-R, real window, day | **0 failures** / 68 | **0 failures** / 68 | `TCC-R-REAL-WINDOW-VALIDATION.md`; control 18 FAIL each |
| TCC-R, real window, night | **0 failures** / 36 | not run (cday only on SDL2) | the same |
| Every XR32 image alone (German + comic) | 0 changed outside TCC-R's candidates | (SDL3 only: the drawing code is backend-independent) | the same |
| Real window = headless pixels | 41/41 files identical | — | the same |
| Window = framebuffer, all zooms, night, scroll (release exes) | 0 px except the pre-existing 8 px at 2/1 | the same | `ZOOM-2-1-PIXELS.md` |
| Dirty regions (twins, oracle) | 0 mismatches in 2,640 verified frames per config, B0 and B3 | B0 0 / 3,240, B3 2 frames (one resize run, 3 px) / 3,240 | `DIRTY-REGIONS.md`; control caught |
| Window events, rate against testing-7 | 0 / 26 runs each | B0 0/36, B3 1/36: no measurable difference | `SDL2-WINDOW-EVENTS.md` |
| Integrated benchmark | done (B0–B3 + A/A) | not claimed | `BENCHMARK-INTEGRATED.md` |
| Memory, 40–60 min sessions | done | (SDL3 only) | `MEMORY-STRESS.md` |
| Thread stress | done (dummy video; threading is backend-independent) | — | `THREAD-SAFETY.md` |
| Packages | CRC OK, contents verified, PE x86-64 | the same | `ASSET-MANIFEST.md` |
| Microsoft Defender | 0 detections | 0 detections | `DEFENDER-SCAN.md` |

## Verdict for this document

**Windows x64 SDL3 and SDL2: certified within the scope tested.**

**Limitations:**
- one machine, one display, one GPU. Multi-monitor is not validated.
- SDL2 timing and SDL2 memory are not measured separately.
- These pre-existing behaviours are unchanged:
  - the intermittent window-event mismatch, SDL2 only;
  - the 8 px at 2/1, on both backends.
- No sanitizer ran in this gate; WSL was not touched.
