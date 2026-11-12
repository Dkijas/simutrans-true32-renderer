# Memory over long sessions: testing-8 against testing-7

**Result: PASS.**
- **No new growth:** testing-8's memory does not grow over 40–60 minute sessions more than testing-7's does.
- **The cache:** it never held more than **14,018,560 bytes (13,690 KiB, 429 entries)**, under its 16 MiB cap. With the cap forced to 256 KiB it held at most 255 KiB.
- **Handles and threads:** identical in both versions.
- **The flag in the first run:** one first-run series was flagged, B3 coastnight, which ran too short and had a single +8 MB step. Two 57-minute reruns of that same configuration show no growth (slope 0.0 MB/h).

## Method

- **Executables:**
  - B0 = testing-7 lab twin `7ab6a437`; B3 = testing-8 lab twin `8b46d0c9`;
  - SDL3 3.4.12 on direct3d11, in real windows (`video=windows`, 1920×1080).
- **Pairs:** each B0/B3 pair ran **side by side**, at the same time, on the same paks and save.
- **Sampler** (`tools/mem.sh`): every 5 s, private bytes, working set, peak working set, handle and thread count of the game process. The game's own per-frame CSV gives the cache's content (`p1_held_kb`, `p1_held_n`).
- **Analysis** (`tools/mem_analyze.py`, `evidence/mem-analysis*.txt`), on private MB:
  - start = median over 300–600 s;
  - end = median of the last 300 s;
  - slope = least squares from 600 s on;
  - late slope = the second half;
  - the largest step between consecutive 5-minute medians.
- **Flags:**
  - **GROWTH** = slope > 15 MB/h;
  - **NEW-GROWTH** = the B3 slope exceeds its B0 pair's by more than 10 MB/h;
  - **CACHE-OVER-CAP**;
  - **SHORT** = under 40 minutes.
- **Machine:** idle during the second series; the last input was before 09:00 (`logs/activity3.csv`, idle time 32,454 s at 17:53).

### Negative control

The control adds a synthetic leak of 20 MB/h, from t = 600 s, to every B3 series. Every B3 row must then say GROWTH and NEW-GROWTH.

| Series | CONTROL | CANDIDATE |
| --- | --- | --- |
| first (out/ML) | **FAIL**, 4 of 4 B3 rows flagged twice (`mem-analysis-control.txt`) | the flags in the table below |
| second (out/ML2) | **FAIL**, 3 of 3 B3 rows flagged twice (`mem-analysis-2-control.txt`) | **0 flags** |

The first version of this detector flagged on "end − start > 32 MB". It **did not** catch a synthetic 30 MB/h leak, so it was replaced before any result was read.

## Results

| Series | Scenario | Paks | Minutes (B0 / B3) | Slope MB/h (B0 / B3) | Largest 5-min step MB (B0 / B3) | Cache held max | Verdict |
| --- | --- | --- | --- | --- | --- | --- | --- |
| ML2 | `coastnight` | LA | 57.6 / 57.0 | +3.8 / **+0.0** | +4.9 / +0.0 | 12,500 KiB (404) | ok |
| ML2 | `coastnight` r2 | LA | 57.8 / 57.8 | +6.0 / **−0.0** | +4.8 / +0.1 | 12,547 KiB (405) | ok |
| ML2 | `playercolor` | N | 58.6 / 58.3 | +0.3 / **+0.0** | +0.2 / +0.1 | 13,690 KiB (429) | ok |
| ML | `coastzoom` | LA | 44.8 / 44.3 | +9.0 / **+7.6** | +1.6 / +1.3 | 13,690 KiB (429) | ok |
| ML | `zoomx`, cap 256 KiB | N | 44.3 / 44.3 | +11.4 / **+5.0** | +1.8 / +1.0 | 255 KiB (8) | ok |
| ML | `coastnight` | LA | 37.8 / 37.6 | +0.2 / +22.9 | +0.0 / **+8.0** | 12,547 KiB (405) | SHORT; B3 GROWTH, NEW-GROWTH → rerun as ML2 |
| ML | `playercolor` | N | 36.8 / 36.5 | +0.1 / +0.0 | +0.0 / +0.0 | 13,690 KiB (429) | SHORT → rerun as ML2 |

**Handles:** 1,191–1,217 → 1,349–1,375 in every run, for both versions alike.

**Threads:** 25 → 44 in every run.

### The first-run flag

- **What happened:** B3 `coastnight` in the first series had one +8 MB step at about 28 minutes. Its slope was +22.9 MB/h over 37.6 minutes, which is short.
- **The rerun:** the same configuration then ran **twice for 57 minutes**, side by side with B0 again.
  - B3 shows slopes of +0.0 and −0.0 MB/h and steps of 0.0 and 0.1 MB;
  - B0 shows +3.8 and +6.0 MB/h and steps of 4.9 and 4.8 MB.
- **Reading:** a single step of this size occurs in testing-7 too. It did not recur in testing-8 in 115 minutes of the same scenario. It is classified as **not reproducible**, not as a leak.

### The slow growth on zoom scenarios

`coastzoom` and `zoomx` grow slowly in **both** versions:
- B0 +9.0 and +11.4 MB/h;
- B3 +7.6 and +5.0 MB/h.

testing-8 grows no faster than testing-7. The growth is pre-existing and below the GROWTH threshold. Its cause is not investigated here.

### Memory level

The level is the median from 300 to 600 s.

| Scenario | Series | testing-8 above testing-7 (MB) |
| --- | --- | ---: |
| `zoomx`, cap 256 KiB | ML | +0.2 |
| `coastnight` | ML | +4.7 |
| `coastnight` r2 | ML2 | +8.8 |
| `playercolor` | ML | +12.6 |
| `playercolor` | ML2 | +12.6 |
| `coastnight` | ML2 | +12.6 |
| `coastzoom` | ML | +26.3 |

- **The cache:** it holds up to 13.4 MiB.
- **Spread between sessions:** the **same** executable sat 41.6 MB apart in two sessions (B0 `coastnight`: 1932.7 MB in the first series, 1974.3 MB in the second). Level differences between processes are therefore only roughly attributable. The +26 MB of the one `coastzoom` pair is inside that spread.
- **Peaks:** every peak above the steady level falls in the first 10 s (loading), or is within 1 MB of the level.

## Coverage against the brief

| Requirement | Status |
| --- | --- |
| ≥ 40 minutes per main scenario | `coastnight` 57 ×2, `playercolor` 58, `coastzoom` 44, `zoomx` 44 (at the 256 KiB cap) |
| Cache ≤ 16 MiB | max 14,018,560 bytes; no CACHE-OVER-CAP flag |
| No new growth against the base | 0 NEW-GROWTH flags in the final runs; the one first-run flag did not recur |

## Limitations

- **Backend:** SDL3 only. SDL2 memory was not measured separately; the cache and the renderer are backend-independent code.
- **`zoomx`:** the 44-minute run used the 256 KiB cap, to force eviction. At the default cap, zooming is covered by `coastzoom` (44 minutes, a 429-entry cache).
- **What was sampled:** process-level counters only. No heap profiler or sanitizer ran.
- **Session length:** sessions longer than one hour were not run.
