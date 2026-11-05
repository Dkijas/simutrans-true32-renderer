> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# A12-D — transition masks: technical sheet

**Status:** limitation **accepted for testing-6** (maintainer decision). Not implemented. Not part of the testing-6 candidate.

## 1. Current limitation

`ground_desc_t::init_ground_textures()` builds three kinds of mask with `create_alpha_tile()`. Each one is a warp of a rotated or flipped copy of `SlopeTrans` / `ShoreTrans` into a slope's shape:

| Mask | Count (pak128.german 2.5 beta) | Used for | Channels read |
| --- | ---: | --- | --- |
| `alpha_image` | 64 | snow line | G+B, or B |
| `alpha_corners_image` | 975 (15 per slope) | climate transitions | G+B |
| `alpha_water_image` | ≤ 975 | beach outline | R |

`grund.cc` draws the neighbour's climate tile, the snow tile or the water tile *through* the mask (`draw_alpha`). The 32-bit renderer decodes the mask's **RGB555 words into 5-bit weights** (`decode_alpha_rows`, 0..31) and blends with **32 levels** (`blend_alpha32_argb`).

After A12:
- the **images** blended through the masks are 8-bit (the derived climate and snow tiles; the A4 water tiles);
- the **weights** are still 5-bit.

A12 therefore does not give full 8-bit terrain: the transition weights are the remaining 16-bit part.

## 2. Measured difference

The data is makie's real masks (`evidence/F-*`, and the A12 lab's `C3-mask-weight-error.txt` and `C3-simulation.txt`).

| | |
| --- | --- |
| Weight error, 5-bit vs the 8-bit mask, transition pixels | mean 1.4 %; maximum 2.9 % (one channel), 4.7 % (G+B) |
| Colour error at the pak's highest contrast (rock → snow, 179 levels), offline simulation with the real textures and masks | mean 2.4 levels, max 5–11 |
| Visible? | not at ×3 in that simulation; the contour steps appear only when the difference is amplified ×20 |
| In the engine (Phase B of testing-6) | the transition bands of the candidate show no RGB555 grid signature: their inputs are 8-bit. See `VISUAL-COMPARISON.md` |

## 3. Memory if the masks were stored at 8 bits

Exact count over makie's 65 slopes (`evidence/F-mask-memory.txt`):

| Item | Pixels |
| --- | ---: |
| snow line | 267,332 |
| climate corners | 4,071,420 |
| beach | ≤ 4,071,420 |
| **total** | **≤ 8.4 M** |

| Representation | Size |
| --- | --- |
| 1 byte per pixel (only the channel each use reads); 2 bytes for the snow line (G+B and B) | **8.0–8.3 MB** |
| full RGB | 23.8 MB |
| for comparison, the existing 16-bit words of the same masks | 16.0 MB (they stay; the renderer and the 16-bit renderer still need them) |

## 4. On-demand recomposition (no stored planes)

The mask value of a pixel is the source pixel the warp picks. On demand it could be:
- **(a) recomputed per draw:** the warp of `create_alpha_tile`, about 20 integer operations per pixel, several times the cost of today's `decode_alpha_rows` and paid on every transition drawn;
- **(b) kept in a small LRU** of recently used 8-bit masks: some MB, a recomposition only on a miss. This is the A4 texture-cache pattern.

Both need the XR32 planes of the 15 `SlopeTrans` and 12 `ShoreTrans` sources kept, ≈ 1.7 MB, with the rotation and flip mapping replicated exactly.

## 5. Risks to A11 / H1 and to the geometry

- **Zoom.** The mask is zoomed by the 16-bit resampler (`rezoom_img32` on its words) and decoded at the zoomed geometry. An 8-bit mask must keep **exactly the same coverage** at every zoom, or seams appear on every transition.
- **Beach masks at 2/1 (`ShoreTrans`).** These are exactly the outline A11 aligned the coast water tile to. Any change in their 2/1 coverage reopens A11.
- **Attaching an XR32 node to a mask image** would route its zoom to the XR32 path and change what `build_alpha_plane` reads (`current_stored`). It must not be done; the 8-bit values must live beside the mask, keyed by image id.
- **Blend arithmetic.** Today `> 30` means opaque and 16..30 is shifted by one, a 5-bit contract. An 8-bit path needs its own blend operation (`/255`), in both families (`draw_rezoomed_img_alpha` and `draw_base_img_alpha`).

## 6. Design alternatives

| # | Design | Memory | Per-frame cost | Risk |
| --- | --- | --- | --- | --- |
| M1 | 8-bit planes stored at load, coverage taken from the 16-bit mask at draw time (8-bit value only where the 16-bit weight > 0) | +8.0–8.3 MB | ≈ same as today | low at 1/1; zoom needs an 8-bit resample matching the 16-bit coverage |
| M2 | M1 plus an 8-bit zoom of the planes through the XR32 zoom core (same footprint rules) | +8 MB + zoom caches of the masks in view | zoom builds | medium: coverage equality must be proven at 2/1, 9/8, 3/4, 1/2 |
| M3 | LRU of composed 8-bit masks (on demand) | 1.7 MB sources + a few MB | warp on miss | medium |
| M4 | warp per draw | 1.7 MB | every transition draw | low memory, measurable frame cost |
| M5 | do nothing (testing-6) | 0 | 0 | 0, and the measured gain is below visibility |

## 7. Recommendation for the roadmap

- **Keep M5 for testing-6.**
- Revisit only if testers report contour steps in transitions, especially on high-contrast pak art (rock/snow, sand/water). The evidence so far says the gain is below visibility.
- If revisited, start with **M1 at 1/1 only, behind the existing derived-tile switch**, and certify its coverage against the 16-bit mask at every zoom before extending it. Leave `ShoreTrans` (A11) for last, with A11's own 2/1 gates.
- **Do not claim 8-bit terrain masks for A12 or testing-6.**
