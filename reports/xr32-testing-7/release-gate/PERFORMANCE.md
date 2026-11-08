> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# Performance

**Method.** Plan `tools/plan-gate-perf.txt`, runner `tools/bench.sh`, analysis `tools/perf_a12.py` and `tools/perf_events.py`. Output: `evidence/perf-frames.txt`, `evidence/perf-events.txt`, `evidence/bench-manifest.csv`.

**Builds.** This lane's fresh lab twins, both hash-identical to the previous mission's: candidate `e027fca2` (SDL3) and `fd65a64e` (SDL2), against testing-6 lab `f6eeeaec` and `7d6d3f8c`.

**Runs.** Interleaved and rotated, **n = 5 per cell, 320 runs, 0 rejected.** Scene `tcorners`: 2,500 tiles, all corner slopes. 1920×1080 on SDL3, 1024×768 on SDL2 (dummy driver).

**Machine state.** The benchmark started only after 10 minutes without user input. `logs/activity.csv` shows **no user input during the whole benchmark** (180 samples, one every 30 s). Windows Search kept running: uncontrolled noise.

"base" in the tools' output is testing-6; "a12" is the candidate.

## The four paks

| Pak | Keys | Comparisons with no measurable difference |
| --- | --- | ---: |
| PAK-NEW | none | **22 of 24** |
| NK | light map | 15 of 24 |
| TK | climate texture | 14 of 24 |
| LT | both | 14 of 24 |

**Without keys: equivalent to testing-6.** The two exceptions are P95 day/night **0.07 and 0.15 ms faster** on the candidate, below anything to read into: the code drawn is the same.

## Cost per event, with keys

Median over the repetitions of the event frames, difference candidate minus testing-6 (range across repetitions):

| Event | Pak | SDL3 Δ | SDL2 Δ |
| --- | --- | --- | --- |
| **Zoom step** | NK | **+1.96 ms** (+1.71..+2.60) | **+2.49** (+2.28..+2.77) |
| | TK | **+2.20** (+2.03..+2.30) | **+2.06** (+1.89..+2.19) |
| | LT | **+2.41** (+2.37..+3.07) | **+2.98** (+2.63..+3.23) |
| Light change | NK, TK, LT | −0.09 to +0.48 | −0.36 to +0.85, ranges crossing zero |
| Frame without event | all | −0.06 to +0.06 | −0.10 to +0.03 |

**The zoom-step cost of the previous mission reproduces:** +2.5 to +2.8 ms then, +2.0 to +3.0 ms now, on both backends and on each kind of key.

The light-change event cost does not reproduce cleanly as a median: its per-repetition ranges cross zero. In the percentiles, though, it is clear:

| Percentile | Pak | SDL3 Δ | SDL2 Δ |
| --- | --- | ---: | ---: |
| Day/night P95 | NK / TK / LT | +2.84 / +2.36 / +3.21 | +2.85 / +2.29 / +3.36 |
| Day/night P99 | NK / TK / LT | +2.86 / +2.72 / +3.86 | +2.63 / +2.58 / +3.18 |
| Zoom P95 | TK / LT | +2.00 / +2.31 | +1.87 / +0.93 |
| Zoom P99 | NK / TK / LT | +2.14 / +1.23 / +2.42 | +2.03 / +1.54 / +2.66 |

So a light change costs up to about +3 ms in its slowest frames. The median event frame shows less because only some frames of a light change rebuild tiles.

Small items, under 0.25 ms: static P99 +0.24 and +0.23 ms on SDL2 (NK, LT); scroll P50 +0.10 ms on SDL2 (TK); scroll P50 −0.03 ms on SDL3 (LT). None beyond the scale of testing-6's own spread.

## Why this is not a regression

The candidate does the work testing-6 skipped by falling back to 16 bits. The candidate on a keyed pak costs what both cost on a pak without keys:

| Candidate, SDL3 | PAK-NEW (no keys) | LT (keys) |
| --- | ---: | ---: |
| Zoom step | 6.38 ms | 5.87 ms |
| Day/night P95 | 8.75 ms | 8.64 ms |
| Frame without event | 3.30 ms | 3.28 ms |

The number of compositions scales linearly with the affected tiles and stops at the keyless total. That was measured with the counters twin in the previous mission (0, 252, 462, 924, 2,100 extra for 0 to 8 of 8 corner rows); the twin here is hash-identical (`84d21471`).

## Peak memory in these runs

Within ±0.7 MB in every cell. Long sessions: `MEMORY-STRESS.md`.

## Not done

- No new optimisation (A12-P1 not implemented, as ordered).
- No GPU or driver variation, n = 5 per cell, and one machine.
