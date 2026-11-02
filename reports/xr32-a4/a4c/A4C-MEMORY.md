# A4C — memory report

## Measured: peak working set (gate M, `evidence/shutdown-mem-cache.txt`)

Setup:
- Windows, SDL dummy video. The game loads a save, runs about two months under `-until`, and exits by itself.
- n = 3, the three executables interleaved: PUBLISHED testing-3 (`6e016e96` / `69444edc`), A4 frozen (`b8b2c4c9` /
  `52a0e2cb`), cache candidate (`42e9089c` / `43420d63`).
- Paired per repetition. 72 runs, **72 PASS** for clean exit and no FATAL / `Error:`.
- Controls: an injected FATAL line is counted; a foreign hash is flagged.

| Workload | Backend | XR32 | testing-3 | A4 | cache | cache − A4 (median; range; cache higher in) | cache − testing-3 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| coast map, mutant pak (XR32 sea, 2,080 derived tiles) | SDL2 | on | 521.4 | 522.4 | 522.0 | **−0.5** (−0.6..+0.0; 0 of 3) | +0.6 (+0.5..+0.8) |
| | SDL3 | on | 496.2 | 497.2 | 497.1 | **−0.1** (−0.1..+0.0; 0 of 3) | +0.9 (+0.8..+1.1) |
| | SDL2 | off | 518.5 | 518.3 | 518.3 | +0.1 | −0.2 |
| | SDL3 | off | 493.0 | 493.1 | 493.1 | +0.0 | +0.1 |
| oepnv1950, pak128.german-cat (no XR32 water) | SDL2 | on | 563.2 | 563.1 | 563.0 | −0.3 | −0.2 |
| | SDL3 | on | 537.9 | 538.2 | 538.0 | +0.1 | +0.1 |
| | SDL2 | off | 559.1 | 559.0 | 559.1 | +0.1 | −0.1 |
| | SDL3 | off | 534.1 | 534.0 | 534.0 | +0.0 | −0.1 |

All values are in MB.

**Reading.**
- The cache adds **no measurable memory** compared with A4. With XR32 water its peak is equal or slightly lower
  (−0.1 / −0.5 MB, never higher in 3 of 3). A plausible reason is fewer large transient allocations: one 64 KB
  texture per burst instead of one per tile. That reason is not separately measured.
- Against testing-3 the coast workload stays at +0.6 / +0.9 MB, the A4 cost; A4 itself measured +1.0 MB in this run.
- Without XR32 water, or with XR32 off, all three are within ±0.3 MB (noise).

## Bounded by design (`A4C-CACHE-DESIGN.md`)

| Structure | Size | Lifetime |
| --- | --- | --- |
| recipes + node shells (A4, unchanged) | about 0.25 MB for 2,080 tiles (estimated) | the whole game |
| texture cache | at most 4 × (`rw × rh × 4` B day, + the same at night): for a 128 px water climate texture, at most about 0.25 MB by day, 0.5 MB with night planes (estimated from the struct) | built on first use; LRU of 4; cleared at `release_all` |
| per composition | the tile raster, `w × h × 4` B (+ night) | freed when the cache build returns |
| composed tile rasters kept | **none**; neither the 2,144 tiles nor all 32 stage textures are kept | — |

## Stability

- **Clean shutdown:** 72 of 72 runs exit with code 0 on SDL2 and SDL3, with XR32 on and off.
- **LSan at exit:** no new leak origin in 40 sanitizer runs (`asan-compare-cache.txt`). The texture cache and the
  recipes are freed by `release_all`.
- **Re-init without `release_above`:** clean (`reinit-probe-cache.txt`).
