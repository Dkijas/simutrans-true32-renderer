> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# Known issues — XR32 testing-6 candidate (testing-5 + A12)

testing-6 = testing-5 + A12: the climate and snow tiles are composed at 8 bits from the XR32 data of `ClimateTexture` / `LightTexture`. Nothing else changes; the XR32 format is unchanged.

**Measurement scope.** Unless an item says otherwise, results were measured on the candidate's Windows executables (SDL2 and SDL3) with makie's PAK128.german 2.5 beta. The Linux builds are experimental and not certified (K7).

## New with A12

### K1. Transition weights stay at 32 levels — accepted limitation for testing-6

The climate transitions, the snow line and the beach outline blend 8-bit images with 5-bit mask weights. Measured:
- weight error 1.4 % on average;
- at most 2–5 levels of colour at the pak's highest contrast;
- not visible in the tests.

Design options and costs (≈ 8 MB, or on-demand recomposition; risks to A11): `A12-D-TRANSITION-MASKS.md`. Do not describe the terrain as fully 8-bit.

### K2. The ground of a pak with XR32 terrain is brighter than in testing-5

+8 to +13 levels per channel (luma ×1.08–1.11) with PAK128.german 2.5 beta. It is the original colour, the texture under the pak's own light map, which the old 16-bit composition darkened and hue-shifted by rounding (`VISUAL-COMPARISON.md`, Phase C).

**Artistic review pending** (`ARTIST-REVIEW-PENDING.md`). Paks without XR32 terrain are unaffected.

### K3. Snow on lit slopes clips (authored, pre-existing)

56 % of snow pixels have a channel at the maximum, because the light map (up to 178) × the snow (≈ 245) exceeds 255. testing-5 clipped the same pixels. It is more noticeable next to the now-smooth texture. A pak matter.

### K4. Frames that rebuild the derived tiles' caches cost more

A light change or a zoom step on a pak with XR32 terrain costs more than in testing-5. The steady state does not. Measured in `PERFORMANCE.md`.

### K5. Conservative derivation

A generated tile stays 16-bit when any of the following holds:
- the light map and tile geometry differ;
- the texture is not a full rectangle covering the tile's rows;
- a source has special colours or translucent runs;
- the texture node has semantic runs.

None of these holds for PAK128.german 2.5 beta: 520 of 520 tiles are derived.

## Inherited from testing-5 (unchanged)

### K6. Inherited items

- **TCC-R:** zoom colour residual of translucent company colours (up to about 6 levels with an opaque key).
- **A7:** night pictures cost memory (by design).
- **A8:** the DAY/NIGHT pak syntax and the XR32 format are experimental.
- **A10:** a zoom change costs about 1 ms more (A4).
- **N2–N4:** open.
- **T1:** after a live theme change two toolbar elements can keep their old picture (trunk behaviour).
- The 16-bit renderer darkens translucent company colours twice at night (its own behaviour).

### K7. Platforms

| Platform | Status |
| --- | --- |
| Windows x64 SDL2 and SDL3 | certification platforms, one 3440×1440 display |
| Linux x86_64 SDL2 and SDL3 | **experimental, not certified**. Built from the same source with the release recipe. Checked only for architecture, libraries, start/stop on WSL 2 with SDL's dummy driver (including a run on makie's pak, so the A12 path ran) and one WSLg X11 window. WSLg is not a native Linux desktop. |
| macOS, Android, ARM, native Linux desktop, several displays | not tested |

### K8. Official trunk r12377 itself (both renderers)

- Founding a city next to a road with a private-way sign can crash the game.
- Scenario text files can come out empty.
- The chat's public tab shows whispers.
- The music can play very quietly after off/on.
- Windows 8 or newer is required (r12361).
