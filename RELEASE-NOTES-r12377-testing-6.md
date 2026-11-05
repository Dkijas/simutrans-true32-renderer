> **IMPORTANT: start the game with `SIMUTRANS_XR32=1`, or you will not see any 32-bit art.**
>
> Windows (cmd): `set SIMUTRANS_XR32=1`, then `simutrans-rgba32.exe` ·
> Windows (PowerShell): `$env:SIMUTRANS_XR32="1"`, then `.\simutrans-rgba32.exe` ·
> Linux: `SIMUTRANS_XR32=1 ./simutrans-rgba32`
>
> Without it, paks with 32-bit data still load and play, but the game draws their old 16-bit images.
> makeobj has its own switch: `STLAB_XR32_WRITE=1` writes the 32-bit data into the paks it builds.

**Published 2026-11-05. Experimental test build (pre-release).** It is not an official Simutrans release, it is not merged
into Simutrans, it is not part of the official trunk, and it is not a stable version. The XR32 format is experimental.
Use a separate folder and copies of your savegames. Nothing is promised about loading savegames or paks made with this
build in later test builds.

> **Linux builds — Experimental / Not certified**
>
> The Linux x86_64 SDL2 and SDL3 builds are provided for community testing. They include the A12 generated-terrain
> improvements but have not undergone the full Linux graphics certification process.
>
> Windows x64 SDL2 and SDL3 have been certified with limitations.
>
> Please use a separate installation and back up your savegames.

> **Artistic review pending.** The terrain of a pak with 32-bit terrain data looks brighter than in testing-5 (see
> below). The author of PAK128.german (makie) has not reviewed the new look; no approval is implied.

> **Warning, from the official trunk r12377 itself:** founding a city whose first road runs onto a road with a
> **private-way sign** crashes the game. This happens in both programs of this release, as in testing-1 to testing-5.

## What this build is

- **The same Simutrans SVN trunk as testing-1 to testing-5, r12377** (the game shows "125.0.2 Nightly"). No rebase.
- **testing-5 plus one change of the XR32 32-bit renderer: the generated terrain at 8-bit precision (A12)**, two
  commits (`4c71953` ClimateTexture and snow, `f04638a` LightTexture), three source files.
- **Unchanged:** the XR32 format, makeobj (byte-identical to testing-5's), the DAY/NIGHT pak syntax, the test kit, and
  the 16-bit renderer's source.

Each package has two programs: `simutrans-rgba32` (32-bit, XR32) and `simutrans-16bit` (the normal renderer, for
comparison).

## What's new since testing-5: A12 — generated terrain at 8-bit precision

The game makes every ground tile itself: the slope's light map (`ground.LightTexture`) times the climate texture
(`ground.ClimateTexture`), and the same for snow. Up to testing-5 this was always done from the 16-bit copies of both
images (5 bits × 5 bits into 5 bits), even when the pak carries 32-bit data. A texture of over two thousand colours kept
a few dozen, and showed blotches and bands.

**Now, when `ClimateTexture` and/or `LightTexture` carry XR32 data** (built with the XR32 makeobj and
`STLAB_XR32_WRITE=1`):

- **ClimateTexture:** the generated climate and snow tiles keep the 32-bit (RGBA32) colours of the original textures.
- **LightTexture:** the light map is read from its 32-bit data too, without the earlier quantisation to 5 bits, so its
  gradients on corner and double slopes are no longer cut to 32 levels.
- **Terrain and snow:** less colour banding and more colour variety. In the scenes we measured with PAK128.german 2.5
  beta the terrain shows 2 to 4 times as many colours, and the step pattern of the 16-bit grid is gone.
- The operation is the one Simutrans always used (texture × light, neutral light 128 = the old 16). Tile shapes,
  slopes, coasts, zoom outlines and every other picture are unchanged.

This is **not** dynamic lighting, **not** a shader and **not** GPU acceleration: it is the same software composition,
done with 32-bit colours.

**The ground becomes brighter.** With PAK128.german 2.5 beta, about +8 to +13 levels per channel. These are the pak's
own colours: on the flat tile of every climate the result equals the texture under the pak's own light map exactly.
The old 16-bit composition rounded down several times, which made the ground darker and slightly shifted in hue.
*The artistic review of this by the pak's author is pending.*

**Compatibility with testing-5 (what was checked):**

- XR32 format unchanged; the 16-bit renderer unchanged; makeobj byte-identical.
- Paks without 32-bit terrain look exactly as in testing-5.
- Every other 32-bit picture is drawn exactly as in testing-5, by day and at night: all other 50,299 XR32 images of
  PAK128.german 2.5 beta are identical (company colours / TCC, lights, night planes, water / A4).
- The A11/H1 zoom 2/1 coast fix of testing-5 is kept; screen updates (dirty regions): 0 of 2,160 checked frames
  mismatched.
- Savegames and paks are unchanged.

## Cost

- **Ordinary frames:** no measurable difference (still, scrolling, animated water).
- **Light changes and zoom steps** cost more on a pak with 32-bit terrain, because the visible generated tiles are
  recomposed: about +0.4 to +2.2 ms per event (median), up to about +2.7 ms on the slowest frames, depending on how many
  different ground tiles are in view (Windows, PAK128.german 2.5 beta). A cache that would remove most of the
  light-change part (A12-P1) has been proposed; **it is not implemented in this build**.
- **Memory:** peak memory within about 1 MB of testing-5, except very frequent zooming (+5 to +9 MB, about 0.5 %); no
  sustained growth over 3,000 frames of light changes. Long zoom runs were not measured.

## Checked (Windows x64, SDL2 and SDL3)

- **Model:** all 520 generated tiles match an independent model exactly, with PAK128.german 2.5 beta and with synthetic
  sources, on SDL2 and SDL3; deliberately wrong models fail.
- **Geometry:** unchanged at zoom 1/1, 2/1, 9/8, 3/4 and 1/2.
- **Screen updates:** every one of the 2,160 checked frames matched what the game drew.
- **Other pictures:** the other 50,299 XR32 images are identical, by day and at night.
- **Build identity:** the Windows executables are the certified builds; fresh builds reproduce them byte for byte.
- **Antivirus:** Microsoft Defender (definitions 1.463.130.0): the testing-6 Windows executables and packages were
  scanned clean.

## Known issues

- **Transition blending stays at 32 levels:** the climate transitions, the snow line and the beach outline blend
  32-bit pictures, but with 32 weight levels. A known, accepted limit of this build.
- **Brighter ground** on paks with 32-bit terrain, compared with testing-5 and with the traditional 16-bit renderer
  (above); **artistic review pending**.
- **Snow on some steep, strongly lit slopes clips to pure white or a cyan-tinted white** with PAK128.german 2.5 beta: the pak's
  light × its snow exceeds the maximum. testing-5 clipped the same pixels.
- **Light changes and zoom steps cost more** with 32-bit terrain: +0.4 to +2.2 ms on the frames that rebuild the terrain
  caches, slowest cases near +2.7 ms (above). The proposed cache (A12-P1) is **not implemented** yet.
- **Limited artistic testing:** one real pak with 32-bit terrain (PAK128.german 2.5 beta) plus synthetic paks. This is
  not a claim of compatibility with, or of a good look on, every pakset. The pak author's (makie's) assessment is pending.
- **Code base:** official Simutrans SVN trunk **r12377**; no later trunk revision is included.
- **XR32 is experimental**, and so is the DAY/NIGHT pak syntax.
- **Inherited from testing-5:** the zoom colour residual of translucent company colours; night pictures cost memory;
  the 16-bit renderer darkens translucent company colours twice at night (its own behaviour, unchanged).
- **Microsoft Defender and testing-4 (D1, history):** after testing-4 was published, Defender's machine-learning
  detection (`Trojan:Win32/Bearfoos.A!ml`, definitions 1.463.101.0) flagged testing-4's Windows SDL3
  `simutrans-rgba32.exe`. With definitions 1.463.102.0 the same file is no longer detected. The testing-6 files were
  scanned clean (above). See `docs/KNOWN-ISSUES-r12377-testing-6.md`.
- **From the official trunk r12377 itself, not caused by XR32:** the private-way sign crash; empty scenario texts; the
  chat's public tab shows whispers; the music volume after off/on; Windows 8 or newer is required.

Full list: `docs/KNOWN-ISSUES-r12377-testing-6.md`.

## Where it was tested

- **Windows 11 x64 — certified with limitations:** SDL2 (Direct3D 9) and SDL3 (Direct3D 11), one 3440×1440 display,
  one real pak with 32-bit terrain. Limitations: the 32-level transitions, the light-change/zoom cost, one display, no
  AddressSanitizer run for A12.
- **Linux x86_64 — experimental, NOT certified:** built on Debian 13 (GCC 14.2) with the release recipe from the same
  source, rebuilt independently for this publication with identical bytes. Checked only: architecture, libraries,
  `-help`, start and clean exit (SDL's dummy driver under WSL 2, including PAK128.german 2.5 beta) and one WSLg X11
  window. WSL 2 / WSLg is not a native Linux desktop.
- **Not tested:** macOS, Android, ARM, a native Linux desktop, several displays.

Platforms: x86_64 only. Windows 8 or newer; Linux with glibc 2.38 or newer; both with SDL2 or SDL3.

## How to test

1. Unpack into a new, empty folder and add a pakset. No pakset is included.
2. Start `simutrans-rgba32` with `SIMUTRANS_XR32=1`, as shown above.
3. With a pak whose terrain has 32-bit data (for example PAK128.german 2.5 beta): look at slopes, climate bands and
   snow, by day and at night, at every zoom; compare with testing-5 on the same savegame and view.
4. Does the brighter ground look right to you? Report it either way, with a PNG screenshot.
5. Without such paks nothing should look different from testing-5. Compare with `simutrans-16bit`.

## Downloads

| File | What |
| --- | --- |
| `simutrans-xr32-night-prototype-r12377-testing-6-win64-sdl2.zip` | Windows x64, SDL2 — certified with limitations |
| `simutrans-xr32-night-prototype-r12377-testing-6-win64-sdl3.zip` | Windows x64, SDL3 — certified with limitations |
| `simutrans-xr32-night-prototype-r12377-testing-6-linux-x86_64-sdl2.tar.gz` | Linux x86_64, SDL2 — experimental, not certified |
| `simutrans-xr32-night-prototype-r12377-testing-6-linux-x86_64-sdl3.tar.gz` | Linux x86_64, SDL3 — experimental, not certified |
| `makeobj-xr32-night-prototype-r12377-testing-6-windows-x86_64.zip`, `…-linux-x86_64.tar.gz` | makeobj — the program is byte-identical to testing-5's (A12 changes no makeobj file) |
| `xr32-night-prototype-r12377-testing-2-testkit.zip` | the test kit, unchanged since testing-2 |
| `xr32-night-prototype-r12377-testing-6-source.tar.gz` | the full source (commit `f04638a`) |
| `XR32-NIGHT-PROTOTYPE-FULL.r12377-testing-6.diff` | everything XR32 changes against official r12377 |
| `SHA256SUMS.txt`, `NIGHT-PROTOTYPE-MANIFEST.md` | hashes and manifest |

No pakset and no third-party art is included in any download.

## Acknowledgements

- **makie**, for the observation that the generated terrain of PAK128.german was still processed at 16 bits, which started
  A12, and for his work on 32-bit terrain art. His assessment of the new look is still pending; nothing here implies his
  approval.
- **prissi**, for his technical contributions to Simutrans and the night-image design this prototype builds on.
- The pakset artists and the Simutrans community, for their art, tests and feedback.

## Feedback

https://github.com/Dkijas/simutrans-true32-renderer/issues: give the build name, your OS and package, the pakset,
whether `SIMUTRANS_XR32=1` was set, and attach a PNG screenshot.
