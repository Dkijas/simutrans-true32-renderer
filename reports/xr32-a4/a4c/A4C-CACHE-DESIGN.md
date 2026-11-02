# A4C — the per-stage texture cache: design, keys, life cycle

Mission SIMUTRANS-XR32-A4-PHASE-CACHE-OPTIMIZATION-01. Base: the A4 frozen candidate `84b3225` (on testing-3
`df2a1b0`), which this work derives from without changing it.

## 1. Branches and the three diffs

```text
df2a1b0  testing-3 (published e71c024c, source/xr32-r12377/)
  └─ 84b3225  A4 frozen candidate (XR32-COAST-RENDERING-01), unchanged
       └─ 9b03e8b  a4/prod  : production cleanup (decisions 2 and 3 below)
            └─ 0afdfbd  a4/cache : the per-stage texture cache (decision 4)  ← final candidate
```

| Diff | Range | Content |
| --- | --- | --- |
| `XR32-A4-COMPLETE-r12377-testing-3.diff` | `df2a1b0..a4/cache` | the whole A4 for production, cache included |
| `XR32-A4C-CACHE-ONLY.diff` | `a4/prod..a4/cache` | only the optimization |
| `XR32-A4C-CLEANUP-vs-84b3225.diff` | `84b3225..a4/prod` | only the removals relative to the certified A4 |

The lab never ships:
- `lab/cache` = `a4/cache` + exactly the lab files of `lab/coast` + one lab-only commit with the counters.
- `mut/cache-stale` is a control mutant whose key leaves out the stage.

## 2. Decisions taken from the brief

1. **H1 accepted.** At 2/1 the sloped coast uses XR32's edge coverage, as the open sea does. No geometric
   discontinuity is introduced; this is checked in `A4C-CERTIFICATION.md`, holes and seams at 2/1.
2. **`stlab_xr32_release_above()` removed**, together with its forward declaration and its call in
   `simgraph32_free_all_images_above()`. `simgraph32.cc` is now untouched relative to testing-3.
   - Why its absence changes neither memory management nor correctness:
     - The only path to it is a second `init_ground_textures()`, and the game makes none.
     - Exit releases everything through `stlab_xr32_release_all()`, which also clears the recipes and the new texture
       cache.
     - If a second init ever happened, `stlab_xr32_attach()` deletes the old node of a reused id. The recipe map entry
       is overwritten with the same ids. The remaining zoom / base caches describe the same pak images, regenerated
       identically.
   - Verified again on the cache candidate by the re-init probe, and by LSan at exit.
3. **The measurement counters removed** from production: five `std::atomic`, the only use of `<atomic>` in the
   tree, plus the `steady_clock` timing and the `from_sea` bookkeeping that only fed a counter. They live in the
   lab-only commit of `lab/cache`, with the same symbol names, so the probes work unchanged.
4. **Per-stage texture cache**, sections 3–5.

## 3. What is cached, and why that is enough

A composition has two steps:
1. **The texture:** the water climate texture with every sea pixel pasted five times. It depends on the sea image of
   the stage and the texture, not on the slope.
2. **The cut:** the tile's light map selects texture pixels. This depends on the slope.

The A4 candidate rebuilt step 1 for every tile: 65 slopes × the same texture.

The cache keeps the **last 4 textures** built (`STLAB_DERIVE_TEX_KEEP`), most recent first, and only those. Each
composition still does step 2 into memory owned by its caller, exactly as before. Neither the 2,144 composed tiles
nor all 32 stage textures are stored.

Why 4 entries are enough, as measured in `A4C-PERFORMANCE.md`: a rebuild burst comes from a light change, a new
animation stage in view, a scroll or a zoom step, and it composes the tiles of one stage. The first composition builds
the stage's texture; the rest of the burst hits. The extra entries cover the two display lanes composing at the same
time, and a stage change inside a burst.

## 4. Key: identity of a texture

The key is everything step 1 reads:

| Field | Why |
| --- | --- |
| `sea` id, `ref` id | which stage, which climate texture |
| `sea_data`, `ref_data` (the 16-bit streams) | the legacy colours and the run structure |
| `sea_rgba`, `sea_night` (sea node planes) | the XR32 day / night colours of the sea |
| `ref_rgba`, `ref_night` (texture node planes, or NULL) | the texture's XR32 colours, if it has any |
| `light_level` | the legacy level-4 night colour of pixels without XR32 data (`stlab_derive_legacy_rgba`) |
| `rw`, `ry`, `rh` | the texture rectangle |

Pointers identify the data, not just the ids. A node replaced at an id, or a night plane attached later (XNEX), means
a different pointer, so a miss and a rebuild. An old texture is never served for new data.

### What is not in the key, because step 1 does not depend on it

| State | Where it applies |
| --- | --- |
| zoom | `zoom_build` scales the composed tile afterwards; the texture is at 1:1 |
| `night_shift`, palette, day/night resolution | `stlab_xr32_resolve` reads the composed day / night planes afterwards |
| player | resolve |
| water animation stage | not separately: it is the sea id and its planes, which are in the key |

## 5. Life cycle and invalidation

| Event | What happens to the texture cache | Pixels affected? |
| --- | --- | --- |
| new stage in view | its texture is built on first use (miss), then hits; the oldest of 4 is evicted | no, a texture is a pure function of its key |
| light change (`night_shift`) | nothing; the screen caches are rebuilt by the existing invalidation, and their compositions hit | no |
| zoom change | nothing; zoom caches are rebuilt, and their compositions hit | no |
| palette change, day/night of the UI | nothing (resolve-level state) | no |
| resource change (a node or plane replaced) | pointer key mismatch, so a rebuild | no |
| `light_level` change (constant 0 in the 32-bit renderer today) | key mismatch, so a rebuild | no |
| `stlab_xr32_release_all()` (display shutdown / exit) | cleared | — |
| second `init_ground_textures()` (never in the game) | sea / ref ids and pointers unchanged, so it may hit; the texture is still exactly what step 1 would build from that data | no |

Ownership and threads:
- A cached texture is `std::shared_ptr<const stlab_derive_tex_t>`: immutable once built.
- The LRU list is guarded by its own mutex under `MULTI_THREAD`. The screen and base caches are built under
  `recode_img_mutex32`, the zoom cache under `rezoom_img_mutex32`, so two lanes can compose at the same time.
- A lane takes a reference under the mutex and then reads without it. An eviction meanwhile only drops the list's
  reference.
- A miss builds outside the mutex. Two lanes missing the same key at once would both build it; the results are
  identical, and the duplicate is harmless and transient.

## 6. Memory

- One entry is `rw × rh × 4` bytes, plus the same again with a night plane, plus about 100 bytes of key.
- For pak128 water (128 px climate texture), 4 entries cost at most about 0.3 MB by day (estimated). The measured
  peak working set is in `A4C-MEMORY.md`.
- No per-tile raster is kept, as in A4.
