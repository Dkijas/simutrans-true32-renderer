# Known issues — XR32 testing-6 (r12377)

testing-6 = testing-5 + A12: the generated climate and snow tiles are composed at 8 bits from the XR32 data of
`ClimateTexture` / `LightTexture`. Nothing else changes; the XR32 format is unchanged. XR32 remains **experimental**.

**Measurement scope.** Unless an item says otherwise, results were measured on the Windows x64 executables of this
release (SDL2 and SDL3) with PAK128.german 2.5 beta, one display. The Linux builds are experimental and not certified
(K7).

## A. New with A12

### K1. Transition weights stay at 32 levels — accepted limitation

The climate transitions, the snow line and the beach outline blend 8-bit pictures with 5-bit (32-level) mask weights.
Measured: weight error 1.4 % on average; at most 2–5 levels of colour at the pak's highest contrast; not visible in our
tests. 256-level masks are a possible later change; they are not part of this build. Do not describe the terrain as
fully 8-bit.

### K2. The ground of a pak with XR32 terrain is brighter than in testing-5

+8 to +13 levels per channel (luma ×1.08–1.11) with PAK128.german 2.5 beta. It is the original colour — the texture under
the pak's own light map — which the old 16-bit composition darkened and shifted in hue by rounding. Paks without XR32
terrain are unaffected.

**Artistic review pending.** The author of PAK128.german has not reviewed the new look; no approval is implied.

### K3. Snow on strongly lit slopes clips (pre-existing)

On steep slopes facing the light, a large share of snow pixels reach the maximum of a channel (pure white or a
cyan-tinted white), because the pak's light map (up to 178) × its snow colour (about 245) exceeds 255. testing-5 clipped
the same pixels; it is more noticeable next to the now-smooth texture. It is a matter of the pak's data.

### K4. Light changes and zoom steps cost more

A light change or a zoom step on a pak with XR32 terrain recomposes the visible generated tiles: about +0.4 to +2.2 ms
per event (median), up to about +2.7 ms on the slowest frames, scaling with the number of distinct ground tiles in view.
Ordinary frames do not cost more. Peak memory: within about 1 MB of testing-5, except very frequent zooming (+5 to
+9 MB). A cache of the composed tiles that would remove most of the light-change cost (A12-P1) has been proposed and
**is not implemented** in this build.

### K5. Conservative derivation

A generated tile stays 16-bit when any of the following holds: the light map and tile geometry differ; the texture is
not a full rectangle covering the tile's rows; a source has special colours or translucent runs; the texture node has
semantic runs. None of these holds for PAK128.german 2.5 beta: 520 of 520 tiles are derived.

### K6. Compatibility scope

One real pak with XR32 terrain (PAK128.german 2.5 beta) and synthetic paks were tested. Paks without XR32 terrain draw
as in testing-5. This is not a claim of compatibility with every pakset.

## B. Inherited from testing-5 (unchanged)

- **TCC-R:** zoom colour residual of translucent company colours (up to about 6 levels with an opaque key).
- **A7:** night pictures cost memory (by design).
- **A8:** the DAY/NIGHT pak syntax and the XR32 format are experimental.
- **A10:** a zoom change costs about 1 ms more (A4 coast cache).
- **N2–N4:** open minor points.
- **T1:** after a live theme change two toolbar elements can keep their old picture (trunk behaviour).
- The 16-bit renderer darkens translucent company colours twice at night (its own behaviour).

### D1. Microsoft Defender flagged testing-4's Windows SDL3 executable (history)

- After testing-4 was published, Microsoft Defender's machine-learning detection (`Trojan:Win32/Bearfoos.A!ml`, with
  definitions 1.463.101.0) flagged `simutrans-rgba32.exe` of testing-4's Windows SDL3 package (SHA-256 `43420d63…`).
- With definitions 1.463.102.0 the same file was no longer detected. It is rebuilt byte for byte from testing-4's
  published source by the build recipe of these releases.
- testing-4's release is left unchanged. testing-5 was scanned clean with 1.463.102.0.
- **testing-6:** every asset and every executable inside the Windows packages was scanned with definitions
  1.463.130.0: no threats.

## C. Platforms

### K7. Platforms

| Platform | Status |
| --- | --- |
| Windows x64 SDL2 and SDL3 | **certified with limitations** (K1–K6, one 3440×1440 display, no AddressSanitizer run for A12) |
| Linux x86_64 SDL2 and SDL3 | **experimental, not certified**. Built from the same source with the release recipe, rebuilt independently for publication with identical bytes. Checked only for architecture, libraries, `-help`, start and clean exit on WSL 2 with SDL's dummy driver (including PAK128.german 2.5 beta, so the A12 path ran) and one WSLg X11 window. WSLg is not a native Linux desktop. |
| macOS, Android, ARM, native Linux desktop, several displays | not tested |

## D. Official trunk r12377 itself (both renderers, not caused by XR32)

- Founding a city next to a road with a private-way sign can crash the game.
- Scenario text files can come out empty.
- The chat's public tab shows whispers.
- The music can play very quietly after switching it off and on, until the volume slider is moved.
- Windows 8 or newer is required (r12361).
