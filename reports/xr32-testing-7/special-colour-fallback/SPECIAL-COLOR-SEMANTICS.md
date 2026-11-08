> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# Special colours: when they mean something

A pixel can be exactly a key of `image_t::rgbtab` for two different reasons. Either the artist wants a company colour or a light, or a colour happens to land there. RGB alone cannot tell which: `6B6B6B` is both a light key and a plain grey.

What decides the meaning here is **where the image is used** and **whether the semantics are declared**.

## The rule the candidate applies

| Situation | Meaning of the key | What the engine does | Measured |
| --- | --- | --- | --- |
| Image drawn as such (vehicle, building, tree, way) | **Real** | Unchanged: recoloured, lit | 50,823 images alone, day and night: 0 differ from testing-6 |
| Light map or climate texture of a climate or snow tile, **v1 node** | **Coincidence.** `create_textured_tile()` writes plain RGB555, so a generated tile has no special word whatever its sources hold | Plane colour, 8 bits | NK, LA, TK, LT: 0 tiles at 16 bits; every tile exact |
| The same, **source without XR32 plane**, the other source with one | Coincidence | The key's own RGB, what the sheet painted | LX, TKX, V3: exact |
| The same, **v2 node with declared runs** | **Declared.** Meaning for a terrain tile undefined | Keeps testing-6's fallback, 16 bits | TM: 65 tiles, same on all three executables |
| The same, **generated node** (`SIMUTRANS_XR32_NIGHTGEN=1`) | Its plane comes from the words | Keeps the refusal | BX under NIGHTGEN: identical to testing-6 |
| Neither source has XR32 | Inherited behaviour | 16 bits, blue dots included, as ever | BX: 513 tiles legacy + 7 where both models agree, 0 errors |
| Translucent pixel in a source | Other layout, undefined | Texture: refused. Light map: crashes at load, also in r12377 | TA; `LIGHTTEXTURE-ALPHA-CRASH.md` |
| **Coast water tile**, sea or water climate texture | **Real**: a binary tile copies the word, so the tile holds a special colour | Unchanged | TW: 38 of 130 tiles refused, on the candidate and on the experiment |
| Coast water tile, **light map** | Coincidence: read as black or not | **Unchanged in the candidate**: keeps the fallback | `A4-SPECIAL-COLOR-FALLBACK.md` |

## The classes of key

All 31 keys were placed in sources (paks LA, TK). Each pixel stays what was painted, multiplied by the other source.

| Keys | Class | In a source of a generated tile |
| --- | --- | --- |
| 0–7, 8–15 | Company colours | Painted colour. **Not recoloured**: tiles as player 3 equal tiles as player 0, 520 of 520, on the keyed pak |
| 16–20, 26, 27, 29, 30 | Lights | Painted colour. **No glow**: at light level 4 every pixel fits the darkening learned from a pak without keys (2,171,424 px, 0 off) |
| 21–25, 28 | Plain greys (`6B`, `9B`, `B3`, `C9`, `DF`, `4D`) | Painted colour |

Note what testing-6 did with these: it never recoloured or lit them either. It turned them into blue dots and dropped the tile to 16 bits. So no semantic is lost, because in a generated tile there never was one.

## The source of this reading

- `ground_desc.cc:81`: `*dest++ = (rc<<10) | (gc<<5) | bc;`, no high bit, whatever the inputs.
- `ground_desc.cc:84`: in binary mode, `*dest = mix;`: the word is copied. This is why water differs.
- `ground_desc.cc:105`: "no special colors or your will see literally blue!", upstream's own comment on the mask path, and the blue dots makie saw.
- `stlab_semantic.h`: v1 runs are inferred from RGB; `stlab_semantic_masks.h`: "makeobj does NOT guess semantics from the final RGB" in v2.
- `image_writer.cc`: "exact special colours ... ordinary paint in XR32, still special in the legacy image".

## The open question for a maintainer

What should a **declared** company colour on a terrain source mean? No real pak does this. The candidate declines to decide and leaves testing-6's behaviour.
