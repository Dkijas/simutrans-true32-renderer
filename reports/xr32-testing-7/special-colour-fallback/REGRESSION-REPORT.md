> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# Regression report

The candidate against testing-6. **Without keys the fix does not act**, so the test is byte identity. With keys it acts only on the tiles it is meant to.

| Item | Result | Evidence |
| --- | --- | --- |
| **A4** coast water | Water tiles byte-identical on PAK-NEW; slots unchanged. Mutant pak frames: 0 real outside the clock, SDL3 and SDL2. Keyed coast water: as testing-6, by decision | `water.txt`, `frames.txt` |
| **A11/H1** 2/1 | Mutant pak at 2/1: 0 real px, SDL3 and SDL2. Tiles at 2/1: byte-identical. The derived tile's 2/1 path untouched | `frames.txt`, `B-combinations.txt` |
| **TCC** | The TCC fixture on pak192.comic: 4 XR32 images, day and night, 0 differ, SDL3 and SDL2. TCC's own gates not re-run | `tcc.txt` |
| **TCC, wider** | All images of PAK128.german: 0 of 50,823, day and night | `drawall.txt` |
| **A12** | 520 tiles and 570 transitions match the RGBA32 + 32-level model exactly | `matrix-summary.txt` |
| **Dirty regions** | 3,120 verified frames (2,280 on the candidate, 840 on testing-6), 0 mismatches. With and without keys, scene `tcorners` included | `verify-summary.txt` |
| **DAY/NIGHT** | 0 of 50,823 at light level 4; frames unchanged | `drawall.txt`, `frames.txt` |
| **makeobj** | Byte-identical, `8a150381` | `builds-t7.txt` |
| **16-bit** | Byte-identical, `8feaaf49`, `20cbf7da` | `builds-t7.txt` |
| **XR32 format** | No reader or writer file in the diff | `T7-ON-TESTING-6.diff` |
| **Suites** | 309 of 309 on testing-6 and on the candidate, SDL3, SDL2, 16-bit | `suites.txt` |
| **Warning sets** | Windows: identical to testing-6, SDL3 and SDL2. Linux: identical | `logs/build-*.warnings`, `linux-t7.txt` |
| **NIGHTGEN** (experimental switch) | Generated nodes with keys: identical to testing-6 | `nightgen.txt` |
| **Snow** | Byte-identical: D2 respected | `windows-summary.txt` |
| **Masks** | Not touched: 32 levels, mask streams byte-identical | `masks-identical.txt` |

## Controls: each of these checks can fail

| Check | CONTROL | Result |
| --- | --- | --- |
| Suites | one injected failing test | **FAIL** `[1/310]` |
| Dirty regions | a renderer that drops dirty rectangles | **109 of 240** frames caught, 615,746 px at worst |
| Byte comparison of tiles | PAK-OLD against PAK-NEW | 3,965 of 4,025 differ |
| Drawn images | day against night | 50,560 differ; on TCC 4 of 4 |
| Frames | a keyed pak | 13.8 M px |
| Oracle | engine mutants, corrupted dumps | `INDEPENDENT-ORACLE.md` |

## NIGHTGEN, read carefully

`evidence/nightgen.txt`. With the switch on:

| Pak | testing-6 | Candidate |
| --- | --- | --- |
| BX (keys, no XR32 in the sources: only generated nodes) | `ca35f6c2` | `ca35f6c2`: **identical** |
| NK (keys, authored XR32 in the light) | `976d1562` | `46202a04`: differs |

NK differs because its light map's node is authored, not generated, so the fix applies as without the switch. This is correct by the rule; the switch does not change it.

## Compatibility

| Resource | Result |
| --- | --- |
| Traditional pak 2.4 | no real difference; real window 1 to 4 px of the capture quirk |
| PAK-OLD, PAK-NEW | byte-identical |
| XR32 off | no real difference |
| Light or texture with keys; without XR32 | now 8-bit, exact |
| Both without XR32 | inherited |
| Savegames | `terrainA12scene`, `coastworld`, `tsnow`, `tcorners` |

**Not a claim of universal compatibility.** One real pak with XR32 terrain, its lab variants, a traditional pak, and pak192.comic with a fixture.

## Differences that appeared and why they are not real

| What | Explanation (measured) |
| --- | --- |
| 28–42 px at x 137–151, status bar | The clock's digits |
| 978,178 px, coast, 1/1, unpaused | Water stage: 98 % water, 0 px in 36 later pairs |
| 1,180,670 px, one candidate run at 1/2, unpaused start | The same: 98.4 % water. Its siblings 255 px apart, and 190 from testing-6 |
| 142–266 px, zoom 1/2, coast and 2.4, paused | Testing-6 against itself 205–264; candidate 173–255 |
| 1–23 px in a real window | `CopyFromScreen` reads 13,11,12 as black |

## Regressions found

**None.** The translucent light crash and the SDL3 posted-key zoom-out behave the same on testing-6.
