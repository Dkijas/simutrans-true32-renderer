# A4C — performance: does the cache reduce the reconstruction cost?

The full data is in `A4C-BENCHMARKS.csv` (88 rows). The rule for a claim: a difference is called real only if every
paired repetition has the same sign **and** the median exceeds the reference configuration's own run-to-run spread.
Otherwise the reading is "no measurable difference", and with n < 3 there is no claim.

## Setup

| | |
| --- | --- |
| Executables | lab builds with identical lab code: testing-3 `w-t3` / `l-t3`; A4 frozen `w-co` / `l-co` (`lab/coast` = 84b3225 + lab files + its counters); cache `w-ca` / `l-ca` (`lab/cache` = a4/cache + the same lab files + the same counters) |
| Map | the fixed coast map `coastworld` (93 sloped coasts, view centred on one), makie water (2,080 derived tiles in A4 and cache) |
| Scenarios | `coast` (view still; light and animation advance), `coastnight` (light-level changes), `coastscroll` (camera scroll), `coastzoom` (zoom steps) |
| Phases | WB Windows 1920×1080 n=5; WH Windows 2560×1440 n=5; LBX Linux WSLg X11 1920×1080 n=3. The three configurations are interleaved and rotated. 600 measured frames per run; busy = frame − sleep |
| Machine | otherwise idle: the phase started after every other job, with 0 lane game processes alive (`logs/perf.txt`). All 312 runs were accepted (exe hash, rc, no oracle, frame count) |

## 1. Reconstruction cost (the thing the cache targets)

`evidence/rebuild-cost.txt`. Composition counters are read at every frame under gdb, 600 frames per scenario, on
Windows SDL3. Both exes run under the same conditions.

| Scenario | Compositions | Time per composition, A4 → cache | Per-frame composing time P99, A4 → cache | Max, A4 → cache | Texture builds / hits |
| --- | --- | --- | --- | --- | --- |
| `coast` | 433 / 433 | 193.7 → **48.3 µs** (×0.25) | 2.32 → **0.60 ms** | 3.17 → 1.63 ms | 13 / 420 (97.0 %) |
| `coastnight` | 1,247 / 1,247 | 134.9 → **26.6 µs** (×0.20) | 1.78 → **0.46 ms** | 2.81 → 0.97 ms | 13 / 1,234 (99.0 %) |
| `coastscroll` | 405 / 404 | 150.8 → **40.2 µs** (×0.27) | 1.67 → **0.43 ms** | 2.56 → 1.03 ms | 13 / 391 (96.8 %) |
| `coastzoom` | 1,202 / 1,246 | 122.2 → **27.7 µs** (×0.23) | 1.65 → **0.50 ms** | 3.56 → 0.81 ms | 14 / 1,232 (98.9 %) |

The same compositions happen (same counts, ±4 %), but each one costs **4–5× less**. The texture cache serves
97–99 % of them, and building the 13–14 textures took 1.4–2.1 ms in total.

## 2. Frame time

All values are in ms, as medians over runs.

### P99 on Windows, where A4's cost was

| Phase | Backend | Scenario | testing-3 | A4 | cache | cache − A4 (lower in) | cache − testing-3 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| WB 1080p | SDL2 | coastnight | 7.82 | 9.65 | 8.46 | **−1.17 (5/5)** | +0.61 (0/5) |
| WB 1080p | SDL3 | coastnight | 7.84 | 10.54 | 9.06 | **−1.49 (5/5)** | +0.75 (0/5) |
| WB 1080p | SDL2 | coast | 5.36 | 6.32 | 5.47 | **−0.93 (5/5)** | +0.03 |
| WB 1080p | SDL3 | coast | 5.78 | 6.28 | 5.72 | −0.42 (5/5) | +0.09 |
| WB 1080p | SDL2 / SDL3 | coastscroll | 6.05 / 6.43 | 6.77 / 7.26 | 6.52 / 6.64 | −0.25 (4/5) / **−0.53 (5/5)** | +0.24 / +0.13 |
| WB 1080p | SDL2 / SDL3 | coastzoom | 38.94 / 39.09 | 40.36 / 40.33 | 39.40 / 39.30 | **−1.35 / −1.31 (5/5)** | +0.10 / +0.69 |
| WH 1440p | SDL2 / SDL3 | coastnight | 14.02 / 12.38 | 15.66 / 14.24 | 14.57 / 12.83 | −0.42 (4/5) / −1.39 (4/5) | +0.63 / +0.61 |
| WH 1440p | SDL2 / SDL3 | coastzoom | 43.94 / 42.14 | 47.06 / 45.31 | 44.23 / 43.90 | **−2.86 (5/5)** / −1.01 (4/5) | +0.57 / **+2.45 (0/5)** |
| WH 1440p | SDL3 | coastscroll | 10.47 | 11.14 | 10.56 | −0.67 (4/5) | +0.01 |

### Share of A4's P99 excess (against testing-3) that the cache removes

The excess is the median of the deltas paired by repetition (A4 − testing-3 and cache − testing-3).

| Phase | Backend | Scenario | A4 excess | Cache excess | Removed |
| --- | --- | --- | --- | --- | --- |
| WB | SDL2 | coastnight | +1.63 | +0.61 | 63 % |
| WB | SDL3 | coastnight | +2.73 | +0.75 | 73 % |
| WB | SDL2 | coast | +0.96 | +0.03 | ≈ 100 % |
| WB | SDL3 | coastscroll | +0.84 | +0.13 | ≈ 85 % |
| WB | SDL2 | coastzoom | +1.41 | +0.10 | ≈ 93 % |
| WH | SDL2 | coastzoom | +3.02 | +0.57 | 81 % |
| WH | SDL3 | coastzoom | +3.18 | +2.45 | 23 % (the smallest gain measured) |
| WH | SDL3 | coastnight | +1.89 | +0.61 | 68 % |

### P95

The same direction as P99. Cache − A4 is −0.3 to −2.4 ms with all repetitions lower in most cells, for example WB
coastnight −0.96 / −1.39 ms (5/5). On Linux:
- `coastzoom` SDL3: −2.42 ms (3/3);
- `coastnight`: −1.43 ms (SDL2, 3/3) and −1.00 ms (SDL3, 3/3).

### P50

No measurable difference between any two of the three configurations, in any phase or scenario.

### Linux (WSLg X11, n=3, indicative)

The same pattern. P99 cache − A4:
- `coastnight`: −1.53 (SDL2) and −1.34 (SDL3) ms, 3/3;
- `coastzoom` SDL3: −2.03 ms, 3/3;
- `coastscroll` SDL2: −0.39 ms, 3/3.

The cache stays above testing-3 by +0.5 to +0.8 ms at P99 in `coastnight` and `coast`. One P95 cell is worse than
testing-3: `coastzoom` SDL2, +1.81 (0/3 lower). The same cell at P99 shows no measurable difference, and A4 was
already +1.07 there.

## 3. Reading

- **The reduction is real and reproducible.**
  - The reconstruction itself is 4–5× cheaper per composition.
  - Its per-frame P99 drops from 1.65–2.32 ms to 0.43–0.60 ms.
  - Frame-time P95/P99 fall against A4 consistently on Windows SDL2/SDL3 at both resolutions, and on Linux, mostly
    in 5 of 5 (or 3 of 3) repetitions.
- **Not all of A4's tail cost goes away.**
  - Against testing-3, `coastnight` P99 stays at +0.6 to +0.75 ms.
  - `coastzoom` at 1440p on SDL3 stays at +2.45 ms (0/5 lower).
  - What remains is plausibly the rest of what a 32-bit coast costs and a 16-bit one did not: the per-tile cut (step
    2), and the XR32 resolve / encode / zoom-build of the composed tile, which testing-3 never ran for these tiles.
    These were **not separately profiled**.
- **The typical frame (P50) is unchanged.**
