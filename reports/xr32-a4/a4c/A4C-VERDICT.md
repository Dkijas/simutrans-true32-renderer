# A4C — verdict (SIMUTRANS-XR32-A4-PHASE-CACHE-OPTIMIZATION-01)

## **ACCEPTED: the optimized candidate replaces 84b3225 as the A4 candidate. Verdict B, CERTIFIED WITH LIMITATIONS.**

The brief's acceptance criterion is met:

| Criterion | Result |
| --- | --- |
| **Exactly A4's graphic result** | 11,247 coast-tile draws (every zoom, light level 0–4, every stage tested, 3 water paks + a traditional pak) and 87,760 battery pictures (pak64 fixture, 2 routes × 2 zooms × 2 backends) are **byte-identical** to the A4 frozen binary's. The stale-key mutant is caught: 2,862 of 3,735 draws differ |
| **Reproducible reduction of the reconstruction cost** | each composition costs 4–5× less (e.g. 134.9 → 26.6 µs in `coastnight`); per-frame composing P99 goes from 1.65–2.32 to 0.43–0.60 ms; frame-time P95/P99 fall against A4 consistently on Windows SDL2/SDL3 at 1080p and 1440p (mostly 5/5) and on Linux (3/3); 63–100 % of A4's P99 excess over testing-3 is gone in most cells. P50 is unchanged |
| **No significant memory or stability regression** | peak working set cache − A4 = −0.5 / −0.1 MB with XR32 water (never higher); K 72 / 72 PASS; ASan 0 errors, 0 new UBSan / LSan in 40 runs; re-init clean without `release_above` |

Also re-certified:
- presentation oracle: Windows and Linux 0 px;
- warning sets: equal to testing-3;
- 16-bit: unchanged, byte-identical to testing-3 on Linux;
- Linux battery: equal to A4's run;
- Windows battery: equal to A4's run, with the automated-test suites at 309 / 309 on every executable and the
  control failing.

## Why B and not A

1. **H1 discontinuities, inherited from A4 and newly found.** At world zoom 2/1, about **7 isolated 1-px sand gaps**
   appear inside the water near sloped coasts in a 1080p view. testing-3 has none at 2/1, and 1/1 is clean. They come
   from XR32's 2/1 coverage of the coast tile meeting the 16-bit-scaled beach mask. The cache does not cause them: its
   pictures are A4's byte for byte. The brief accepted H1 on condition that it introduces no discontinuity, and it
   does introduce a small one. → **NEEDS_MAINTAINER_DECISION** (`A4C-CERTIFICATION.md` §2).
2. **Residual tail cost against testing-3.** `coastnight` P99 is +0.6 to +0.75 ms, and `coastzoom` at 1440p on SDL3
   is +2.45 ms. That cell gained least from the cache: 23 % of A4's excess removed. What remains was not profiled.
3. **Platform limits, unchanged from A4.**
   - Linux was tested only under WSLg, which is not a native desktop.
   - No MSVC, macOS, multi-monitor or HiDPI.
   - The tile oracles ran on the pak128.german family; pak64 was seen through the battery's A6 gate.

## Changes relative to the certified A4 (`84b3225`)

- **Removed** (cleanup, `XR32-A4C-CLEANUP-vs-84b3225.diff`):
  - `stlab_xr32_release_above()`, its declaration and its call; `simgraph32.cc` is now identical to testing-3;
  - the five `std::atomic` counters and their timing, the only `<atomic>` use in the tree. They now live only in
    `lab/cache`.
- **Added** (`XR32-A4C-CACHE-ONLY.diff`, +100 / −12 in `stlab_xr32_hook.cci`):
  - a per-stage texture cache, an LRU of 4 entries keyed by everything step 1 of the composition reads;
  - shared, immutable entries; its own mutex; cleared at `release_all`.
- **Whole A4 for production** (`XR32-A4-COMPLETE-r12377-testing-3.diff`): 4 files, `ground_desc.cc` 11 lines under
  `COLOUR_DEPTH == 32`, the rest in the XR32 hook / zoom / header.

## Frozen local candidate

| | |
| --- | --- |
| Commit | `a4/cache` `0afdfbd31ee38c8a581a7f6251ade9bc80f60439`, tree `96f3d410` (history: `df2a1b0` → `84b3225` → `9b03e8b` → `0afdfbd`) |
| Diffs | COMPLETE sha256 `b7020b02…`, CACHE-ONLY `f1d9d168…`, CLEANUP `73e086ff…`, all LF; bundle `a4-cache.bundle` (+ `lab/cache`, `mut/cache-stale`) |
| Application | 5 routes reproduce their target trees exactly, including the PUBLISHED commit `e71c024c`'s `source/xr32-r12377/` |
| Product binaries | Windows SDL3 `43420d63…`, SDL2 `42e9089c…`; Linux SDL2 `c0cec57e…`, SDL3 `7a556636…`; 16-bit unchanged |

## Publication recommendation

- **Readiness: READY_FOR_APPROVAL** as the A4 content of a future XR32 testing build (testing-4), **replacing**
  `84b3225`.
  - It gives the same pictures, at a lower cost, with a smaller and cleaner production diff.
  - There is no reason to publish `84b3225` instead.
- **Before publishing, decide H1:**
  - (a) publish with the 7-px-per-screen gaps at 2/1 documented as a known issue, as the open sea's own H1 already is;
  - (b) first try, in a separate local iteration, drawing the sloped-coast water at 2/1 with the 16-bit-scaled
    coverage (the legacy geometry) and the XR32 colours, then re-certify.

  Recommendation: **(a)** for a *testing* build, which exists to gather such feedback, and (b) before any upstream
  proposal.
- **Publication: NOT_AUTHORISED.**
  - No push, remote tag, release, SVN change or forum post was made.
  - testing-3 and `84b3225` are unchanged.
  - Non-lab processes terminated: 0.
