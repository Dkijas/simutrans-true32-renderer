# Known issues — XR32 DAY+NIGHT test build, trunk r12377 (testing-4)

testing-4 is testing-3 plus one graphics change: the water of sloped coasts, which the game generates from the sea
picture, now uses the pakset's 32-bit sea (A4). Nothing else changes.

- Part A lists what is still open.
- Part B lists what testing-4 changed.
- Part C lists the defects of official trunk r12377 itself.

The numbering of testing-1 to testing-3 is kept.

Every result below was measured on this build's executables, with a control that shows the check can fail, unless it
says otherwise. The certification reports are in `reports/xr32-a4/`.

## A. Still open in this test build

### A11 (new). Zoom 2/1: single sand-coloured pixels in the water next to sloped coasts

**Accepted for this experimental test build. A separate fix is planned.**

What was measured:
- Setup: world zoom 2/1, a fixed test map with 93 sloped coasts, a 1920×1080 view, forced day, a test pak whose 32-bit
  sea is one colour.
- testing-4 shows **7 isolated 1-px gaps** inside the water near sloped coasts, where the land underneath (sand) shows
  through. testing-3 shows none at 2/1.
- At 1/1 and at every other zoom level there are none; the gaps found at 1/1 are beach-mask blends, not holes.
- Method: holes enclosed by water, classified pixel by pixel against testing-3. A control with water pixels removed on
  purpose finds new holes.

The cause:
- At 2/1 the 32-bit coast water tile is magnified by XR32's own scaler. Its edge coverage differs from the 16-bit
  scaler's, by 1.6 % of the tiles' pixels.
- The 16-bit beach outline it is blended with is magnified by the 16-bit scaler.
- The open sea and flat coasts already used XR32's scaler in testing-3 (that is "H1"). testing-4 brings sloped coasts
  onto the same path.

Not caused by: the per-stage texture cache of testing-4. Its pictures are byte-identical to the first, uncached A4
version's.

Image: `docs/images/testing-4/xr32-r12377-testing-4-h1-2x1.png`.

### A12 (new). Other generated ground pictures and the beach outline stay 16-bit

The same cause as A4, a wider scope, not changed in testing-4:
- the ground pictures the game makes itself (climate transitions, snow on slopes) are made from 16-bit words;
- the beach outline (`ShoreTrans`) blends the coast with 5-bit weights.

A pak with 32-bit grass would show a 16-bit strip where climates meet.

### T1. After a live theme change two small toolbar elements can keep their old picture (trunk behaviour)

Unchanged from testing-3. The official 16-bit renderer of r12377 leaves exactly the same pixels stale.

### A7. Night pictures cost memory

Unchanged (by design). testing-4's coast water adds about +0.6 MB (SDL2) and +0.9 MB (SDL3) of peak memory with 32-bit
water, and nothing without it. Full coast pictures are never stored.

### A8. The pak syntax is a prototype

Unchanged (by design). The makeobj of this release is testing-2's, byte for byte (its sources do not include any file
testing-4 changes).

### A9. Builds and platforms

Unchanged from testing-3:
- **Linux** was tested under WSL 2 / WSLg, which is **not a native Linux desktop**.
- **Windows** was tested on one display.
- **Not tested:** macOS, Android, ARM, MSVC builds.

### A10. A zoom change costs about 1 ms more

Unchanged.

### N2, N3, N4

Unchanged. Open.

## B. Changed in testing-4

| # | What | testing-4 | Measured |
| --- | --- | --- | --- |
| A4 | The water of sloped coasts had no 32-bit detail: the game generated it from the 16-bit sea | **Fixed**: generated from the sea's 32-bit picture (and night picture) | an independent model of the generation predicts every coast tile of three water paks with 0 differences at 1:1 by day; at night every sea-sourced pixel equals the sea image at the same light level (levels 0–4, real night planes included); a pak without 32-bit water: coast tiles byte-identical to testing-3; frame time against testing-3: P50 no measurable difference, P99 +0.0 to +0.75 ms (1080p), up to +2.45 ms when zooming at 1440p on SDL3; peak memory +0.6 / +0.9 MB with 32-bit water |

testing-3's results were measured again on testing-4's executables with the same tests, and gave the same values:
- screen updates, every frame, on Windows SDL2/SDL3 and Linux;
- A1, A2, A3, A5, A6, N1, N5, the automated tests (309 / 309), clean shutdown, sanitizers and compiler warnings.

The one expected change is that the counts of 32-bit images now include the coast water tiles.

## C. Official trunk r12377 itself

Unchanged from testing-1 to testing-3 (same trunk): B1–B7 of `docs/KNOWN-ISSUES-r12377.md`, plus T1 above.
