# A4 — design alternatives

## Requirements

From the brief:
1. sloped-coast water uses the pak's RGBA32 sea when `SIMUTRANS_XR32=1`;
2. the 16-bit renderer is unchanged;
3. traditional paks keep working;
4. DAY / NIGHT, zoom and dirty regions are correct;
5. memory and CPU stay reasonable;
6. the patch covers A4 only.

Numbers for `pak128.german` water (32 stages), from `evidence/rootcause-probe-t3.txt`:
- 2,144 generated tile ids (2,080 of them real water tiles);
- 21,036,480 bounding-box pixels;
- 9,297,281 stored 16-bit words, which is 17.7 MB.

## Alternatives

| | Idea | Memory | CPU | Touches | Verdict |
| --- | --- | --- | --- | --- | --- |
| **A. Eager nodes** | At `init_ground_textures`, compose a full RGBA node per tile (and a night plane if the sea has one) and attach it | **+80.2 MB** day, **+160 MB** with night planes, held all game | one composition per tile at start (2,080 × ~0.2 ms ≈ 0.4 s) | ground_desc.cc + an attach call | rejected: four times the 16-bit tiles' memory for pixels mostly never drawn |
| **B. Recipe + composition when a cache is built** (chosen) | Store per tile only `{light map, sea, texture}` and a geometry shell; compose the raster into a buffer owned by the cache build, then free it | measured peak working set +0.8 / +1.1 MB (SDL2 / SDL3) on the coast map. Map entries and shells are about 0.25 MB; the rest is the derived tiles' caches and composition buffers | ~0.2 ms per (tile, zoom, light) cache build; 0 in a steady view (measured) | ground_desc.cc (11 lines, `COLOUR_DEPTH == 32`), the XR32 hook (`source()` in front of the three cache builders), one release call | **chosen** |
| C. Change the draw call | In `grund.cc`, draw the pak sea image through the light map and the beach mask at draw time (a double mask), instead of a generated tile | none | a second mask per coast tile per frame, in both renderers | `grund.cc`, simgraph16 and simgraph32 (`draw_alpha` with two masks) | rejected: changes the 16-bit renderer and the traditional paks' picture; the five pasted copies of `create_texture_from_tile` are not a mask, so the picture would also change |
| D. B + a raster cache | Keep composed rasters in an LRU | bounded by the LRU | saves ~0.2 ms per cache rebuild | B + an eviction policy | not needed: the measured cost is ~0.6 compositions per frame while the animation first fills, then 0 |
| E. In the pak | Ask paksets to ship XR32 nodes for coast tiles | — | — | the pak format | impossible: the tiles are generated in the game from slope × stage; they do not exist in any pak |
| F. Recolour the 16-bit tile | Map each legacy colour of the tile to the sea's XR32 colour of the same legacy value | small | small | the XR32 hook | wrong: the map is many-to-one (58 legacy colours against 1,063 XR32 colours in makie), so the detail cannot be recovered |
| G. Every generated ground tile | The B mechanism for climates, snow and alpha transitions too | B per kind | B per kind | wider | out of A4 scope; B's recipe idea extends to them (separate mission) |

## Why B, and how it is kept small

- The raster is **composed, not converted**. It replays steps 1–2 of `init_ground_textures`:
  - the same order and the same bounds;
  - the same five copies;
  - the same binary light-map test.

  The candidate's pixel set therefore equals the 16-bit tile's. This is proven at every zoom by
  `evidence/coast-zoom-geometry.txt`; the 2/1 edge difference there is H1, inherited.
- **No new path.** The composed raster enters the existing cache builders, so every certified XR32 behaviour
  applies to coast tiles without new code:
  - resolve, night planes, darkening, zoom, `draw_alpha`;
  - dirty regions, which are framebuffer level and indifferent to where a colour comes from.
- **Fail closed.** The tile keeps its 16-bit picture, exactly as in testing-3, in any of these cases:
  - a sea without its own XR32 plane;
  - geometry that does not match the light map;
  - a texture that is not at x = 0;
  - special colours in any input;
  - XR32 off.

  This is why traditional paks are byte-identical (`evidence/coast-plain-identity.txt`).
- **16-bit renderer untouched.** The registration is under `#if COLOUR_DEPTH == 32`. The 16-bit binaries are
  byte-identical to testing-3's on Linux, and the same program on Windows (`A4-REGRESSION-MATRIX.md`).
- **The slot struct is unchanged** (40 bytes, the layout the gdbgate tools read). The recipe lives in a separate map.
