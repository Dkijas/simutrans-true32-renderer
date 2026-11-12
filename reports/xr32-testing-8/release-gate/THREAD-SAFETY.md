# Thread safety (the A12-P1 cache in the integrated candidate)

**Result: PASS within what a native Windows stress can show.** The stress ran 12 times on the candidate's cache code with
TCC-R included. Every run completed with identical results. The cache without its lock crashed in all 12 control runs.
**No race detector ran in this gate** (see "Limitations").

## What runs concurrently

The caller and lock analysis is the A12-P1 lane's (`<a12-p1-cache-1-lab>/deliver/THREAD-SAFETY.md`):
- **Locking:** one mutex guards all of the cache's shared state, and a raster is composed outside the lock.
- **Entries:** immutable and held by `shared_ptr`, so evicting or forgetting an entry never frees a raster that another thread
  is reading.
- **TCC-R** changes one weighting expression in the zoom code (`stlab_xr32_zoom.cci`). It adds no shared state, no lock and no
  new caller, so the analysis is unchanged.

## The stress, repeated on the integrated candidate (`tools/stress.sh`, `logs/stress.txt`)

| Build | Source | sha256 |
| --- | --- | --- |
| **S3** (candidate stress) | `stress-b3` = `lab-b3p` (A4 + A12-P1 + TCC-R twin) + the A12-P1 lane's `tools/apply_stress.py`, unchanged | `d93a4803…` |
| **SN** (control) | the A12-P1 lane's `stress-nolock`: the same stress on the cache with all 9 lock/unlock calls removed | `32cc2373…` |

Both builds: MinGW-w64 GCC 16.1, Release, `MULTI_THREAD`, SDL3, `-DSTLAB_PERF`.

**The stress (scenario `p1stress`):**
- 8 threads × 20,000 steps. Each step looks up a random derived ground tile (composing, inserting and evicting on a miss) and
  reads one pixel of the raster it got.
- Thread 0 also forgets a random tile every 16th step, as a re-registration does.
- At 256 KiB capacity the cache evicts all the time, including rasters that other threads hold.
- Matrix: 2 capacities (256 KiB, 16 MiB) × 2 paks (PAK128.German 2.5 N, LA with coast keys) × 3 repetitions. SDL dummy video,
  timeout 600 s.

| Build | Runs | Completed | Rasters read per run | Checksum per pak | Cache held at the end |
| --- | ---: | ---: | ---: | --- | --- |
| **S3 candidate** | 12 | **12** | 158,750 | N `9087919`, LA `9087989`, **identical** in all 6 runs of each pak, at both capacities | 256 KiB: 229–247 KB (5–6 entries); 16 MiB: 16,746,080–16,770,624 B (410–433 entries), under the cap of 16,777,216 B |
| **SN control** | 12 | **0** | — | — | **12 of 12 crashed**: 7 access violations (rc 139), 5 abnormal exits without output (rc 127) |

**The checksum** is the sum of the pixels the threads read. It is the same whatever the capacity, the evictions and the thread
interleaving, so every thread always read a correctly composed raster.

## The game's own threads

The 40-minute sessions (`MEMORY-STRESS.md`) ran the multithreaded release twins under zoom, light changes and company-colour
switching with real windows. None crashed or hung. Handles and threads were the same between testing-7 and testing-8:
1,349–1,374 handles and 44 threads at the end of every run.

## Limitations

- **No ThreadSanitizer or AddressSanitizer in this gate.** The A12-P1 lane's TSan and ASan runs used WSL Debian. The brief
  forbids modifying the WSL distributions, and they stayed untouched. The A12-P1 results (0 TSan reports in the cache's code
  under stress, ASan 0, control caught) apply to the cache code. That code is byte-identical here: the twins `lab-b2p` and
  `prof-p1` are identical binaries. TCC-R does not touch it.
- **What a stress can and cannot show.** It shows completion, consistent results and the absence of crashes and deadlocks. It
  cannot prove the absence of a data race the way a race detector can.
- **The control is built on the A12-P1 lane's P0 version of the cache, without its lock.** P0 differs from P1 in one
  water-tile check, outside the cache's locking.
