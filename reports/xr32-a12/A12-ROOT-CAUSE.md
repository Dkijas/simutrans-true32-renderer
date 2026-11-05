> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# A12 — root cause: the generated terrain loses its RGBA32 precision

Base: XR32 testing-5, public commit `cfcc03c6`, functional source `038b538` (tree `c8297dca`), official SVN r12377.
Every number on this page was measured on this base, with the controls listed. Evidence files are in `evidence/`.

## 1. Premises checked first

| Premise of the brief | Result |
| --- | --- |
| Tag `xr32-prissi-night-image-prototype-01-r12377-testing-5` → `cfcc03c6` | confirmed: anonymous `ls-remote`; `source/xr32-r12377` == tree `c8297dca` == lane base (`evidence/base-identity.txt`) |
| The four functions in `descriptor/ground_desc.cc` | present: `create_textured_tile` (l. 40), `create_alpha_tile` (107), `create_texture_from_tile` (239), `init_ground_textures` (450) |
| `https://pak128-german.de/PAK128german_2.5_beta.zip` | **404.** The file is at `PAK128.german_2.5_beta.zip` (with a dot): 814,028,683 bytes, `Last-Modified` 2026-11-03 16:03:49 GMT, SHA-256 `5220f4d4…` (`evidence/ext-pak128german-2.5beta.sha256`). |
| BASE executables == testing-5 certification | SDL3 `f7f7ce82`, SDL2 `e82a2ce8`, makeobj `8a150381` rebuilt byte for byte; 16-bit `8feaaf49` (`evidence/base-exes.txt`) |

## 2. Makie's resource carries 8-bit terrain

The pak contains compiled paks only, with no PNG or `.dat` sources. Its terrain paks were written with the XR32 makeobj.
`tools/xr32_census.py` was run on them (`evidence/census-p128g-ground.txt`); its control is a synthetic 555-only plane → 0 % off-grid, and a smooth plane → 97 %.

| Pak | XR32 nodes | pixels off the RGB555 grid | colours, 8-bit → as RGB555 |
| --- | ---: | ---: | --- |
| `ground.ClimateTexture` | 8 of 8 (sea + 7 climates) | 99.5 % | 274…2494 → **12…69** per texture |
| `ground.LightTexture` | 81 | 100 % | red levels per slope image: median 3; gradient slopes 43…73 → **7…11** (`evidence/A-lighttexture-levels.txt`) |
| `ground.SlopeTrans` | 15 | 99.6 % | 441…543 → 62…73 |
| `ground.ShoreTrans` | 12 | 100 % | 436…898 → 79…124 |

So the 32-bit data exists in the pak. The question is where the engine drops it.

## 3. The pipeline, PNG → framebuffer

```
PNG ──makeobj──► IMG node (RGB555 words, RLE)  +  XR32 node (RGBA8888 plane)          [per pak image]
                      │                                   │
                 pakset load                       stlab_xr32_attach (slot.img)
                      │                                   │
ground_desc_t::init_ground_textures()  ◄── reads ONLY the IMG words ───────────────────────────────┐
  ├─ sea tiles:     create_texture_from_tile + create_textured_tile(binary)  ── A4: derived XR32 ✔ │
  ├─ climate tiles: create_textured_tile(light[slope], ClimateTexture[c])    ── 16-bit only ✘ (A)  │
  ├─ snow tiles:    create_textured_tile(light[slope], ClimateTexture[arctic]) ─ 16-bit only ✘ (A) │
  ├─ alpha_image[]  (snow line)    : create_alpha_tile(light, slope, SlopeTrans rot.) ─ 555 mask ✘ (C)
  ├─ alpha_corners_image[] (climate transitions): create_alpha_tile(…, SlopeTrans rot.) ─ 555 ✘ (C)
  └─ alpha_water_image[] (beach):  create_alpha_tile(…, ShoreTrans rot.)            ─ 555 mask ✘ (C)
            │ register_image() → images[id].base_data (RGB555 words)
            ▼
grund_t::display_boden (grund.cc 1040-1140)
  ├─ draw_img(get_ground_tile)                         climate / snow / water tile
  ├─ draw_alpha(climate tile of neighbour, alpha_corners, ALPHA_GREEN|BLUE)   transitions
  ├─ draw_alpha(water tile, alpha_water, ALPHA_RED)                           beach
  └─ draw_alpha(snow tile, alpha_image, ALPHA_GREEN|BLUE or BLUE)             snow line
            ▼
simgraph32: image with an XR32 slot → stlab_xr32_recode / zoom (8-bit); otherwise the RGB555 words expanded
draw_alpha → decode_alpha_rows: 5-bit plane (0..31) → blend_alpha32_argb, 32 levels
            ▼
framebuffer (ARGB8888)
```

- **A12-A, ClimateTexture.** `create_textured_tile` computes, per channel, `min(31, light5 * tex5 / 16)`. Both inputs are 5-bit, the output is 5-bit, and the generated tile gets no XR32 node. The 8-bit counterpart is `min(255, light8 * tex8 / 128)`: neutral light is 16/16 in 5 bits and 128/128 in 8 bits, so the scale is identical.
- **A12-B, LightTexture.** The same multiplication reads the light map's RGB555 words, so its gradients are quantised to 32 levels before the product. The light's role and geometry are otherwise unchanged: the tile is a copy of the light map's pixel set.
- **A12-C, transitions.** The masks are built by copying 555 words through `create_alpha_tile`'s warp, from rotated or flipped copies of `SlopeTrans` / `ShoreTrans`, which are not registered images. At draw time `decode_alpha_rows` sums the selected 5-bit channels and the blend has 32 levels. The images blended through the masks (climate, snow and water tiles) are themselves the tiles of A and B.
- **Day/night.** The legacy tile is darkened by the palette, and an XR32 image without a night plane is darkened by the XR32 night LUT with the same multiplicative arithmetic. None of the terrain sources in makie's pak has an XNEX night node.
- **Zoom and cache.** The legacy tiles are zoomed by `rezoom_img32` from their words. A derived XR32 tile (A4) is zoomed natively; at 2/1 it takes the legacy outline (A11), and that applies to **any** derived tile (`stlab_xr32_zoom.cci:647`).
- **Resources without XR32 data** (every pakset built with an ordinary makeobj) go through exactly the legacy path; nothing above changes them.

## 4. Measured on the engine (BASE, makie's pak, day, 1:1, 1920×1080)

A fixed test world was built for this mission and frozen as `save/terrainA12scene.sve`. The game's own tools built it, through a lab scenario (`scene/stlab_a12/scenario.nut`, `out/scene-build/script-scenario.log`):
- a pyramid from the sea to z 11;
- 182 tiles of each single slope and 14 of each corner slope;
- a steep cone of double slopes;
- seven climate bands across the faces, desert to arctic.

| | result (`evidence/A-rootcause-engine.txt`) |
| --- | --- |
| world-area pixels exactly on the RGB555 grid (XR32 BASE) | **88.11 %** |
| those pixels compared with the 16-bit renderer's frame (quantised to RGB565) | **100.00 % equal** |
| off-grid pixels | 11.89 %: sea and A4 coast water (bluish), plus exactly the climate and beach transition bands (mask in `scratch/A-offgrid-mask.png`): two RGB555 colours blended in 32 levels |
| distinct colours | XR32 BASE 12,019 (mostly blend levels), 16-bit 1,259 |
| CONTROL | +3 on every red channel → 0.01 % on the grid (was 88.11 %) |

**Root cause.** Every terrain picture the engine generates is composed from the 16-bit words of its sources, even when the sources carry 8-bit XR32 data:
- `init_ground_textures` reads only `IMG` data, and `create_textured_tile` multiplies 5-bit by 5-bit into 5 bits;
- the generated climate and snow tiles get no XR32 node, so the XR32 renderer shows them as RGB555;
- the transition masks are 5-bit, and the blend through them has 32 levels.

In makie's resource this turns textures of 274…2494 colours into 12…69. That is the banding makie reported.
Not a cause: the zoom, the night LUT, the screen cache and the dirty-region upload all handle 8-bit data already; the A4 sea tiles prove it.
