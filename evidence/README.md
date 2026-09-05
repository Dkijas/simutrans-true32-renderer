# Evidence

Selected, labelled material from RECERTIFICATION-02 (see `reports/`).

- `screenshots/` - 13 production screenshots (the in-game `take_screenshot`
  path, i.e. the framebuffer), each with a label band naming backend, bit
  depth, pakset and state. 01-08 are the same paused pak128 state through
  GDI 16-bit and GDI/SDL2/SDL3 32-bit (day, night, transparency); 09-13
  are zoom, resize, a pak64 generated world with a freshly built road, a
  pak192.comic world and a player-owned station. Images are unmodified
  apart from the label band.
- `comparison-grid.png` - the four backends side by side, three states.
- `true32-precision.png` - the same paused frame at 16 and at 32 bits with
  a mask of the pixels whose value cannot exist in RGB565 (0.0 % vs 99.7 %).
- `hashes/` - the 18 candidate files: raw SHA256/MD5 of the certified bytes,
  SHA256 after line-ending normalisation, and the M/A list.
- `runtime/` - the certification hook's logs for the stability run, the
  three TRUE32 scenarios, the three resize sequences and the 16-bit
  smoke; plus the consistency / TRUE32-pixel summary.
- `performance/benchmark.txt` - the real-frame benchmark, medians and raw
  per-window lines, and the memory readings.

Paksets are not redistributed; the screenshots were produced during local
validation with pak64, pak128 2.10.1 (as the game reports it) and pak192.comic, as
downloaded from their official locations; the exact files are identified in
`reports/RECERTIFICATION-02-provenance.txt`.
- `build-integration/` (CMAKE-MSVC-01) - the hook logs of the TRUE32
  scenario on the CMake-built GDI-32 and SDL3-32 executables, the legacy
  smoke on the CMake-built GDI-16 one, the T1-T4 probe linked against the
  CMake objects, and the width guards; screenshots 14-16 are window
  captures of the MSVC-built executables (hand project 32 and 16, CMake
  Visual Studio generator 32).
