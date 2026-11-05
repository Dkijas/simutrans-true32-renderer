> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# A12 — implementation report

| | |
| --- | --- |
| Base | `038b538e16166229266707f036cb213be5a5261b`, the functional source of XR32 testing-5 (public commit `cfcc03c6`, tag `xr32-prissi-night-image-prototype-01-r12377-testing-5`), on official SVN r12377 |
| Candidate | `f04638adc06130dc73121df50f9f2f0c89fd4e1d`, branch `a12/work` of `src/` = base + 2 commits |
| Lab twin (measurements only) | `lab/a12` = `lab/t5` (`b762fef`, the testing-5 certification lab) + the same 2 commits (`a13cfcf`, `51ae4e3`); the changed lines are identical (same hash of the sorted `+`/`-` lines) |
| Files | `src/simutrans/descriptor/ground_desc.cc` (+12), `src/simutrans/display/stlab_xr32.h` (+5), `src/simutrans/display/stlab_xr32_hook.cci` (+171 −13) |
| Format | **unchanged**: nothing is written; the XR32 v1/v2 nodes makeobj already writes are read |

## Commits

### 1. `4c71953` XR32 (A12-A): climate and snow tiles take their 32-bit picture from the climate texture's XR32 data

`ground_desc_t::init_ground_textures()` calls `stlab_xr32_derive_climate_tile(tile, light, texture)` after each
`create_textured_tile()` of a climate tile (7 climates × every slope) and a snow tile (the arctic texture × every slope).

If the texture carries XR32 colour, the tile gets:
- an XR32 node *shell*, with the geometry of the 16-bit tile and no raster;
- a *recipe* (light map, texture) in the same table as the A4 coast tiles.

The tile is derived only when the following conservative conditions hold:
- the tile and the light map have identical geometry;
- the texture is a full rectangle at x 0 covering the tile's rows (what `create_textured_tile` assumes);
- no source has special colours or translucent runs;
- the texture's node has no semantic runs.

Whenever a cache is built from the tile (screen cache, base-size runs, zoom cache), `stlab_derive_compose_climate` composes its raster into memory owned by that one build:
- **pixel set:** the light map's, exactly as the legacy copy;
- **placement:** texture row `tile.y + j − texture.y`, column `x`, exactly as the legacy;
- **colour, per channel:** `min(255, (light8 · tex8 + 64) >> 7)`. Neutral light 128 corresponds to the legacy 16; a source pixel without XR32 colour takes its RGB555 word as the palette shows it (`v5 << 3`);
- **night:** if the texture has an XNEX night plane, it is composed the same way; otherwise the XR32 night LUT darkens the tile, the same multiplicative arithmetic the palette applies to the legacy tile;
- **transparent key:** a product of exactly `231,255,255` becomes `232,255,255`. `#E7FFFF` is Simutrans' transparent key, which the XR32 zoom reads as a hole (`stlab_xr32_zoom.cci:315`, `:519`).

In this commit the light map is read as its 16-bit words.

### 2. `f04638a` XR32 (A12-B): the light map of a climate or snow tile is read from its XR32 data too

Each light map pixel takes its XR32 colour where it has one. A tile is derived when its texture **or** its light map carries XR32 data.

### Inherited without change

- **2/1:** every derived tile takes the legacy 2/1 outline (A11, `stlab_xr32_zoom.cci:647`).
- **Other zooms:** the native XR32 zoom.
- **Recipes:** they are dropped with the other XR32 state (`stlab_xr32_release_all`) when the ground textures are regenerated (`free_all_images_above`).

## What was deliberately not changed

- **The 16-bit renderer and makeobj.** All new code is under `COLOUR_DEPTH == 32`. The 16-bit executables (SDL2 and SDL3) and makeobj of the candidate are **byte-identical** to the base's (`evidence/cand-builds.txt`).
- **Sea and coast water tiles (A4) and their 2/1 outline (A11).** No line of their path changes.
- **`stlab_xr32_slot_t`.** Its layout, which the lab gate reads, is unchanged; the recipe has one more field.
- **The transition masks** (`create_alpha_tile`: climate transitions, snow line, beach outline). See C3/C4 below.

## Phase C3 / C4 — transition masks: evaluated, not implemented

What C1/C2 already change in the transitions:
- the **images** blended through the masks (the neighbour climate tile, the snow tile) are the derived 8-bit tiles;
- the sea and coast tiles already were (A4).

What stays 16-bit is the **weight** of the blend: the masks' 5-bit values give 32 levels.

**Measured on makie's real masks** (`evidence/C3-mask-weight-error.txt`):

| Mask | Mean weight error vs the 8-bit mask | Maximum |
| --- | --- | --- |
| `SlopeTrans` G, B, G+B | 1.4 % | 2.9 % / 2.8 % / 4.7 % |
| `ShoreTrans` R | 1.4 % | 2.9 % |

**Simulated with makie's real data** at the highest contrast the pak has, rock → snow (179 levels; `evidence/C3-simulation.txt`, `scratch/C3-sim-rock-snow.png`):
- the 32-level and 256-level blends differ by 2.4 levels on average and at most 5–11;
- at ×3 they are **not distinguishable**, because the texture noise is larger than the contour steps.

The simulation is offline: it uses the real textures and masks, not the engine's frame.

What an 8-bit mask would need:
- either ≈ 16 MB of stored 8-bit planes for pak128.german (≈ 2,000 masks), or the warp recomputed at every draw;
- an 8-bit counterpart of the mask's own zoom, whose coverage must equal the legacy one at every scale;
- for `ShoreTrans`, a change to the beach outline whose 2/1 geometry A11 certified.

**Decision:** C3/C4 are left as they are: `DESIGN_REQUIRED`, gain below visibility in the evidence. If a future pak shows visible contouring in a transition, the design above is the starting point. It needs a maintainer decision on memory against per-draw cost.

## Visible consequence of C1/C2: the terrain is brighter, by design

The legacy product truncates three times: the texture to 5 bits, the light to 5 bits, and the product to 5 bits. The exact 8-bit product therefore shows makie's terrain **+8 to +13 levels brighter per channel** (luma +5 % for snow, +8 % to +11 % for the other climates; `evidence/C2-brightness-bias.txt`). This is not an error of the candidate:
- each executable matches its own independent model exactly;
- the size of the shift is the expected truncation bias, ≈ (light · 3.5 + texture · 3.5)/128 + 4 levels.

It is, however, a perceptible change of the terrain's overall look. It **needs explicit approval** (makie as the pak's author, and the maintainers) before it is published. See `VISUAL-COMPARISON.md`.
