> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# Memory and stability under long sessions

**Verdict: no sustained growth.** In 40 minutes of constant zooming on the keyed pak, the candidate's private bytes level off in a band of 12 MB and come back down. The early rise seen in shorter sessions happens on the keyless pak too, on testing-6 as well: it is A12's own warm-up, not the fix.

Evidence: `evidence/mem-long.txt`, samples `out/ML/*.mem.csv`, logs `out/ML/*.csv`, runs `out/ML/runs.txt`. Tools: `tools/mem_long.sh`, `tools/mem_long2.sh`, `tools/mem_sampler.ps1`, `tools/mem_analyze.py`.

## Method

- **Executables:** the lab twins (testing-6 `f6eeeaec`/`7d6d3f8c`; candidate `e027fca2`/`fd65a64e`), no debugger.
- **Scene:** `tcorners` (2,500 corner tiles).
- **Scenarios:** `zoomx`, a zoom step every 6 frames through all levels, in and out, without end; and `daynight`, a light change every 8 frames.
- **Sampling:** every 5 s from outside the process: private bytes, working set, handles, threads.
- **What is analysed:** **private bytes**. The lab log reserves its frame table at start, then fills it, so the working set grows by that table by design of the instrument.
- **Steady slope:** least squares from 120 s to the end. **Control:** the same series plus 1 MB/min must read about +10 MB/10 min. It reads +10.00 more in both controls (`mem-long.txt`).
- **Machine:** idle, no user input from 04:12 to 08:05: 13,963 s by `GetLastInputInfo` at the end.
- **Run status:** all runs exit 0, with no crash and no error output.

## Results

| Run | Duration | Private MB at 60 s → end | Steady slope | Notes |
| --- | ---: | --- | ---: | --- |
| testing-6, keys, day/night, SDL3 | 10 min | 1838.0 → 1838.0 | **+0.01** MB/10 min | flat |
| candidate, keys, day/night, SDL3 | 10 min | 1837.5 → 1837.5 | **+0.02** | flat |
| testing-6, **no keys**, zoomx, SDL3 | 13 min | 1873.4 → 1882.1 | +5.63 | A12's own warm-up |
| candidate, **no keys**, zoomx, SDL3 | 13 min | 1874.0 → 1885.9 | +7.01 | same code path as testing-6 on this pak |
| testing-6, keys, zoomx, SDL3 | 13 min | 1867.9 → 1873.6 | +3.72 | fewer derived tiles: the keyed ones fall back to 16 bits |
| candidate, keys, zoomx, SDL3 | 13 min | 1873.5 → 1887.9 | +9.47 | rising phase only |
| candidate, keys, zoomx, SDL2 | 17 min | 1887.8 → 1909.2 | +13.75 | slowing at the end (1907.8 → 1909.2 in the last 5 min) |
| **candidate, keys, zoomx, SDL3, 40 min** | 40 min | 1892.7 → **1899.9** | **+1.08** | see below |

The 40-minute run, maximum per 2 minutes (MB):

`1894.5 1895.4 1896.9 1897.7 1899.5 1901.6 1902.0 1902.4 1903.0 1903.4 | 1896.2 1896.3 1904.2 1904.2 1904.2 1905.5 1905.5 1899.3 1899.3 1899.9`

| Window | Slope | Range |
| --- | ---: | --- |
| 2–10 min | +5.2 MB/10 min | 1894.4–1899.5 |
| 10–20 min | −2.2 | 1895.7–1903.4 |
| 20–30 min | +12.0 | 1896.0–1904.2 |
| 30–40 min | −5.8 | 1898.9–1905.5 |

The slope changes sign from window to window. The memory **is returned** twice (−7 MB at minute 20, −6 MB at minute 34) and stays within 1894–1906 MB. A leak would rise steadily and never come back down.

## Why it is bounded

Each tile keeps **one** zoom cache, for the current scale, and frees it when the scale changes (`stlab_xr32_zoom_ensure` / `stlab_xr32_zoom_release`). A light change frees and rebuilds per tile. The fix adds no cache: it lets up to the same 520 tiles that a keyless pak already composes be composed. So the bound is that of A12 on any pak with XR32 terrain. The variation in the band is the heap's, as allocation sizes change from zoom level to zoom level.

## Other measures

| | |
| --- | --- |
| Peak working set in the 320 benchmark runs | within ±0.7 MB, candidate against testing-6 |
| Handles and threads | the same course in every run, on both executables (for example 1245 → 1385 handles, 27 → 46 threads) |
| Frame time over the session | busy P50 first and last 5,000 frames: equal (for example 2.544 / 2.513 ms) |
| Rebuilds | one per light change or zoom step per tile in view; the count stops at the keyless total (counters twin, previous mission) |

## Not available

- Heap tracing, ASan or Dr. Memory: MinGW builds, and nothing suitable installed. Private bytes over time were used instead, with a control.
- Sessions of hours.
- Linux.
