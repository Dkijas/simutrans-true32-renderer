> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# Special colour fallback: regression on the fresh builds

Phase A of the release gate, on this lane's fresh builds (hash-identical to the certified ones) and testing-6's certified files, SDL3 and SDL2. **60 of 60 runs PASS** against the independent oracle with each executable's own expectation. Evidence: `evidence/matrix-summary.txt`, `evidence/oracle-t7-A-*.txt`, `evidence/oracle-trans-A-*.txt`, `evidence/night-players.txt`, `evidence/night-plane-control.txt`.

**How a path is identified.** Not by colour statistics. Each tile is drawn alone by the unmodified executable under the gate. The oracle builds two pictures of it from the pak files alone: D, the RGBA32 composition, and L, the RGB555 one. It says which of the two the engine drew, pixel for pixel, or that it drew neither. A tile drawn from 16-bit words matches L exactly, so the path taken is read from the drawing.

## The three fixtures

520 tiles per run, 2,171,424 tile pixels; transitions 570 cases, 979,368 px. Same numbers on SDL3 and SDL2.

| Fixture | Keys | testing-6 | Candidate | Candidate transitions |
| --- | --- | ---: | ---: | --- |
| **NK** | 31,022 px of `6B6B6B`, `9B9B9B`, `4D4D4D` in 38 `LightTexture` rows | **304 tiles at 16 bits** | **0**, 520 exact | 0 px off |
| **TK** | 204 px of all 31 keys by class in 4 climate rows + arctic | **390** | **0**, 520 exact | 0 px off |
| **LT** | both | **466** | **0**, 520 exact | 0 px off |

These equal the previous mission's numbers.

## The controls requested

| Control | Pak | testing-6 | Candidate |
| --- | --- | --- | --- |
| `#6B6B6B`, `#9B9B9B`, `#B3B3B3` and the other keys | NK, LA (all 31, one per light row), TK | 304 / 520 / 390 at 16 bits | 0, exact |
| **Neighbours one level away** | NK1, LA1, TK1, TKN1 | 0 at 16 bits | 0; byte-identical behaviour |
| Ordinary colours | PAK-NEW, N0 | 0 | 0 |
| Resources **without XR32** | LX (light), TKX (texture), BX (both) | LX 304, TKX 385, BX 513 | LX 0, TKX 0, **BX 513: inherited, by design** |
| Resources **with night planes** (new here) | TKN: TK with `night_images=1`, a v2 node whose day sheet holds keys as ordinary paint | 390 | **0**, exact |
| Resources with **company data** | TK row 2 (company keys 0–15), LA | at 16 bits | painted colours, **not recoloured**: player 3 = player 0 on 520 of 520 tiles (TKN, TKN1, LT, NK1) |
| **Declared** company semantics | TM (v2 mask) | 65 at 16 bits | **65 at 16 bits, by design** |

## Night

| Check | SDL3 | SDL2 |
| --- | --- | --- |
| TKN against TKN1 at light level 4, both with night planes: pixel set, largest difference | same set, 2 levels | same set, 2 levels |
| CONTROL: the night plane is used (TKN against TK at night) | 520 of 520 tiles differ | |
| Keyed light map at night against the darkening learned from a keyless pak | 0 of 2,171,424 px off | 0 off |
| CONTROL: the learned map off by one level | 6,513,126 values off | the same |
| Day picture of TKN against TK: the night plane does not leak into the day | 0 of 520 differ | 0 |

## Negative controls, re-run here

`evidence/mutants.txt`. Disposable engine mutants (branches `t7mut/*`, never in the certification tree) against the oracle, on this lane's paks. CONTROL must FAIL, CANDIDATE must PASS:

| Mutant | Pak | CONTROL | CANDIDATE |
| --- | --- | --- | --- |
| `rgb555`: a source word without plane colour ignores the key's RGB | LX | **FAIL** | PASS |
| `rgb555` | TKX | **FAIL** | PASS |
| `planeoff`: key pixels lose their plane colour (5 bits) | NK | **FAIL** | PASS |
| `recolour`: a company key in the light takes a company colour | LA | **FAIL** | PASS |

The oracle, on a candidate dump corrupted in memory, detects all five corruptions: a recoloured key pixel, one channel +1 on one pixel, a 1-px shift, a hole (alpha), a 5-bit tile. And on every keyed pak, testing-6 draws L where the specification asks for D, and the oracle reports it.

## Conclusion

The candidate does not confuse an RGB coincidence with special semantics. Where the semantics are real, it leaves them alone:
- in images drawn as such;
- when declared;
- in a binary water tile;
- on a generated node.

Where a key is only a colour in a terrain source, it keeps the 32-bit picture, by day and at night, for every player.
