> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# Independent oracle

**Tools:** `tools/t7_oracle.py` (tiles), `tools/oracle_trans.py` (transitions), `tools/z_check.py` (zoom, players, night). They read the pak files alone and compare with what the unmodified executable drew under `tools/gdbgate.py`. They never call or copy the corrected function. The keys come from `image.cc`; the plane from the pak's bytes, parsed by `tools/pak_keys.py`.

Summary per run: `evidence/matrix-summary.txt`. Per run: `evidence/oracle-t7-*.txt`, `evidence/oracle-trans-*.txt`.

## The models

For each tile the oracle computes two pictures:

- **D (derived):** per channel `min(255, (l8*t8 + 64) >> 7)`. `l8`, `t8` = the plane where its alpha > 0; otherwise the word: an opaque key = the key's RGB, else word << 3. `231,255,255` becomes `232,255,255`.
- **L (legacy):** per channel `min(31, l5*t5/16) << 3` on the raw words. A key word reads as 0, 0, key.

It then classifies what was drawn as D, L, both, or X (neither), and compares it with the **specification**: expected D unless neither source has XR32, a source is translucent, a source is a v2 node with runs. Transitions use the 32-level mask stream with the model's colours.

## Results at 1/1

Tile counts of 520; 2,171,424 tile pixels. `L` = tiles left at 16 bits. "Keyed" = tiles with a key in a source.

| Pak | What it holds | Keyed | testing-6 | `defca456` | Candidate SDL3 / SDL2 | Candidate transitions (M2, 979,368 px) |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| PAK-OLD, PAK-NEW | makie's real files | 0 | 0 L | 0 L | 0 L | 0 off |
| V1 (corner lab) | 8 corner rows, keys 9B, B3 | 64 | **64 L** | | 0 L | 0 off |
| V3 | V1, no XR32 in the light | 64 | **64 L** | | 0 L | 0 off |
| N0 | PAK-NEW rebuilt | 0 | 0 L | 0 L | 0 L | 0 off |
| NK | 31,022 px exactly `6B`, `9B`, `4D` | 304 | **304 L** | 0 L | 0 L | 0 off |
| NK1 | The same pixels, one level off | 0 | 0 L | 0 L | 0 L | 0 off |
| LA | All 31 keys, one per light row | 520 | **520 L** | 0 L | 0 L | 0 off |
| LA1 | One level off | 0 | 0 L | 0 L | 0 L | 0 off |
| LB | Alpha 250 and 254 in the light, no keys | 0 | 0 L | 0 L | 0 L | 0 off |
| TK | Keys by class in 4 climates + arctic | 390 | **390 L** | **390 L** | 0 L | 0 off |
| TK1 | One level off | 0 | 0 L | 0 L | 0 L | 0 off |
| LT | Keys in both | 466 | **466 L** | **390 L** | 0 L | 0 off |
| TKX | TK, texture without XR32 | 390 | 385 L + 5 both | 385 L + 5 both | 0 L | 0 off |
| LX | NK, light without XR32 | 304 | 304 L | 0 L | 0 L | 0 off |
| TW | Keys in the water row only | 0 | 0 L | 0 L | 0 L | 0 off |
| TM | Declared company mask, v2 node | 65 | 65 L | 65 L | **65 L, by design** | M1/M2 mix, see below |
| BX | Keys in both, no XR32 in either | 466 | 513 L + 7 both | the same | **the same, by design** | M1: 0 off |

All runs PASS against the specification of their own executable. On TM, M2 does not match because one climate of the pair is a 16-bit tile by design; on BX M1 matches exactly.

## What each requested case found

| Requested | Result |
| --- | --- |
| `#6B6B6B`, `#9B9B9B`, `#B3B3B3`, other keys | All 31 placed; exact |
| Ordinary RGB | The one-level-off paks: exact, unchanged from testing-6 |
| Legitimate company colours | Images drawn as such: 0 of 50,823 differ. TCC fixture: 0 of 4 |
| Night special colours | Same images at light level 4; in a source: darken as ordinary colours |
| Pixels with various alpha | 250, 254 in the light: opaque, exact. 128 in the texture: refused, identical to testing-6, no crash. 128 in the light: crash at load, in r12377 too |
| Resources without XR32 | LX, TKX, V3 exact; BX and the 2.4 pak: inherited |

## Zoom, players, night

`evidence/z-check.txt`, all on the candidate, SDL3 and SDL2.

| Check | Result |
| --- | --- |
| Tiles on the RGB555 grid with keys, 2/1, 9/8, 3/4, 1/2 | NK 0, LT 0, of 520, at each zoom |
| NK against NK1 at each zoom | Same pixel set in all 520 tiles; largest channel difference 4 |
| Player 3 against player 0 | 0 of 520 differ (NK1, LT) |
| Night, LT, against the map learned from NK1 | 0 values off among 2,171,424 px (1,146 channel values unknown to the map, not judged) |

## Negative controls

**Engine mutants**, disposable branches `t7mut/*`, never in the certification tree (`evidence/mutants.txt`):

| Mutant | Pak | CONTROL | CANDIDATE |
| --- | --- | --- | --- |
| `rgb555`: a word without plane ignores the key's RGB | LX | **FAIL** (301 tiles X, 3 L) | PASS |
| `rgb555` | TKX | **FAIL** (185 X, 1 L) | PASS |
| `planeoff`: key pixels rounded to 5 bits | NK | **FAIL** (280 X) | PASS |
| `recolour`: a company key in the light takes a company colour | LA | **FAIL** (280 X) | PASS |

**The oracle's failure classes:**

| Must detect | How it was shown | Result |
| --- | --- | --- |
| Unexpected RGB555 fallback | `defca456` against the candidate's spec on TK | Classes L where D is due (390) |
| Colour error | one channel +1 on one pixel of a dump; `planeoff` | DETECTED |
| Improper recolouring | key pixels overwritten with a company colour; `recolour` | DETECTED |
| Alpha change | a hole in the tile | DETECTED |
| Mask alteration | `oracle_trans.py` controls: weight +1, mask 1 px off, order inverted, 5-bit colours, wrong light, row error | DETECTED in every run |
| Geometric shift | tile shifted 1 px | DETECTED |
| Night behaviour change | map off by one level | DETECTED |
| Grid census | testing-6 x V1 at 2/1 | 64 tiles found |

## Limits

- The D model has the same formula as the engine by design: A12 specified it, and the oracle checks the implementation, not whether the formula is the right art decision.
- 1/1 is checked against the model; the other zooms against the keyless control, plus byte identity on real paks. No independent model of the zoom filter was written here.
- Only two climate pairs and 19 slopes are in the transition oracle.
