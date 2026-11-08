> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# Implementation report

## What the candidate is

| | |
| --- | --- |
| Base | testing-6, `f04638a`, tree `b0efabef` |
| Commit 1 | `defca456`, **unchanged and verified**: +49 −4, tree `44f3db83`; the patch file is byte for byte the corner mission's (`6c050f05…`) |
| Commit 2 | `3f1b511` (new): +32 −28, tree `562448a2` |
| Total | one file, `src/simutrans/display/stlab_xr32_hook.cci`, +66 −17 |
| Not touched | makeobj, the XR32 format, the masks, `ground_desc.cc`, the snow. A12-P1 is not in it |

## Why a second commit

`defca456` was recovered, not rewritten, and it is correct for what it covers. Reviewing it against D3 found what it does not cover: a key in the **climate texture** still refused the tile. That is the larger case: one pixel costs 65 tiles rather than 8, and measured on the TK pak, `defca456` still leaves 390 tiles at 16 bits.

The review also found two things that were too permissive in `defca456`, now closed:

| In `defca456` | Why it is wrong | Now |
| --- | --- | --- |
| The light map's node is accepted with any runs | A v2 node's runs are **declared** by the artist. Ignoring them is a semantic decision nobody took | Refused, as in testing-6 |
| A generated node (`SIMUTRANS_XR32_NIGHTGEN=1`) is accepted | Its colours come from the 16-bit words, so a key there has already been given a meaning | A key in its words is refused, as in testing-6 |

## The change

A single gate for both sources, `stlab_derive_source_usable()`, replacing the former `stlab_derive_light_usable()`:

```cpp
static bool stlab_derive_source_usable(image_id n)
{
	const imd32 &d = images[n];
	if(  d.base_data == NULL  ||  (d.recode_flags & FLAG_HAS_TRANSPARENT_COLOR)  ) {
		return false;
	}
	const stlab_xr32_slot_t *s = stlab_slot( n, false );
	if(  s != NULL  &&  s->img != NULL  &&  s->img->version >= 2  &&  s->img->run_count != 0  ) {
		return false;
	}
	const STORED_PIXVAL first_refused = (s != NULL  &&  s->generated) ? 0x8000 : 0x8000 + SPECIAL;
	...
```

And one definition of which node a source contributes:

```cpp
static const stlab_xr32_image_t *stlab_derive_source_node(image_id n)
{
	const stlab_xr32_slot_t *s = stlab_slot( n, false );
	return (s != NULL  &&  s->img != NULL  &&  s->img->rgba != NULL  &&  (s->img->run_count == 0  ||  s->img->version < 2)  &&  !stlab_derived.count( n )) ? s->img : NULL;
}
```

Both gates use it. So `stlab_xr32_derive_climate_tile()` and `stlab_derive_compose_climate()` cannot disagree, and the tile's own check, `stlab_derive_plain( tile )`, stays. A word without plane colour is read by `stlab_derive_source_rgb()`: an opaque key is the key's own RGB, what the sheet painted.

**Not changed:** `stlab_xr32_derive_water_tile()`. A binary tile copies the texture's word, so a key there is a real special colour. See `A4-SPECIAL-COLOR-FALLBACK.md`.

## The brief's requirements

| Requirement | Result | Evidence |
| --- | --- | --- |
| Use valid XR32 data | Keyed tiles equal the RGBA32 model: LT 520 tiles derived, 0 errors | `evidence/matrix-summary.txt` |
| No global fallback on RGB coincidences | 0 tiles at 16 bits on NK, LA, TK, LT | ibid. |
| Preserve real special colours | Tile-only change; every drawn image byte-identical | `evidence/drawall.txt`, `tcc.txt` |
| Preserve alpha and transparency | Translucent sources keep the refusal (pak TA); LB loads; pixel set unchanged | `evidence/matrix-summary.txt` |
| Keep traditional masks | The mask stream is not read or written by this change: 32 levels, transitions exact | `evidence/oracle-trans-*.txt` |
| Keep geometry and zoom | Hash identity without keys at 5 zooms; with keys geometry matches the oracle | `evidence/B-combinations.txt`, `Z-*` |
| Keep fallback for non-XR32 resources | BX: 16-bit, as testing-6 | `evidence/matrix-summary.txt` |
| Keep traditional pakset compatibility | 2.4 frames no real difference | `evidence/frames.txt` |
| Don't modify makeobj or the XR32 format | makeobj byte-identical `8a150381`; no reader or writer file in the diff | `evidence/builds-t7.txt`, `T7-ON-TESTING-6.diff` |

## Semantic ambiguity found, and stopped

A v2 node with declared company or light runs on a **terrain source** has no defined meaning for a generated tile: the 16-bit path shows blue dots, and the node says "company colour". The candidate does not decide this: it keeps testing-6's fallback (pak TM: 65 tiles 16-bit on all three executables). No real pak does this. It is listed in `KNOWN-LIMITATIONS.md` for the maintainer.

## Reproducibility

`tools/build_win.sh` (fresh, `git archive`, fixed `SOURCE_DATE_EPOCH`). The base reproduces testing-6's published hashes. Patches applied to a clean clone give exactly the candidate's tree, three ways (`evidence/patches.txt`); and patch 02 alone on testing-6 refuses to apply.
