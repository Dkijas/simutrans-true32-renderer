# Integrated benchmark — B0 testing-7, B1 + A4, B2 + A4 + A12-P1, B3 testing-8

## Method (fixed before reading results: `tools/plan-perf.txt`, `tools/perf_analyze.py`)

| | |
| --- | --- |
| Executables | lab twins with identical probes (`-DSTLAB_PERF`), SDL3, direct3d11, real window 1920×1080, `-fps 100`: **B0** `7ab6a437` (testing-7), **B0q** = B0's bytes under another name (A/A control), **B1** `0589e8a4` (+ A4), **B2** `d46ca9eb` (+ A4 + A12-P1), **B3** `8b46d0c9` (testing-8 = B2 + TCC-R). B0, B1 and B2 are byte-identical to the A12-P1 lane's prof twins (`SOURCE-VERIFICATION.md`) |
| Runs | 600 logged frames after the lab's own warm-up of 120; n = **5** per configuration, **interleaved** (the order rotates every repetition); 200 runs, all rc = 0 (`evidence/bench-manifest.csv`, exe hash per run) |
| Scenes | **PT** pyramid (`tpyr`: every slope, climate bands, snow): zoom through every level (`zoomx`), a light change every 8 frames (`daynight`), static. **PF** flat world (`tflat`): `zoomx`, `scroll`, static. **PC** coast with coast keys (`coastworld`, PAK128.German LA, A4 derives every coast tile): zoom (`coastzoom`), light changes with animated water (`coastnight`) |
| Machine | 2026-11-12, 13:36–14:24, after more than 10 minutes without input. 146 activity samples from 13:11 to 14:24, **none with user input**: the user had been idle for over 4 hours (`logs/activity2.csv`). No other heavy job ran: the memory sessions and stress had finished at 13:10 |
| Metrics | work = frame − limiter sleep, P50 / P95 / P99; **event frames** = frames where a screen cache was rebuilt or a zoom cache built (light changes, zoom steps); **zoom events** = zoom-cache frames only; composition CPU; cache hit ratio and bytes held; peak working set |
| Statistics | per configuration, the median of the 5 per-run values with min–max. **"separated 5/5"**: every run of one side below every run of the other (chance p = 2/252 = 0.008). Otherwise, a delta within either side's spread is **no measurable difference**. The A/A delta B0q−B0 is the bench's own noise |
| FPS | the game is capped at 100 FPS. **No FPS claim is made.** Static and day/night scenes sit at the cap in every configuration |

Raw files: `out/PT`, `out/PF`, `out/PC`. Full tables: `evidence/perf-analysis.txt`.

## Results: testing-8 (B3) against testing-7 (B0)

Medians in ms, with [min–max] over the 5 runs.

| Scene | Metric | B0 testing-7 | B3 testing-8 | B3 − B0 | A/A B0q − B0 |
| --- | --- | --- | --- | --- | --- |
| **light changes**, pyramid | event P50 | 6.405 [6.079–6.653] | 5.582 [5.531–5.953] | **−0.823 (−12.8 %), separated 5/5** | −0.026, nmd |
| | work P95 | 6.490 | 5.658 | −0.832, overlap (B2−B1 −0.882 separated) | −0.073, nmd |
| | work P99 | 7.334 | 6.387 | −0.948, nmd | −0.068, nmd |
| | work P50 | 3.056 | 3.133 | +0.077, nmd | −0.069, nmd |
| | composition CPU per run | 74.8 | **0.0** | all from the cache (hit ratio 1.000) | |
| **zoom**, pyramid | zoom-event P50 / P95 | 11.62 / 24.39 | 10.92 / 23.49 | −0.70 / −0.90, nmd | +0.42 / +0.13, nmd |
| | work P50 / P99 | 2.563 / 23.97 | 2.531 / 23.42 | nmd / nmd | nmd |
| **zoom**, flat world | work P99 | 46.44 [46.41–48.60] | 45.23 [43.32–46.16] | **−1.21, separated 5/5** | +0.48, nmd |
| | event P95 | 46.58 | 44.87 | **−1.71, separated 5/5** | +0.34, nmd |
| | zoom-event P50 | 22.20 | 22.18 | −0.02, nmd | −0.12, nmd |
| **zoom**, coast with keys | zoom-event P50 | 31.89 [31.74–32.54] | 31.18 [30.24–31.31] | **−0.71, separated 5/5** | −0.33, nmd |
| | work P95 / P99 | 13.63 / 34.61 | 13.14 / 34.00 | nmd / nmd | nmd |
| **light changes**, coast with keys | **work P50** | 3.433 [3.366–3.480] | 3.523 [3.506–3.581] | **+0.090 (+2.6 %), separated 5/5: slower** | +0.014, nmd |
| | event P50 / P95 / P99 | 3.70 / 12.18 / 13.37 | 3.79 / 11.65 / 12.89 | +0.09 / −0.53 / −0.48, nmd | nmd |
| **scroll**, flat world | work P50 / P95 / P99 | 2.596 / 3.491 / 4.205 | 2.692 / 3.613 / 4.098 | nmd | nmd |
| **static**, pyramid / flat | work P50, P95, P99 | — | — | nmd in all | nmd |

## By step (which patch does what)

| Scene | B1 − B0 (A4) | B2 − B1 (A12-P1) | B3 − B2 (TCC-R) |
| --- | --- | --- | --- |
| light changes, pyramid | nmd | **event P50 −0.915, P99 −1.003, work P95 −0.882, P99 −0.942: all separated 5/5** | nmd |
| zoom, pyramid | nmd | event P50 −0.819, separated | nmd |
| coast, light changes | work P50 +0.067, nmd; composition CPU 50.9 → 71.7 ms (A4 composes the coast tiles that testing-7 left at 16 bits) | composition CPU 71.7 → 5.4 ms; work P50 +0.027, nmd | nmd |
| coast, zoom | nmd | composition CPU 62.8 → 2.7 ms | nmd |
| every other metric and scene | nmd | nmd | **nmd in every scene and metric** |

**TCC-R costs nothing measurable.** **A12-P1 accounts for the gain on light changes.** The coast cost of A4 (more 32-bit
composition) is taken back by the A12-P1 cache, as composition CPU. The one unfavourable separated result is coast with light
changes: median work is +0.090 ms. That is about 6× the A/A noise, and no single step explains it: A4 gives +0.067 and A12-P1
+0.027, neither separated. Its tail (P95, P99) is lower, not higher.

## Memory (peak working set, from the same runs)

| Scene | B0 | B3 | B3 − B0 | Explained by |
| --- | ---: | ---: | --- | --- |
| coast, light changes | 1,868 MB | 1,880 MB | **+12.0 MB, separated** | the cache: 12.3 MiB held (B2−B1 +11.9) |
| coast, zoom | 1,876 MB | 1,891 MB | **+15.4 MB, separated** | the cache 13.4 MiB held, plus A4 +3.5 |
| zoom, flat world | 1,895 MB | 1,901 MB | **+5.5 MB, separated** | cache 7.8 MiB held |
| zoom, pyramid | 1,880 MB | 1,886 MB | +6.2 MB, nmd | cache 11.8 MiB held |
| light changes, pyramid; scroll | 1,859 / 1,864 MB | 1,861 / 1,865 MB | +1.3 MB, separated | cache 1.3 / 1.1 MiB held |
| static | 1,860 / 1,864 MB | 1,861 / 1,864 MB | nmd | |

The cache is bounded at 16 MiB (`STLAB_DERIVE_CACHE_BYTES`). Its largest observed content was 14,018,560 bytes (13.4 MiB; the probe column `p1_held_kb` is KiB). Stabilised memory over
40-minute sessions is covered in `MEMORY-STRESS.md`.

## Noise statement

The A/A control (B0q against B0, the same bytes) showed **no separated difference** in any of the 8 scenes, on any metric.

**Every claim above is either separated 5/5 or stated as no measurable difference (nmd).** The single-run spread is up to 1–4 ms
on P99 and on zoom events. A delta smaller than that is not a result.

## Limitations

- **Backends and builds:** SDL3 only. SDL2 timing is not claimed, as in the A12-P1 lane. The twins carry probes, so absolute
  times are those of the instrumented build. The release executables contain no probes.
- **Hardware:** one machine, one GPU, one display.
- **FPS:** capped at 100. FPS was recorded but **no FPS claim is made**.
- **Zoom:** testing-8 does **not** make zooming generally faster. A10, the zoom speed-up, was excluded. Zoom-event medians move by
  at most 0.7 ms.
