# How coast water is drawn, and where XR32 fits (SIMUTRANS-XR32-COAST-RENDERING-01)

Base: XR32 testing-3 (published commit `e71c024c`, source tree `e0c48441`, lab commit `df2a1b0`) on trunk r12377.
All line numbers are of that tree.

## 1. Three kinds of water picture

| Where | Drawn by | Image | Has 32-bit data in testing-3? |
| --- | --- | --- | --- |
| Open sea (a water tile) | `wasser_t` | `ground_desc_t::sea->get_image(0, stage)`, a **pak image** | yes, when the pak gives the sea an XR32 node |
| Flat coast (land tile, flat, with water corners) | `grund.cc:1121` `draw_alpha(sea(0, stage), beach_mask, ALPHA_RED)` | the same pak sea image, masked | yes (same image) |
| **Sloped coast** (land tile, sloped, with water corners) | `grund.cc:1110` `draw_alpha(get_water_tile(slope, stage), beach_mask, ALPHA_RED)` | a **generated** tile | **no**: this is A4 |

`draw_alpha` (simgraph32) takes the colour from the image's screen cache (`images[n].data[player]`; XR32 fills
it through `stlab_xr32_recode()` when the image has a node) and the weight from the 16-bit alpha image (the beach mask
from `ShoreTrans`, 5-bit weights).

## 2. How a sloped-coast water tile is made

`ground_desc_t::init_ground_textures()` (`ground_desc.cc:839-858`), called once, from `karte_t`'s constructor:

1. `create_texture_from_tile(sea->get_image_ptr(0, stage), boden_texture->get_image_ptr(water_climate))`: a copy of
   the water climate texture, onto which every pixel of the sea image is pasted five times, at (x, y) and at
   (x ± w/2, y ± w/4), inside rows `ref.y .. ref.h - 1` and columns `0 .. w - 1`.
2. `create_textured_tile(light_map->get_image_ptr(slope), texture, binary = true)`: the slope's light map
   cuts it. Where the light map is not black the tile takes the texture pixel; where it is black, the tile stays black.
3. The result is registered with `register_image()`, as 16-bit `STORED_PIXVAL` words only.

There is one tile per slope image and animation stage. In `pak128.german` (32 stages) that is 2,144 ids: 67 slope
images × 32. The real water tiles are 65 slopes × 32 = 2,080 (`evidence/probe-cand.txt`); slopes 40 and 80 alias ids
that are not water tiles. `pak128.german` with its own 8-stage sea gives 536.

## 3. XR32 data for an image

- Per image id there is a slot `stlab_xr32_slot_t` (40 bytes; gdbgate depends on its layout, and A4 does not change
  it). It holds the node `stlab_xr32_image_t *img` (straight RGBA8888 day plane, optional night plane, run list,
  geometry), the zoom cache, the hires choice and the resolution state.
- A pak image gets its node from the pak's XR32 v2 node at load time (`stlab_xr32_attach`, with a geometry guard).
- Every cache is built from the node:
  - the screen cache, by `stlab_xr32_recode`, through `stlab_xr32_resolve` and `encode_rows`;
  - the base-size runs, by `stlab_xr32_base_runs`;
  - the per-scale zoom cache, by `stlab_xr32_zoom_ensure` and `zoom_build`.
- Night is resolved from the node too: from its night plane if it has one, otherwise by the multiplicative darkening
  certified in testing-2/3.
- An image without a node takes the legacy 16-bit path, which is the unchanged simgraph code.

## 4. What the candidate adds

- **Registration.** `init_ground_textures()`, compiled only when `COLOUR_DEPTH == 32`, calls
  `stlab_xr32_derive_water_tile(tile, light map, sea, texture)` after each water tile. It refuses, and the tile stays
  16-bit, in any of these cases:
  - XR32 is off;
  - the sea has no XR32 day plane of its own;
  - the sea is itself derived;
  - the geometry differs from the light map's;
  - the texture is not at x = 0;
  - any of the three legacy streams carries player colours or transparent or special colours.

  Otherwise the tile gets a node **shell** (geometry only, no raster) and a **recipe** `{light, sea, ref}` in
  `std::map<image_id, stlab_xr32_derive_t>`.
- **Composition.** When a cache of the tile is built (screen, base runs or zoom), `stlab_xr32_source()` composes the
  raster into memory owned by that one call, in `stlab_derive_compose()`. It repeats steps 1 and 2 above in the same
  order, with the same bounds and the same pixel set, taking colours from:
  - each sea pixel: the sea's XR32 day plane, and its night plane if it has one. Where the node has no pixel, the
    legacy colour is used, expanded the way NIGHTGEN expands it; this is counted in `legacy_px`, and in every pak tested
    the count was 0.
  - each texture pixel: the texture's XR32 plane if it has one, otherwise its legacy colour, and at night the legacy
    level-4 colour.
  - where the light map is black: black.

  The cache is then built from that raster exactly as from a pak node, and the raster is freed.
- **Release.** `simgraph32_free_all_images_above()` first drops the XR32 slots and recipes of the ids it frees,
  through `stlab_xr32_release_above()`.
- **Unchanged.**
  - the slot struct, the resolve, the zoom and the encode code;
  - `draw_alpha`, the beach mask and the shore images;
  - the 16-bit renderer;
  - every pak image.

## 5. Life of a derived tile

```text
init_ground_textures (once)
  └─ create_textured_tile → 16-bit words (unchanged)  ── XR32 off / no sea node → legacy path, as before
  └─ stlab_xr32_derive_water_tile → shell + recipe
first draw at a zoom / light / player that has no cache yet
  └─ recode / base_runs / zoom_ensure → stlab_xr32_source → compose (≈200 µs) → cache → raster freed
later draws → the cache (no composition)
light change, palette change, zoom change → caches invalid → one composition per tile on its next draw
```

Measured in `evidence/derive-counters-w-co-s3-pak128.german-makie-nightshift.txt` (makie water, fixed coast map,
1920×1080, view not moving, renderer `night_shift` sampled every 100 frames):

| Frames | `night_shift` | Compositions |
| --- | --- | --- |
| 0–1,000 | 4 → 0 (dawn) | 587, as the light steps and the 32-stage animation reach each (slope, stage) in view |
| 1,000–1,800 | constant 0 | none (flat) |
| 1,900–2,800 | 0 → 4 (dusk) | 484 more, then flat at 4 |

There is no composition per frame in a steady view. A second run, without the `night_shift` column
(`evidence/derive-counters-long-coast.txt`), shows the same shape: 576, flat, then +484.
