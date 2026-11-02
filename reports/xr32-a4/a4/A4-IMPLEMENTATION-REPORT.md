# A4 — implementation report

## Identity of the candidate

| | |
| --- | --- |
| Base | XR32 testing-3: published commit `e71c024c54d6ff550d0f1b2c2a3dcf6789ffc937` (its `source/xr32-r12377/` = lab commit `df2a1b0f5624`, tree `e0c48441`), trunk r12377 |
| Candidate | lab branch `coast/candidate` = `84b3225506804ff6e14edb5e70bd0924ab78faf5`, tree `64548ac8c9e3`, one commit on `df2a1b0` |
| Patch | `deliver/candidate/XR32-A4-COAST-r12377-testing-3.diff`, sha256 `05792adcf794d8bd…`, 18,455 bytes, LF only |
| Bundle | `deliver/candidate/coast-candidate.bundle` (`df2a1b0..coast/candidate`, verified) |
| Size | 5 files, +289 / −4 |

```text
src/simutrans/descriptor/ground_desc.cc   |  11 ++   (under #if COLOUR_DEPTH == 32)
src/simutrans/display/simgraph32.cc       |   2 +
src/simutrans/display/stlab_xr32.h        |   5 +
src/simutrans/display/stlab_xr32_hook.cci | 268 +++++-
src/simutrans/display/stlab_xr32_zoom.cci |   7 +-
```

### Applying the patch

Each route below was applied to a fresh copy and then compared with the candidate's own tree
(`evidence/freeze-candidate.txt`):

| Route | Target | Result |
| --- | --- | --- |
| R1 | `git apply`, on testing-3's source (`df2a1b0`) | 0 differing entries |
| R2 | GNU `patch -p1`, on the same source | 0 differing entries |
| R3 | `git apply`, on the published commit `e71c024c` (its `source/xr32-r12377/` equals `df2a1b0`: 0 differing entries before applying) | 0 differing entries |

Controls:
- Before applying, the 5 touched files differ, so the routes really change the tree.
- One altered context line makes `git apply --check` fail (rc 1).

## What each change does

### `descriptor/ground_desc.cc` (the only change outside the XR32 files)

After each sloped-coast water tile is generated and registered, the code calls
`stlab_xr32_derive_water_tile(tile, light map, sea image, water climate texture)` when the slope has a light map.
- It is compiled only when `COLOUR_DEPTH == 32`, so the 16-bit build has none of this code.
- In the 32-bit build the tile is generated exactly as before. The call only adds XR32 data.

### `display/stlab_xr32_hook.cci` (the A4 block, 268 lines)

- **`stlab_xr32_derive_water_tile()`** registers the tile, or returns false and leaves the 16-bit tile alone (fail
  closed). It returns false when:
  - XR32 is off, or an id is out of range;
  - the sea image has no XR32 day plane of its own, has run classes (player colours, lights), or is itself derived;
  - the tile's geometry differs from its light map's;
  - the texture is not at x = 0 or is narrower than 2 px;
  - any of the three legacy streams has player colours, transparent runs or special colours (≥ 0x8000);
  - `stlab_xr32_attach()`'s geometry guard refuses.

  When it registers the tile, it attaches a node **shell**: geometry, zoomable flag and the sea's version, with no
  raster. It also stores a recipe `{light, sea, ref}` in `std::map<image_id, stlab_xr32_derive_t> stlab_derived`.
- **`stlab_derive_compose()`** builds the tile's RGBA raster, and a night plane when the sea has one, in heap memory
  owned by the caller's temporary node. It follows the generation:
  1. The texture rectangle (`ref.base_w × ref.base_h`). Each texture pixel takes the texture's XR32 colour if the
     texture has a node, otherwise its legacy colour; at night, the node's night plane or the legacy level-4 colour.
  2. Every sea pixel at the five `create_texture_from_tile` places, in the same order and bounds. Its colour comes from
     the sea's XR32 day plane, plus the night plane if present; only where the node has no pixel does it use the
     legacy colour, which is counted in `legacy_px` and was 0 in every pak tested.
  3. The tile itself, cut as `create_textured_tile(binary)` cuts it: where the light map word is not 0 it takes the
     texture pixel at the same column and row; where it is 0, the legacy black.
- **`stlab_xr32_source()`** returns the node a cache is built from: the slot's own node, or, for a derived tile, the
  composed temporary.
- **`stlab_xr32_release_above()`** drops the slots, zoom caches, base runs and recipes of freed ids.
- **`stlab_xr32_recode()`** and **`stlab_xr32_base_runs()`** call `stlab_xr32_source()`, which makes the screen cache
  and the base-size runs of a derived tile come from the composed raster. The raster is freed when the function
  returns.
- **`stlab_xr32_release_all()`** clears the recipes.
- Measurement counters, five `std::atomic`, are read only by the lab's gdb tools. See `A4-MAINTAINER-REVIEW.md`.

### `display/stlab_xr32_zoom.cci`

`stlab_xr32_zoom_ensure()` builds the per-scale cache from `stlab_xr32_source()`. It previously used `s->img`
directly.

### `display/simgraph32.cc`

`simgraph32_free_all_images_above()` calls `stlab_xr32_release_above(above)` first.

### `display/stlab_xr32.h`

Declares `stlab_xr32_derive_water_tile()`.

## Not changed

- `stlab_xr32_slot_t`: 40 bytes, the layout the gdbgate tools read.
- The resolve, encode, zoom-build and hires code.
- `draw_alpha`, the beach masks, the shore images, `grund.cc` and `wasser.cc`.
- The 16-bit renderer (simgraph16) and every pak image.

There are no trunk changes and no other mission's work: not A7, A8, N2, N3 or N4, and no SDL3 change.

## Memory and threads

- **Steady state.** 2,080 map entries plus 2,080 node shells, held for the whole game. That is about 0.25 MB,
  estimated from the struct sizes. **Measured** (gate M, Windows, coast map, mutant pak, n = 3, paired with
  testing-3): peak working set +0.8 MB on SDL2 and +1.1 MB on SDL3, against 497–522 MB. Without XR32 water, or with
  XR32 off, the difference is ±0.1 MB. No raster is
  kept. A screen cache, base run or zoom cache of a derived tile has the size the 16-bit tile's own cache had; that
  cache existed in testing-3 too.
- **Per composition.** A temporary texture of `ref.w × ref.h × 4` bytes, doubled when there is a night plane, plus
  a tile raster of `w × h × 4` bytes, doubled when there is a night plane. For a 128 px tile that is under 0.4 MB,
  freed at once.
- **Threads.**
  - The recipe map is written only by `init_ground_textures`, which runs from `karte_t`'s constructor before any
    drawing, and by the release functions.
  - Cache builds only read the map.
  - Composition uses only local buffers.
  - The counters are atomic.
  - The cache builders keep their existing locks: `recode_img_mutex32` around the screen cache and the base runs,
    `rezoom_img_mutex32` around the zoom cache. Two zoom builds of different images can therefore compose at the
    same time, which is safe because composition writes nothing shared.

## Measured CPU

From `evidence/derive-counters-*`: one composition is about 200 µs. It runs once per (tile, zoom, light level) the
first time that tile is drawn, never in a steady view. Over 3,000 frames there were 1,071 compositions and 205 ms in
total. The frame-time effect is measured in `A4-BENCHMARKS.csv`.

## Build results

All builds are fresh and non-incremental, with `SOURCE_DATE_EPOCH=1793534400`:

| Binary | sha256 |
| --- | --- |
| Windows SDL2 rgba32 | `b8b2c4c9…` |
| Windows SDL3 rgba32 | `52a0e2cb…` |
| Windows SDL2 16-bit | `20cbf7da…` |
| Windows SDL3 16-bit | `8feaaf49…` |
| Linux SDL2 32-bit | `f2cbc892…` |
| Linux SDL3 32-bit | `4d8efce6…` |
| Linux SDL2 16-bit | `3e7cd80e…` |
| Linux SDL3 16-bit | `9822aee1…` |

The full hashes are in `evidence/candidate-binaries.sha256`. Warning sets equal testing-3's on all four 32-bit
lanes: 21 / 21 on Windows and 16 / 16 on Linux, with none new. The check catches an injected warning
(`evidence/warning-sets.txt`).
