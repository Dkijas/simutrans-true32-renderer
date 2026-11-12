# SDL2 window events: testing-8 against testing-7

**Result: no difference between testing-7 and testing-8.** The inherited SDL2 behaviour, rare screen-oracle mismatches during
window events, appeared **once in 108 SDL2 runs** across both versions, and the two versions cannot be told apart at that rate.
**No new defect.**

## What was inherited

The A12-P1 lane (`<a12-p1-cache-1-lab>/deliver/WINDOWS-SDL2-SDL3-CERTIFICATION.md`, L14) saw intermittent mismatches in
the SDL2 `windows` scenario only:
- 2 of 26 runs on testing-7-based builds, 1 of 26 on the cache builds;
- the game window ran behind the foreground editor;
- SDL3 was clean throughout.

## What ran here

All runs were on the screen oracle (`DIRTY-REGIONS.md`), with real windows 1920×1080, interleaved configurations, and an idle
machine (0 input samples). Analysis: `evidence/verify-analysis*.txt`.

| Series | Scenario | Paks | B0 testing-7 runs (with a mismatch) | B3 testing-8 runs (with a mismatch) |
| --- | --- | --- | --- | --- |
| VC (gate plan) | `windows` | N | SDL2 2 (0), SDL3 2 (0) | SDL2 2 (0), SDL3 2 (0) |
| VC | `resize` | N | SDL2 2 (0), SDL3 2 (0) | SDL2 2 (**1**), SDL3 2 (0) |
| VC | `fullscreen` | N | SDL2 2 (0), SDL3 2 (0) | SDL2 2 (0), SDL3 2 (0) |
| **WE** (added) | `windows` | N + NK | SDL2 20 (0), SDL3 NK 10 (0) | SDL2 20 (0), SDL3 NK 10 (0) |
| **WR** (added after the resize event) | `resize` | N | SDL2 10 (0), SDL3 10 (0) | SDL2 10 (0), SDL3 10 (0) |
| **window events, total** | | | **SDL2 36 (0), SDL3 26 (0)** | **SDL2 36 (1), SDL3 26 (0)** |

## The one event

- **Run:** `VC r2 resize s2-B3` (`out/VC/s2-B3__resize__r2.csv`): SDL2, testing-8 twin.
- **Size:** 2 verified frames of 60 (frames 580 and 590), **3 pixels each**.
- **Context:** the window size had changed 13 frames earlier (frames 567–568 upload the whole 5 MB screen). The mismatching
  frames themselves carry ordinary small updates (3–16 rectangles, `resize_ms` 0).
- **Repeats:** the first repetition of the same configuration was clean. So were the 10 interleaved WR repetitions that followed,
  and all 12 SDL2 resize runs of testing-7.

## Reading

- **testing-7 against testing-8:**
  - 0 of 36 against 1 of 36 window-event runs on SDL2;
  - adding the A12-P1 lane's `windows` runs: testing-7-based builds 2 of 62, cache builds 2 of 62. Here the counts also include resize and fullscreen runs.

  At such rates a difference cannot be measured (n is far too small for one or two events). There is no sign that testing-8
  increased it.
- **Code:** none of the three patches touches the window, the presentation or SDL code (`INTEGRATED-COMMIT-IDENTITY.md`: only
  `stlab_xr32_hook.cci` and `stlab_xr32_zoom.cci`). Dirty-rectangle bookkeeping is unchanged, so the patches have no mechanism to
  cause it.
- **Classification:** inherited, intermittent, SDL2 only, **not a testing-8 regression**. Its cause is still not established.
  Investigating it needs its own mission, with SDL2 event tracing. This mission does not change code.

## Limitations

- **Foreground:** the game windows mostly ran behind the editor (57 of 74 activity samples in this series), as in the A12-P1
  lane. A foreground-only series was not run.
- **Setup:** one machine, one display, one GPU. Multi-monitor not tested.
