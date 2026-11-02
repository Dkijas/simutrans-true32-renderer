# A4 — root cause: sloped-coast water has no 32-bit detail

Verdict of the discovery: **REAL**, present in XR32 testing-3 as published (measured below). The testing-3 tree has no
XR32 code in `ground_desc.cc`, `grund.cc` or `wasser.cc` (`git grep stlab df2a1b0`: 0 hits), so nothing in it can
give a generated tile a node. Earlier testing builds were not measured here.

## Symptom

With `SIMUTRANS_XR32=1` and a pak whose sea has an XR32 node, the open sea and the flat coasts show the 32-bit water,
but the water strip of every **sloped** coast shows the legacy RGB555 water. In a world view this is a visible seam:
a strip of the 16-bit colour set (58 colours in the makie water) next to the 32-bit sea (1,063 colours).

## Cause

1. Sloped-coast water is not a pak image. `grund.cc:1110` draws
   `ground_desc_t::get_water_tile(slope, stage)`, which is one of the tiles that `init_ground_textures()` **generates**
   (`ground_desc.cc:839-858`), from the sea image's 16-bit words.
2. The generation runs in `create_texture_from_tile` and `create_textured_tile` on `STORED_PIXVAL`. The result is
   registered as a plain 16-bit image. No XR32 node exists for it, and nothing gives it one.
3. With no node, `stlab_xr32_recode()` does not serve it. The legacy path draws it: RGB555 expanded `c << 3`, darkened
   by the legacy palette.

The flat coast does not have the defect, because it draws the pak sea image itself.

## Measured (testing-3 product binary, `evidence/rootcause-probe-t3.txt`, `tools/coastprobe.py` under gdb)

| | XR32 on | XR32 off |
| --- | --- | --- |
| pak images with an XR32 node (`pak128.german-mutant`) | 54 | 0 |
| generated coast water ids (67 slope images × 32 stages) | 2,144 | 2,144 |
| … of them with an XR32 node | **0** | 0 |

- Composition oracle (`evidence/composition-oracle-mutant-1x1.txt`): at 1/1, testing-3's tiles match the
  **16-bit** model exactly (195 of 195), and differ from the 32-bit model in 192 of 195 tiles.
- `pak128.german-mutant` paints the XR32 sea in the key colour 201,7,133, which legacy water cannot produce. testing-3
  draws **0** key-colour pixels on sloped coasts; the candidate draws 588,414 in the same 195 tiles.
- Makie water (`evidence/composition-oracle-makie.txt`), 1/1, day:

  | | px with a channel off the RGB555 grid | distinct colours |
  | --- | --- | --- |
  | testing-3 | 0 % | 58 |
  | candidate | 71.9 % | 1,063 |

- In a world (`out/cworld-co/diff_vs_t3.png`, fixed map `coastworld`, forced day, mutant pak):
  - every pixel that differs between testing-3 and the candidate lies on a sloped-coast strip (13,512 px);
  - the candidate adds 3,413 exact key-colour pixels and removes none.

## Why it was not noticed earlier

- The XR32 certification gates draw pak images: catenary, vehicles, the A1/A2 zoom bars, the identity pak.
- The default `pak128.german` has no XR32 water at all. There the defect cannot be seen, and the candidate is
  byte-identical to testing-3 (`evidence/coast-plain-identity.txt`).

## Related findings, not part of A4 (reported, not changed)

- **All other generated ground tiles are 16-bit as well:** climate textures on slopes, snow, and the alpha climate
  transitions (`create_textured_tile` non-binary, `create_alpha_tile`). A pak with XR32 grass would show the same kind
  of seam. Same cause, wider scope: a separate mission.
- **The beach masks** (`ShoreTrans`) are 5-bit weights. `draw_alpha` blends with them, so the edge of the coast keeps
  32 blend steps even when the water is 32-bit. This is a property of the pak format, out of scope.
- **H1 (inherited):** at 2/1, an XR32 image's edge coverage differs from the 16-bit scaler's. This holds already for
  the pak sea images in testing-3: 13,176 px over 54 images, XR32 on against off (`evidence/coast-2x1-inherited.txt`).
  Derived tiles go the same XR32 path and show the same property: 52,026 px over 195 tiles against testing-3's
  16-bit tiles. There is no geometry difference at any zoom.
- **Freed images keep their XR32 slots** (`simgraph32_free_all_images_above`). The only caller that frees XR32 images
  is a second `init_ground_textures()`, which the game never makes, so it has no effect in play. The candidate makes
  the release explicit; see `A4-MAINTAINER-REVIEW.md`.
