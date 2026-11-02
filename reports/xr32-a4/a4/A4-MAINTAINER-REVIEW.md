# A4 — maintainer review notes

For whoever decides whether this enters a later XR32 testing build. This is a self-review: it says what a reviewer
would ask, and answers it with the evidence or with an open decision. Nothing here was sent to anyone.

## The patch in one paragraph

Sloped-coast water is a tile that `ground_desc_t::init_ground_textures()` generates from the sea's 16-bit words, so it
never had XR32 data. The patch gives each such tile a recipe: the light map, the sea and the water climate texture.
Whenever a cache of the tile is built, it composes a 32-bit raster from the sea's XR32 planes by replaying the same
generation steps, and frees the raster again. It touches one game file, `ground_desc.cc`, with 11 lines under
`#if COLOUR_DEPTH == 32`; the rest is in the XR32 hook files. Patch: `candidate/XR32-A4-COAST-r12377-testing-3.diff`.

## Questions a reviewer will ask

| Question | Answer | Evidence |
| --- | --- | --- |
| Does the 16-bit build change? | No. The registration call is compiled only at `COLOUR_DEPTH == 32`. The Linux 16-bit binaries are byte-identical to testing-3's; on Windows the program is the same (only the 4–5 PE timestamp and checksum bytes differ). | `A4-REGRESSION-MATRIX.md` |
| Do traditional paks change? | No. A sea without its own XR32 plane registers nothing. `pak128.german`'s 1,560 tile draws are byte-identical to testing-3's. | `evidence/final-gates.txt` |
| Is the picture right, or only different? | It is right against an independent model of the generation (0 mismatches in 650 tiles over 3 paks). The model is checked against the real 16-bit output, and two controls fail. | `A4-VISUAL-VALIDATION.md` §1 |
| Night? | Each sea-sourced coast pixel is drawn exactly like the sea image at every light level 0–4, including a pak with night planes. | same |
| Zoom? | The geometry is identical at every zoom. At 2/1 the edge coverage follows XR32 magnification (H1), as the open sea already does in testing-3. | same |
| Dirty regions? | The framebuffer→window oracle finds 0 px on every coast scenario, Windows SDL2/SDL3 and Linux. The mutant control fails. | `A4-REGRESSION-MATRIX.md` |
| Memory? | Measured peak working set +0.8 MB (SDL2) and +1.1 MB (SDL3) on a coast map, paired with testing-3; ±0.1 MB without XR32 water. Rasters are never kept: keeping them would have cost 80–160 MB. | `A4-DESIGN-ALTERNATIVES.md`, gate M |
| CPU? | The typical frame (P50) shows no measurable difference. **P99 is +0.4 to +3 ms** on frames that rebuild coast caches (light change, a new animation stage, scrolling, zoom), consistent on every lane. Composition comes in bursts of about 11 tiles × ~130 µs, and its per-frame P99 (1.83 ms) explains the cost. | `A4-BENCHMARKS.csv`, `evidence/perframe-compose.txt` |
| Threads? | The map is written only at init and at release, and read during cache builds. Composition uses local buffers only. The existing mutexes are unchanged. | `A4-IMPLEMENTATION-REPORT.md` |
| Sanitizers, warnings? | ASan/UBSan/LSan show no new report location, and the warning sets are equal. | `evidence/asan-compare-a4.txt`, `evidence/warning-sets.txt` |

## Points that need a decision (NEEDS_MAINTAINER_DECISION)

1. **`stlab_xr32_release_above()` is defensive.** Its only caller path is a second `init_ground_textures()`, and the
   game never makes one: the function runs once, from `karte_t`'s constructor.
   - A gdb test called it a second time on a running world. The same ids came back with nodes again, and the oracle
     stayed clean (`evidence/reinit-probe.txt`).
   - Without the release, `stlab_xr32_attach` would replace the old node anyway, so with the same pak nothing visible
     would change. No control can show it failing in the game as it is.
   - Options: keep it (correct ownership: a 12-line function, one forward declaration and one call) or drop it (a smaller patch). Recommendation: **keep**. Freed ids
     should not keep XR32 data that describes other images.
2. **The measurement counters** are five `std::atomic`, read only by the lab's gdb tools. They cost one atomic
   increment per composition. Options: keep them for the testing builds, or drop them before anything goes upstream.
   Recommendation: drop them before an upstream proposal, and keep them while XR32 is in testing. They are also the
   only use of `std::atomic` / `<atomic>` in the whole tree (0 files in `df2a1b0`), and dropping them removes that
   dependency.
3. **H1 now reaches the coast.** At 2/1 the sloped-coast tiles get XR32's edge coverage, which differs from the
   16-bit scaler's in 1.6 % of the tiles' opaque pixels (52,026 of 3,265,584 over 195 tiles). This is the property the open sea and the flat coasts already
   have in testing-3. Fixing H1 is a separate, renderer-wide question.
4. **The P99 cost, and an obvious next step (NOT applied: it would change the frozen candidate).** By pixel count
   (estimated, not profiled), most of a composition is spent rebuilding the texture: the water climate texture with
   the sea pasted five times. That texture depends
   only on (sea stage, texture), not on the slope, so the 65 slopes of a stage rebuild the same one.
   - Keeping one texture per stage would cost 32 × the texture rectangle × 4 bytes, doubled with night planes; about
     2–4 MB for a 128 px pak. Each composition would then only cut the tile.
   - The saving was **not measured**. It needs its own iteration with the same oracles.
   - Option D (an LRU of finished rasters) is the alternative.
5. **Scope left out on purpose.**
   - All other generated ground tiles (climates, snow, alpha transitions) are 16-bit for the same reason, and the
     recipe idea extends to them.
   - The beach masks are 5-bit.

   Both are separate missions if wanted.

## What a reviewer could reasonably prune

- The two `#include`s (`<atomic>`, `<vector>`), if the counters go.
- `stlab_derive_plain()`'s scan of the three streams for special colours. It is a fail-closed guard. Pak water never
  has player colours, but a pak could, and the guard keeps such a tile 16-bit instead of composing a wrong picture.
- The doc comments. They follow the hook file's existing density; trim them to taste.

## Publication status

- **Readiness: READY_FOR_APPROVAL**, as an A4 addition to XR32 r12377 testing (see `A4-VERDICT.md`).
- **Publication: NOT_AUTHORISED.** This mission authorised no push, tag, release, forum post or contact. Nothing was
  published, and testing-3 was not altered.
