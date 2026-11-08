> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# makie's resource: old and new

Forum topic 24081: #93 (5 Nov) and #94 (6 Nov) by makie, read on 7 Nov 2026 (`work/forum/topic24081-p3.html`).

- **#93:** makie reports that the 16-bit route of a terrain edge happens only when `LightTexture` contains special colours, naming `6b6b6b`, and that the few blue pixels in the capture are those special colours.
- **#94:** "this is a pak128.german with 32bit and removed dithering", at the same URL, "with an improved LightTexture", "From my perspective, everything is currently working."

**makie's statements are about makie's own build of the pak, viewed on testing-6. makie has not seen, reviewed or certified the local candidate.**

## The two archives

| | PAK-OLD | PAK-NEW |
| --- | --- | --- |
| Downloaded | 3 Nov 2026 (readiness lane) | 7 Nov 2026 (this lane, new empty folder) |
| Last-Modified | Tue 3 Nov 2026 16:03:49 GMT | Fri 6 Nov 2026 12:06:24 GMT |
| ETag | `3085178b-65f616871a233` | `3084ff11-65f9a70f00e79` |
| Size | 814,028,683 bytes | 814,022,417 bytes |
| SHA-256 | `5220f4d4…` | `82245ff8842eea3e…` |

The new one really is new, so the brief's warning did not apply. **Changed members: 10 of 980** (`evidence/zip-diff.txt`): `ground.LightTexture.pak` and nine `text/*.tab` of equal size. `ground.ClimateTexture.pak` and every other ground pak are unchanged. Originals not modified.

## The new `LightTexture`

Tool `tools/pak_keys.py`, output `evidence/pak-new-light.txt`, `evidence/light-classes.txt`, census of all ground paks in `evidence/pak-new-ground-census.txt`. Control: on the lab paks it reads the known keys (V1 61 pixels, V0 0).

| Measure | OLD | NEW |
| --- | --- | --- |
| Rows with XR32 data | 81 of 81 | 81 of 81 |
| Pixels equal to a special colour | 0 | **0** |
| Semantic runs | 0 | 0 |
| Alpha values in the plane | 0, 255 | 0, 255 |
| Geometry (pixel set of every row) | | **identical to OLD, 81 of 81** |

By class of row:

| Class | Rows | Mean grey OLD → NEW | Largest change | Distinct colours per row OLD → NEW |
| --- | ---: | --- | ---: | --- |
| Flat | 1 | 129.33 → 128.00 | 4 | 1 → 1 |
| Straight slopes | 4 | 132.88 → 131.75 | 4 | 1 → 1 |
| Outer corners | 4 | 133.22 → 129.02 | 49 | 71–92 → 40–50 |
| Inner corners | 4 | 133.68 → 129.04 | 48 | 81–96 → 39–49 |
| Double slopes, saddles | 52 | 137.77 → 137.46 | 81 | up to 194 → up to 120 |

Flat is now exactly the neutral 128. The corners are a smooth fan of a few dozen greys, and their overall level is lower.

## Does it contain the problematic values?

**No.** The file makie published holds **no** exact special colour, in any of the 11 ground paks. So nothing in it can trigger the fallback, and on testing-6 it draws 0 tiles from 16-bit words.

The values must have been in makie's working file behind the capture of 5 Nov, which is **not published**. What can be measured about that file, against the capture `f32gk.png` (same bytes as before, `43db254e`):

| Test on the corner tile at 169,28, slope 37 | NEW light | OLD light | Control (texture off by one) |
| --- | ---: | ---: | ---: |
| Correlation of the capture's implied light | **0.981** | 0.760 | 0.912 |
| Mean absolute difference | 14.4 | 13.4 | 7.6 |
| Exact pixels of the 16-bit model | 3.4 % | 0.7 % | 1.4 % |

So the working file has the **same fan shape** as the published one and **another level**. It was not a matter of nudging a few pixels one level: makie changed the values too. A search of every corner placement in the capture finds none at or above 50 % exact (`evidence/makie-new-match.txt`). The one blue dot in that tile lies on a new grey of 97, not next to a key (`evidence/makie-new-tile.txt`).

**What this means.** I cannot show the keys in makie's working file, because I do not have it. What supports them: makie names `6b6b6b` from direct knowledge of the file; the blue dots of the capture (0,0,64..80) are what a key word gives; and the same mechanism reproduces the look in the lab. The trigger in that exact file remains makie's observation, not my measurement.

## The engine on both

Tiles and transitions alone, 5 zooms, hashes (`evidence/B-combinations.txt`), and the independent oracle at 1/1 (`evidence/matrix-summary.txt`):

| | PAK-OLD | PAK-NEW |
| --- | --- | --- |
| testing-6 | 520 tiles 8-bit, transitions 0 of 979,368 px off the model | the same |
| `defca456` | byte-identical to testing-6: 0 of 4,025 cases, SDL3 and SDL2 | byte-identical: 0 of 4,025 |
| Candidate `3f1b511` | byte-identical to testing-6: 0 of 4,025 | byte-identical: 0 of 4,025 |

Control: testing-6 between OLD and NEW differs in 3,965 of 4,025 cases, and all 65 slopes change at 1/1.

**Do not read this table as the engine fixing makie's picture.** The improvement makie sees between the two archives is makie's art: testing-6 alone already shows it. The engine fix changes nothing on either published file, and acts only on a file with exact keys. Both are true at once.

## Why the engine fix is still wanted

The new file avoids the keys by being different art, and the next edit of a `LightTexture` or a `ClimateTexture` may land on them again: there are six plain greys among the keys. Decision D3 says artists should not have to avoid ordinary colours. In PAK-NEW itself, 31,022 pixels of the light map are exactly one level from a grey key, in 38 rows (`evidence/t7-paks-sources.txt`).

The visual comparison between the four combinations is in `VISUAL-COMPARISON.md`.
