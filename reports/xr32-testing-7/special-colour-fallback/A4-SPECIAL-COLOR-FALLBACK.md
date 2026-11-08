> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# A4: coast water and special colours

**Verdict: confirmed, same cause, for the light map only. Experimental patch made and measured, kept apart, not merged. It needs its own certification.** Evidence: `evidence/water.txt`. Patch: `patch/EXPERIMENTAL-A4-WATER-LIGHT-KEYS-NOT-MERGED.diff`, branch `t7x/water-light` (+4 −1 on the candidate).

## The reproduction

Water tiles drawn alone, 2 of the 32 animation stages × 65 slopes = 130 tiles, at 5 zooms and at light level 4 with day/night on. World `coastworld`. Candidate as it stands (the same gate as testing-6 for water):

| Pak | Tiles with 32-bit picture | Tiles on the RGB555 grid |
| --- | ---: | ---: |
| PAK-NEW, no keys | 130 | 0 |
| NK, keys in 38 light rows | **54** | **76** |
| NK1, the same pixels one level off | 130 | 0 |
| LA, one key in every light row | **0** | **130** |

The same numbers at 1/1, 2/1, 9/8, 3/4, 1/2. A keyed pak drops those slopes of the water to the 16-bit picture, **in all stages**.

**The real files are not affected:** PAK-OLD and PAK-NEW hold no key, and all their water tiles have the 32-bit picture.

## The questions

| Question | Answer |
| --- | --- |
| Same cause? | **For the light map, yes:** `stlab_derive_plain( light )` in `stlab_xr32_derive_water_tile()` |
| Are all the water gates coincidences? | **No.** This is the important difference. A water tile is made in **binary** mode: `if (*dest) { *dest = mix; }`. The light map is read as "black or not", so a key there means nothing. But the sea's or the water texture's word is **copied** into the tile, where a key really is a special colour. Those gates must stay |
| Reusable fix? | Yes, for the light map: one word, `stlab_derive_source_usable( light )`, and a forward declaration |
| Independent patch needed? | Yes: this mission's D4 and the risk below |
| Risk for A4 and A11 | Low by reading, not nil: more tiles go through the A4 composition and its phase cache, and that was not measured under load |
| Geometry and paint order | Kept: the pixel set comes from the tile's own stream, and the paint order is not touched |

## What the experiment gave

SDL3 and SDL2.

| Check | Result |
| --- | --- |
| NK: tiles with a 32-bit picture | 130 of 130; 0 on the grid |
| LA | 130 of 130; 0 on the grid |
| **NK against NK1:** a light map read as black or not must make no difference | **0 of 130 differ**, at 5 zooms and at night, both backends |
| CONTROL: the same comparison on the candidate | 76 of 130 differ: the check can fail |
| PAK-NEW: experiment against candidate | 0 of 130 differ |
| TW, keys in the water texture (real special colours) | 0 of 130 differ from the candidate: 38 tiles still refused, on the slopes that read those words |

That "NK against NK1" comparison is a good oracle: it needs no model, because the two paks must draw the same water.

## What was not done, and why it is not merged

- All 65 slopes × 32 stages: 2 stages of each slope were drawn.
- Whole-world frames, coast at dusk, climate transitions near a coast on the experiment.
- Performance: the phase cache of A4 under keyed compositions.
- Dirty-region verification on the experiment.
- The A4 composition oracle (`coast_check.py`) on the keyed paks.

## Recommendation

Certify it as its own small mission, for testing-8 or a later test build. It matters only for a pak with keys in the light map, and PAK-NEW has none, so nothing forces it into testing-7.

**Consequence for testing-7:** a pak with keys in its `LightTexture` gets 8-bit land and 16-bit coast water on those slopes. The release notes say so.
