> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# Root cause

**A generated terrain tile refused its 32-bit picture because a colour of its source images happened to equal a Simutrans special key, although a generated tile cannot hold special colours at all.**

## The chain

1. makeobj reads a PNG. A pixel whose RGB is exactly a key of `image_t::rgbtab` becomes the 16-bit word `0x8000 + key`. Six keys are plain greys: `4D4D4D`, `6B6B6B`, `9B9B9B`, `B3B3B3`, `C9C9C9`, `DFDFDF`.
2. The XR32 plane of the same pixel keeps the colour as painted. A v1 node adds a "semantic run" there, inferred from the RGB equality (`stlab_semantic.cc`), not declared by anyone.
3. `ground_desc_t::init_ground_textures()` makes each tile with `create_textured_tile()`, which reads both sources word by word and writes `(rc<<10)|(gc<<5)|bc`. **The result never has the high bit set.** A key word reads as red 0, green 0, blue key index: this is the "literally blue" of the comment in `ground_desc.cc`.
4. `stlab_xr32_derive_climate_tile()` then decides whether the tile gets the 8-bit composition. Testing-6 refuses when:
   - `stlab_derive_plain(light)` or `stlab_derive_plain(tex)` finds any word `>= 0x8000`;
   - the node has `run_count != 0`.
5. **Refusal is whole-tile.** One pixel leaves the tile at 5 bits × 5 bits, with blue dots.

| One key in | Tiles lost | Measured (testing-6) |
| --- | --- | --- |
| A `LightTexture` row | that slope in 8 climates | V1: 64 tiles |
| A `ClimateTexture` row | **all 65 slopes of that climate** | TK, 4 climates + snow row: 390 tiles |

## The ten questions of phase A

| # | Question | Answer | Evidence |
| --- | --- | --- | --- |
| 1 | Conditions that produce the fallback | No XR32 node in either source; translucent runs; a key word; runs in the node; texture not a full rectangle; tile geometry different from the light map | `stlab_xr32_hook.cci` testing-6, lines 497–515 |
| 2 | Legacy indicators that trigger it | Any word `>= 0x8000`, and `FLAG_HAS_PLAYER_COLOR` / `FLAG_HAS_TRANSPARENT_COLOR` | `stlab_derive_plain()` |
| 3 | RGBA32 data available at that moment | All of it: the plane holds the painted colour at every pixel, key pixels too | `evidence/t7-paks-sources.txt`; lab V1 row 37: word `0x8016`, plane 155,155,155,255 |
| 4 | When a key has real semantics | In an image **drawn as such** (vehicles, buildings), in a **binary water tile** (it copies the word), and when **declared** in a v2 node | `SPECIAL-COLOR-SEMANTICS.md` |
| 5 | When it is mere coincidence | In a light map or climate texture read by the non-binary `create_textured_tile()`, in a v1 node | ibid. |
| 6 | Role of alpha | Translucent runs have another layout: refused by the gate, and a translucent light map crashes before the gate is reached | `LIGHTTEXTURE-ALPHA-CRASH.md` |
| 7 | Company colours | A company key in a source never recoloured a generated tile: the tile has no special word | `evidence/matrix-summary.txt`, pak LA; player 3 run |
| 8 | Night lights | The same: a light key in a source never glowed in a generated tile | ibid. |
| 9 | DAY/NIGHT | Derived tiles darken like any 8-bit tile. A source's night plane is honoured as in A12 | `evidence/z-check.txt` |
| 10 | Images without an XR32 plane | Neither source: unchanged, 16-bit. One source only: the other's words are read, a key as its own RGB | paks LX, TKX, BX |

## Reproduction on the real engine

Unmodified testing-6, 1/1, each tile drawn alone, SDL3 and SDL2 (`evidence/A-repro.txt`):

| Pak | Tiles on the RGB555 grid | Oracle |
| --- | ---: | --- |
| Positive V1 (keys in the 8 corner rows) | **64** of 520 = 8 slopes × 8 climates | 456 derived, 64 legacy, 0 errors |
| Negative V0 (the same pixels one level off) | **0** | 520 derived |
| Without XR32 in the light map, V3 | 64 | 456 derived, 64 legacy |
| Public pak | 0 | 520 derived |

**The reproduction of the 64 tiles still holds.** Slopes 1, 3, 9, 13, 27, 31, 37, 39.

## What `defca456` left open

`defca456` removes the refusal for the light map only. By reading its diff, a key in the **climate texture** still refuses, and measurement confirms it (`evidence/matrix-summary.txt`):

| Pak | testing-6 | `defca456` | Candidate |
| --- | ---: | ---: | ---: |
| NK (31,022 key pixels in the light map) | 304 tiles lost | 0 | 0 |
| TK (204 key pixels in 4 climates + arctic) | 390 tiles lost | **390 tiles lost** | 0 |
| LT (both) | 466 tiles lost | **390 tiles lost** | 0 |

Decision D3 says valid RGBA32 data must not be rejected and artists must not have to avoid ordinary colours. So `defca456` was not enough, and the candidate adds one commit.

## What the masks have to do with it

Nothing. 32-level masks are untouched; on keyed paks the transitions match the RGBA32 + 32-level model exactly once the tile is composed (0 of 979,368 px). See `INDEPENDENT-ORACLE.md`.

## What remains a limit

makie's exact keyed file is not available: `MAKIE-RESOURCE-COMPARISON.md`.
