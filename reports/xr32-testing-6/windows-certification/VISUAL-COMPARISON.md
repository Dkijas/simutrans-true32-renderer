> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# testing-6 candidate — visual comparison (Phase B) and brightness (Phase C)

**Resource:** makie's PAK128.german 2.5 beta, `https://pak128-german.de/PAK128.german_2.5_beta.zip`, downloaded again for this mission.
- SHA-256 `5220f4d4…`, 814,028,683 bytes, `Last-Modified` 2026-11-03 16:03:49 GMT;
- identical to the copy A12 used;
- the URL without the dot still answers 404 (`evidence/A-makie-resource.txt`).

The crops contain makie's art: **local review only, not for redistribution.**

**Captures:** straight from the engine's framebuffer (`tools/t6_captures.sh` → `tools/gdbgate.py`, executables unmodified). The pixels are not edited; crops are only cut and enlarged ×3 by pixel repetition.

| Setting | Value |
| --- | --- |
| Pakset, save, camera | the same for all three configurations, positioned by the game's own `viewport_t::change_world_position` |
| Resolution | 1920×1080 |
| Zoom | 1/1 (and 2/1 on the pyramid) |
| Light | forced day (darkness 0), darkness 2 and darkness 4 |

| Configuration | Executable |
| --- | --- |
| A — traditional 16-bit renderer | `8feaaf49` (the candidate's 16-bit build = BASE's) |
| B — XR32 testing-5 | `f7f7ce82` (= the published testing-5 executable) |
| C — XR32 A12 candidate | `ed58e66d` |

| Scene | World and view | Shows |
| --- | --- | --- |
| flat | makie's demo world, tile 100,40 | flat grass |
| pyr | the A12 terrain world, 16,16 | single and corner slopes, seven climate bands, transitions, snow |
| cone | the A12 terrain world, 56,56 | double slopes, lit/shaded faces, coast |

## Metrics (`evidence/B-visual.txt`)

"Terrain" = the pixels A12 changes against testing-5. Only generated ground tiles can change: every other XR32 image is drawn byte-identically by day and at night (`evidence/D-drawall.txt`).

- `grid555`: share on the RGB555 grid.
- `steps8`: share of the colour steps that are whole multiples of 8 (banding).

**Control:** A12 quantised to RGB555 reads 100 % / 100 % in every scene.

| Scene, light | Terrain changed | testing-5: colours / grid555 / steps8 | A12: colours / grid555 / steps8 | 16-bit colours | A12 / testing-5 luma |
| --- | --- | --- | --- | --- | --- |
| flat, day | 93.0 % | 26,779 / 96.3 % / 92.4 % | **34,336 / 0.4 % / 0.0 %** | 1,544 | 1.082 |
| pyr, day | 97.3 % | 11,011 / 89.5 % / 80.1 % | **40,673 / 0.6 % / 0.0 %** | 1,232 | 1.104 |
| pyr, 2/1 | 100 % | 4,643 / 88.7 % / 77.9 % | **18,113 / 1.0 % / 0.0 %** | 527 | 1.107 |
| cone, day | 39.2 % | 110,465 / 53.2 % / 37.6 % | **149,853 / 0.2 % / 0.0 %** | 4,354 | 1.084 |
| flat, darkness 2 / 4 | 92.9 / 92.8 % | 15,796 / 7,444 colours | 17,462 / 7,843 colours | 530 / 175 | 1.077 / 1.084 |
| pyr, darkness 2 / 4 | 97.3 % | 8,097 / 4,847 | 18,390 / 7,212 | 431 / 166 | 1.103 / 1.109 |
| cone, darkness 2 / 4 | 38.7 / 38.1 % | 60,605 / 24,930 | 71,778 / 26,765 | 1,301 / 375 | 1.083 / 1.088 |

At darkness 2 and 4 the grid metric does not apply: the night multiplier moves testing-5's colours off the RGB555 grid too. There, compare colour counts and the images.

**Night is applied once.** The A12/testing-5 luma ratio is the same at day, darkness 2 and darkness 4 (1.08–1.11 in each scene). A double darkening of the derived tiles would make it fall at night. Tile-level confirmation in the A12 lab: the night/day ratio of 16 tiles matches BASE within 0.009.

## What the images show (`evidence/visual/t6-<scene>-<light>-16bit-t5-a12-x3.png`)

- **Day.** 16-bit and testing-5 posterise the texture into flat blotches. The candidate shows the continuous texture and continuous shading across the faces (`t6-cone-d0`, `t6-pyr-d0`, `t6-flat-d0`). This is visible at ×3 and on the half-size full views (`t6-*-d0-t5-a12-half.png`).
- **The candidate is visibly brighter** (see the luma factor above).
- **Night.** The scene is dark, so the difference is subtle. The candidate is smoother and a little brighter (`t6-pyr-d4`).
- **2/1.** The same at the closest zoom (`t6-pyr-z2d0`), with the legacy 2/1 outline of the derived tiles (A11).

## Artefacts

| Check | Result |
| --- | --- |
| Geometry | the drawn pixel set of all 520 generated tiles equals BASE at 1/1, 2/1, 9/8, 3/4 and 1/2 (A12 certification; the candidate binaries are the same bytes) |
| Seams | none possible from A12: coverage unchanged |
| Transparent key | the transparent-key nudge (`231,255,255` → `232,255,255`) is exercised by the synthetic sources (27,436 px, A12 lab) |
| SDL2 vs SDL3 | the 520 tile draws are **byte-identical** between the SDL2 and SDL3 builds, for BASE and for the candidate, with makie's and with the synthetic sources (`evidence/D-oracle.txt`) |

## Brightness: is it the original colour? (Phase C, `evidence/C-luminosity.txt`)

1. **The XR32 data is the PNG.** For the synthetic sources, made from PNG files in this lab, the XR32 planes makeobj wrote equal the PNG's RGB on 131,072 of 131,072 texture pixels and 271,444 of 271,444 light pixels.
2. **makie's light map is authored with a slight cool tint:** the flat tile is lit at **(124, 132, 132)**. Over all 81 slope images the range is 50–178, and 58 % of the pixels are above neutral 128 in some channel.
3. **Flat tile, per climate.** The candidate equals the PNG colour × (124, 132, 132)/128 exactly in every climate (ratios 0.967–0.969 / 1.029–1.034 / 1.031–1.037). testing-5 is 0.86–0.91 in red and 0.97–0.98 in green: darker, and shifted in hue (more red lost than green). **The candidate's brightness is the original colour under the pak's own light.** testing-5's darkness was a rounding error.
4. **Clipping.** Only the snow clips: 56.2 % of snow pixels have a channel at 255, every one predicted by the formula (light up to 178 × snow around 245). testing-5 clipped the same pixels (56.0 %, at 248). It is authored and pre-existing; no other climate clips at all.
5. **Snow hue.**

   | | Hue | Saturation | Value |
   | --- | --- | --- | --- |
   | arctic texture | 224° | 0.089 | 0.962 |
   | flat snow tile, candidate | 207° | 0.143 | 0.992 |
   | flat snow tile, testing-5 | 204° | 0.155 | 0.948 |

   The shift towards cyan comes from the authored light tint and is the same in both. On lit slopes the clipped G and B make the snow cyan-white (median hue 180°) in both.
6. **LightTexture transform.** The independent model (`texture × light / 128`, per channel, 8-bit) matches all 520 tiles exactly. Neutral 128 corresponds to the legacy 16.
7. **SDL2 vs SDL3.** No difference (byte-identical tile draws).

No deviation from the approved composition model was found.

**Open: artistic judgement** (`ARTIST-REVIEW-PENDING.md`). The brighter ground is the faithful one, but whether the pak was tuned to the darker look is makie's call. Snow clipping on lit slopes is an authored effect worth his look.
