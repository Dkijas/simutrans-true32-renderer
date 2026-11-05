> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# A12 — architecture options

Input: `A12-ROOT-CAUSE.md`. Three pieces lose precision:
- (A) the climate and snow tiles, as a texture × light product;
- (B) the light map that product reads;
- (C) the 5-bit masks of the climate, snow and beach transitions.

**Status of this comparison.** Alternative 1 was implemented and measured (see `IMPLEMENTATION-REPORT.md` and `PERFORMANCE.md`). Alternative 2 was **analysed, not implemented**. Its cost figures below are estimates from the code and the image sizes, and are marked as such.

## Alternative 1 — derived RGBA32 composition (the certified A4 pattern)

Each generated climate or snow tile whose sources carry XR32 data gets an empty XR32 node shell and a **recipe**: light map image, texture image. Whenever a cache is built from the tile (screen cache, base-size runs, zoom cache), its raster is composed in 8 bits into memory owned by that call, with:
- exactly the pixel set of the 16-bit tile (a copy of the light map);
- the same texture placement;
- `out = min(255, (light8 * tex8 + 64) >> 7)` per channel, where each input is its XR32 colour if it has one, otherwise its RGB555 word expanded as the palette shows it.

This is how A4 composes the coast water today (`stlab_xr32_derive_water_tile`, `stlab_derive_compose`).

The transition masks (C) get the same treatment for their *values only*. The mask keeps the coverage the legacy warp gives it, and inside that coverage the 5-bit value is replaced by the 8-bit value of the same source pixel of the XR32 `SlopeTrans` / `ShoreTrans` plane. The blend then has 256 levels instead of 32.

| Criterion | Assessment |
| --- | --- |
| Fidelity | 8-bit end to end; the product is the same operation as the legacy one at 8 bits (neutral 128 ≡ 16) |
| Geometry | unchanged by construction: same pixel set, same placement; at 2/1 the legacy outline already applies to every derived tile (A11); masks keep the legacy coverage |
| Memory | no stored rasters (recipe of 3 ids per tile); the screen caches are the same size as the legacy ones (ARGB in both cases) |
| Performance | composition only when a cache is built (zoom step, light change, first view), never per frame; measured in `PERFORMANCE.md` |
| Maintenance | one recipe type beside A4's; no change to `ground_desc.cc`'s algorithm, only one call per generated tile, as A4 |
| Traditional paksets | a tile whose sources have no XR32 data is not derived: byte-identical legacy path |
| DAY/NIGHT | a source without a night plane is darkened by the XR32 night LUT, the same multiplicative arithmetic as the palette applies to the legacy tile; a source with an XNEX night plane composes its night the same way |
| Zoom | the existing native XR32 zoom, plus the A11 2/1 outline for derived tiles |
| Format | **no change**: it only reads XR32 v1/v2 data that makeobj already writes |

## Alternative 2 — reuse the traditional drawing (prissi's proposal)

No composed climate images. At draw time:
1. the climate texture is drawn cut to the slope's shape;
2. the light map is applied over it.

Transitions and snow keep their masks and painting order.

| Criterion | Assessment (analysis) |
| --- | --- |
| Fidelity | the legacy light map is a **multiplier with gain up to ≈ 1.8** (pak128.german light values up to 231/128). Simutrans has no multiply blend: alpha-over can only darken toward a colour or lighten toward one, never scale a channel above 1. A faithful result needs a **new blend operation** (multiply with gain) in the blitter. |
| Geometry | the texture and the light map are zoomed separately; the cut of (1) must match (2) pixel for pixel at every zoom (2/1 outline rule, 3/4 box filter), or seams appear on every ground tile |
| Memory | saves the composed tiles: for pak128.german ≈ 520 climate and snow tiles × ≈ 8 k px. That is ≈ 8.5 MB of 16-bit words, plus ≈ 17 MB of 32-bit screen caches (estimate) |
| Performance | **two full-screen passes** (cut texture plus multiply) on the ground, which covers most of the screen, **every frame** a region is redrawn; today the same pixels cost one copy of a cached run (estimate: the ground pass at least doubles) |
| Maintenance | a new blitter op in simgraph32 (and in simgraph16 if both renderers must agree); changes in `grund.cc`'s drawing order; the ground image ids that other code relies on (`get_ground_tile`, `free_all_images_above`) change meaning |
| Traditional paksets | the 16-bit renderer would either keep the old path (two code paths for the same ground) or change its look |
| DAY/NIGHT | the multiply would have to be applied before the night LUT, per frame |
| Format | none needed |

## Decision

**Alternative 1** for A, B and C:
- it keeps the full 8-bit precision;
- it changes no geometry, no format and no per-frame cost;
- it reuses a certified mechanism (A4, its texture cache and A11).

Alternative 2 would save memory, but it:
- needs a new blend operation;
- moves the work to every frame;
- makes the seam-free zoom of two separately zoomed images a new correctness problem.

None of that is needed for precision. If the memory of the composed tiles ever becomes the limit, Alternative 2 is a separate architecture decision for the maintainers (**NEEDS_MAINTAINER_DECISION**), and it is not part of A12.

Not chosen either: storing the composed 8-bit rasters. That would cost ≈ 34 MB for pak128.german (4 B/px of base raster on top of the caches) and gain nothing over composing at cache build, which A4 already showed is cheap.
