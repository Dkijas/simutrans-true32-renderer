> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# testing-6 candidate — performance (Phase E), Windows x64 SDL2 and SDL3

## Method

| | |
| --- | --- |
| Builds | lab BASE `lab/t5` (testing-5 + the lab measurement code; SDL3 `f9e42af8`, SDL2 `53809471`) against lab A12 `lab/a12` (+ the two A12 commits; SDL3 `f6eeeaec`, SDL2 `7d6d3f8c`), rebuilt here byte-identical to the A12 certification |
| Runs | **292 runs, 0 rejected, every exit code 0**; n = 5 interleaved and rotated repetitions (n = 3 for the long runs) |
| Content | makie's PAK128.german 2.5 beta, 1920×1080, `-fps 100`, 600 logged frames (3,000 for the drift runs) after 120 warm-up frames |
| Machine | otherwise idle (CPU load 0 % at start; no other lab process; bench 22:38–00:03) |
| Analysis | `tools/perf_a12.py` (P50/P95/P99 of busy = frame − sleep; per-repetition difference, its range and sign; BASE's spread) and `tools/perf_events.py` (frames of a light change or a zoom step vs the rest) |
| Data | `evidence/E-perf-analysis.txt`, `E-perf-events.txt`, `E-zoomx-p99.txt`, `E-counters-analysis.txt`; manifest with the hash of every run's executable: `evidence/bench-manifest.csv` |

**Views** (saves made with the gate's `lookat` + `save`, `evidence/E-perf-saves.txt`):

| View | Content |
| --- | --- |
| tpyr | the pyramid: every slope, seven climate bands, transitions, snow |
| tsnow | the snow face |
| tcone | the steep cone, coast and animated water |
| tflat | makie's demo world: flat grass, few distinct ground tiles |

**Noise statement.** BASE's own spread (max − min of its n runs) is 0.02–1.5 ms at P95 and up to 14 ms at P99 on the zoom scenarios. A difference is called measurable only when it is consistent in sign across repetitions and larger than that spread. "No measurable difference" is reported as such.

## 1. Steady state: no measurable difference

| Scenario (view) | SDL3 / SDL2 |
| --- | --- |
| static, scroll (tpyr, tsnow, tcone, tflat) | P50 / P95 / P99 within the noise. Single flagged cells: +0.07 to +0.11 ms (SDL3 tpyr static P99, scroll P95) and −0.11 ms (SDL2 tflat scroll P99), against spreads of 0.02–0.35 ms. Not claimed. |
| coastscroll (tcone, animated water) | no measurable difference |
| frames without an event, inside the daynight / zoom / zoomx runs | 0.00 ± 0.07 ms |

## 2. The frames that rebuild the derived tiles' caches

These are the frames of a light change or a zoom step (`E-perf-events.txt`). Median busy time of those frames, A12 − BASE; A12 was slower in every repetition except where noted.

| Event | Pyramid (tpyr) SDL3 / SDL2 | Flat (tflat) SDL3 / SDL2 |
| --- | --- | --- |
| day → night (light level up) | **+1.78 / +2.16 ms** | +0.39 / +0.46 ms |
| night → day (light level down) | +0.39 / +1.39 ms | +0.58 / +1.15 ms |
| zoom step (every 10 frames) | **+1.41 / +1.93 ms** | +0.44 / +0.75 ms |
| zoom step (every 6 frames, zoomx) | +0.31 / +1.29 ms (SDL2: 1 of 5 lower) | — |

Seen through the percentiles of whole runs:

| | SDL3 / SDL2 |
| --- | --- |
| daynight P95 / P99, tpyr | +2.3 / +2.6 ms; +2.3 / +2.2 ms |
| daynight P95 / P99, tflat | +1.4 / +0.8 ms; +1.4 / +2.1 ms |
| zoom P95 / P99, tpyr | +2.4 / +2.4 ms; +1.6 / +1.8 ms |
| zoom P95 / P99, tflat | +1.3 / +1.2 ms; P95 not measurable; P99 +1.0 ms |

**This reproduces the first certification's +1.1 to +3.5 ms**, here +0.8 to +2.7 ms.

**Does it depend on the slopes?** Yes. It scales with the number of distinct derived tiles that must be recomposed:
- the pyramid view recomposes ≈ 40 per event;
- the flat view ≈ 25, of which most are the one flat tile;
- the flat view's cost is about a third of the pyramid's.

**Does it depend on the direction of the light change?** On the pyramid, darkening costs more than lightening (+1.8 vs +0.4 ms on SDL3). On the flat view both are similar. Why the direction matters on the pyramid was not measured.

**zoomx P99: "−16 to −17 ms" for A12 is NOT an improvement.** With a zoom step every 6 frames (`E-zoomx-p99.txt`):

| | Frames above 20 ms | Longest frame |
| --- | --- | --- |
| A12 | **6.8 %** | 37–44 ms |
| BASE | 4.8 % | 64–65 ms |

A12 has more slow frames, but BASE's worst frames are longer, so its P99 is lower. The distribution changes shape; it does not improve.

## 3. Cache rebuild cost and data regenerated (lab counters)

Measured with lab-only counters: `lab/a12cnt` = `lab/a12` + counters, never part of a candidate; SDL3, read every frame by gdb (`E-counters-analysis.txt`).

| View, scenario | Events | Tiles recomposed per event | Pixels per event | Composition time per event (median / max) | Total over 600 frames |
| --- | --- | --- | --- | --- | --- |
| tpyr, static | — | 40 once, at the first draw | 0.33 Mpx | 4.3 ms once | — |
| tpyr, daynight (a light change every 8 frames) | 79 | 40 | 0.33 Mpx (≈ 1.3 MB of RGBA, transient) | 4.9 / 6.7 ms | 3,160 tiles, 26 Mpx |
| tpyr, zoom (every 10 frames) | 73 | 40 (max 45) | 0.33 Mpx | 2.1 / 4.4 ms | — |
| tpyr, zoomx (every 6 frames) | 94 | 38 (max 91) | 0.32 Mpx | 2.0 / 4.5 ms | — |
| tflat, daynight | 79 | 25 | 0.20 Mpx | 0.9 / 1.3 ms | — |
| tflat, zoom | 73 | 25 | 0.20 Mpx | 1.35 / 2.0 ms | — |

**Composition rate:** 4–15 µs per thousand pixels.

**Frequency:**
- one recomposition of the visible derived tiles per light change and per zoom step;
- none while the view is still, scrolls, or only animates water;
- nothing is stored afterwards: the composed raster is freed when the cache is built.

The times are wall time under gdb, which stops at every frame: they show proportions, not exact in-game cost. The frame-time deltas of section 2 are the in-game measurement.

## 4. Memory

| Scenario | Peak working set, A12 vs BASE (≈ 1,845–1,890 MB) |
| --- | --- |
| static, scroll, coastscroll, daynight, zoom (every view) | within −1.0 … +1.1 MB, inside the runs' own spread: **no measurable difference** |
| zoomx (a zoom step every 6 frames) | **+5.1 MB (SDL2), +9.4 MB (SDL3)**; ranges only partly overlap. Probably real, ≈ 0.3–0.5 %: the derived tiles' native zoom caches and transient compositions during zoom bursts. Not further explained. |

## 5. Cumulative regression (3,000 frames of light changes, n = 3)

**None measurable**, on either backend:
- the busy-time slope is −0.07 … +0.01 ms per 1,000 frames on both sides;
- the P50 of the first and the last 500 frames is the same within 0.08 ms;
- peak working set: BASE 1,844.7–1,865.3 MB, A12 1,845.7–1,865.7 MB.

Long runs of zooming were not measured.

## 6. Alternative, separate and not applied

The counters show that every light change recomposes the visible derived tiles, because the composed raster is not kept.

**Proposal A12-P1:** keep the composed 8-bit rasters of the derived tiles in a small LRU, sized for the tiles in view (≈ 40–100 tiles × ≈ 8 k px × 4 B ≈ 1.3–3 MB).
- **Effect:** a light change would then only re-resolve them, which is the same work as any other XR32 image. That would remove most of the light-change cost of section 2.
- **Zoom steps:** these would still compose once per scale.
- **Rules:** this is a change of the candidate and needs its own technical review and certification (the brief: no performance changes in the A12 candidate without a new review). Not done here.
