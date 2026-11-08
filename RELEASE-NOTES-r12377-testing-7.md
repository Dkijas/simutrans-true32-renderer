> **IMPORTANT: start the game with `SIMUTRANS_XR32=1`, or you will not see any 32-bit art.**
>
> Windows (cmd): `set SIMUTRANS_XR32=1`, then `simutrans-rgba32.exe` ·
> Windows (PowerShell): `$env:SIMUTRANS_XR32="1"`, then `.\simutrans-rgba32.exe` ·
> Linux: `SIMUTRANS_XR32=1 ./simutrans-rgba32`
>
> Without it, paks with 32-bit data still load and play, but the game draws their old 16-bit images.
> makeobj has its own switch: `STLAB_XR32_WRITE=1` writes the 32-bit data into the paks it builds.

**Published 2026-11-08. Experimental test build (pre-release).** It is not an official Simutrans release, it is not
merged into Simutrans Standard, it is not part of the official trunk, and it is not a stable version. **XR32 remains
experimental**, and so does its format. Use a separate folder and copies of your savegames. Nothing is promised about
loading savegames or paks made with this build in later test builds.

> **Linux builds — Experimental / Not certified**
>
> The Linux x86_64 SDL2 and SDL3 builds are provided for community testing. They are built from the same source as
> the Windows builds, and in our checks under WSL 2 (not a native Linux desktop) they start, load paks and quit
> cleanly. They have not undergone a full Linux graphics certification.
>
> Windows x64 SDL2 and SDL3 have been certified with limitations.
>
> Please use a separate installation and back up your savegames.

## What this build is

- **The same Simutrans SVN trunk as testing-1 to testing-6, r12377** (the game shows "125.0.2 Nightly"). No rebase.
- **testing-6 plus one fix**: two commits (`defca456` LightTexture, `3f1b511` ClimateTexture), one source file,
  `display/stlab_xr32_hook.cci`, +66 −17.
- **Unchanged:** the XR32 format, makeobj (byte-identical to testing-6's), the traditional 16-bit renderer
  (byte-identical to testing-6's), the DAY/NIGHT pak syntax and the test kit.

Each package has two programs: `simutrans-rgba32` (32-bit, XR32) and `simutrans-16bit` (the normal renderer, for
comparison).

## What's new since testing-6: no more 16-bit fallback from special colours in terrain sources

The game makes every ground tile itself: the slope's light map (`ground.LightTexture`) times the climate texture
(`ground.ClimateTexture`). Since testing-6 (A12) this is done from their 32-bit data.

Simutrans reserves 31 exact colours as **special colours**: company colours and lights. Six of them are plain greys:
`4D4D4D`, `6B6B6B`, `9B9B9B`, `B3B3B3`, `C9C9C9`, `DFDFDF`. **Up to testing-6, if a pixel of either image happened to be
exactly one of those colours, the generated tile fell back to its 16-bit (RGB555) picture**, with bands and small blue
dots:

| One such pixel in | Cost in testing-6 |
| --- | --- |
| a row of `LightTexture` | that slope, in every climate and in snow |
| a row of `ClimateTexture` | all 65 slopes of that climate |

A generated tile cannot hold special colours: the game writes plain colour values into it. Such a pixel was only ever a
colour, and makeobj had kept that colour in the 32-bit data. **testing-7 uses the 32-bit data for those tiles.** Artists
do not have to avoid those greys in terrain sources.

## What changes visually

- **In a pakset whose terrain sources contain such colours:** the affected tiles show their full 32-bit picture
  instead of the 16-bit one: no banding, and no blue dots where those colours sit.
- **The updated PAK128.german 2.5 beta, as published on 6 November 2026, shows no visual difference from
  testing-6.** Its light map no longer contains those colours, and testing-7 draws exactly what testing-6 draws on it,
  byte for byte. The improved corners in that version are makie's work on the pak, and testing-6 already shows them.

## Special colours that are real

They are untouched:

- company colours, lights and translucent company colours in buildings and vehicles: the same pictures as testing-6, by
  day and at night;
- a company colour or light **declared by the artist** in a mask on a terrain image: kept as before, at 16 bits;
- semi-transparent pixels in a terrain source: kept as before;
- coast water: kept as before (see the known issues).

## Compatibility with testing-6 (what was checked)

- XR32 format unchanged; makeobj and the 16-bit renderer byte-identical.
- Coast water (A4), the 2/1 zoom fix (A11/H1), translucent company colours (TCC), generated terrain (A12) and
  DAY/NIGHT: kept. Where no such colour exists, the pictures are the same as testing-6's.
- Paksets without 32-bit data: as before. Savegames: nothing saved changes.

## Cost

| Pak | Result (Windows, n = 5 per case, idle machine) |
| --- | --- |
| Without such colours (for example PAK128.german as published) | **No measurable difference** from testing-6 |
| Such colours in every corner tile in view (lab pak) | Ordinary frames unchanged. **A zoom step costs 2 to 3 ms more**; the slowest frames of a light change up to about +3 ms |

The extra cost is the 32-bit composition of tiles that testing-6 skipped by falling back to 16 bits: a pak with such
colours now costs what a pak without them already costs. The proposed cache (A12-P1) would reduce both; **it is not
part of this build**.

**Memory:** no sustained growth in a 40-minute session of constant zooming (private memory stays within a 12 MB band),
and none during day/night changes.

## Checked (Windows x64, SDL2 and SDL3)

- **The fix:** on three lab paks made for it, testing-6 leaves 304, 390 and 466 of the 520 generated tiles at 16 bits;
  testing-7 leaves none, on SDL2 and SDL3. 60 of 60 runs match an independent model of what the engine should draw,
  built from the pak files alone. Deliberately wrong engines (mutants) and corrupted dumps are caught.
- **Real special colours:** a company-colour key in a terrain source is a painted colour and is not recoloured
  (player 3 draws the same tiles as player 0); night planes in a keyed texture are used at night and do not leak into
  the day; declared masks and coast water keep the old behaviour.
- **Zoom** at 1/1, 2/1, 9/8, 3/4 and 1/2, with round trips, including by keyboard (keys sent by test tools to the
  focused window; a physical keyboard was not tested).
- **Screen updates (dirty regions):** 2,940 frames checked against what the game drew (lab builds of the same source with a frame checker, SDL2 and SDL3), with no mismatch; a renderer that deliberately drops screen updates is caught. Once, during the release gate, 8 of the 60 checked frames of one SDL3 run differed (at most 29 px) while another application was in use on the machine. That did not come back in 1,920 frames of the same scenario on an idle machine, on testing-6 and on testing-7, SDL2 and SDL3, so it is not attributed to testing-7.
- **Automated suites:** 309 of 309 on every Windows executable; a deliberately failing test is reported.
- **Build identity:** the published Windows and Linux executables are the certified builds; fresh, non-incremental
  builds reproduce them byte for byte.
- **Antivirus:** Microsoft Defender (definitions 1.463.189.0): every asset, and every executable inside the packages,
  scanned clean.

## Known issues

- **The exact older `LightTexture` behind makie's original report is not available.** The cause rests on makie's own
  diagnosis (special colours in `LightTexture`, naming `6b6b6b`) and on our lab reproduction, which agree. The fix was
  verified on lab paks that reproduce the same fault, not on that file byte for byte.
- **Coast water is out of scope:** on slopes whose `LightTexture` row contains such a colour, the land is 32-bit but the
  coast water stays 16-bit. Same cause, separate fix, not certified and **not in this build**.
- **Extra composition cost** on paks with such colours: a zoom step 2 to 3 ms more (above).
- **A semi-transparent pixel in `LightTexture` crashes the game at load.** It also happens in official r12377 without
  XR32. Not changed here.
- **Transition blending stays at 32 levels** (the colours blended are 32-bit; the blend weight is not). A12-P1 is
  still pending.
- **Linux: experimental, without a full graphics certification** (above).
- **Limited testing:** Windows 11, one display, one GPU, one real pak with 32-bit terrain plus lab paks. Several
  displays and a physical keyboard were not tested. This is not a claim of compatibility with every pakset.
- **Code base:** official Simutrans SVN trunk **r12377**; no later trunk revision is included.
- **Everything listed for testing-6 still applies**, including the issues of the official trunk r12377 itself (the
  private-way sign crash when founding a city, empty scenario texts, whispers in the chat's public tab, the music volume
  after off/on; Windows 8 or newer is required).

Full list: `docs/KNOWN-ISSUES-r12377-testing-7.md`.

## Where it was tested

- **Windows 11 x64 — certified with limitations:** SDL2 (Direct3D 9) and SDL3 (Direct3D 11), one 3440×1440 display,
  MinGW builds. Limitations: one machine and display, no physical keyboard, the composition cost above.
- **Linux x86_64 — experimental, NOT certified:** built on Debian 13 (GCC 14.2) with the release recipe from the same
  source. Checked only: architecture, libraries, start and clean exit (SDL's dummy driver under WSL 2, including the
  path of the fix) and one WSLg X11 window. WSL 2 / WSLg is not a native Linux desktop.
- **Not tested:** macOS, Android, ARM, a native Linux desktop, several displays.

Platforms: x86_64 only. Windows 8 or newer; Linux with glibc 2.38 or newer; both with SDL2 or SDL3.

## How to test

1. Unpack into a new, empty folder and add a pakset. No pakset is included.
2. Start `simutrans-rgba32` with `SIMUTRANS_XR32=1`, as shown above.
3. If you make a pakset: try a `LightTexture` or `ClimateTexture` with the greys above in it. Look at slopes, climate
   bands and snow, by day and at night, at every zoom.
4. On any pak with 32-bit terrain, nothing should look different from testing-6. Compare with `simutrans-16bit`.

## Downloads

| File | What |
| --- | --- |
| `simutrans-xr32-night-prototype-r12377-testing-7-win64-sdl2.zip` | Windows x64, SDL2 — certified with limitations |
| `simutrans-xr32-night-prototype-r12377-testing-7-win64-sdl3.zip` | Windows x64, SDL3 — certified with limitations |
| `simutrans-xr32-night-prototype-r12377-testing-7-linux-x86_64-sdl2.tar.gz` | Linux x86_64, SDL2 — experimental, not certified |
| `simutrans-xr32-night-prototype-r12377-testing-7-linux-x86_64-sdl3.tar.gz` | Linux x86_64, SDL3 — experimental, not certified |
| `makeobj-xr32-night-prototype-r12377-testing-7-windows-x86_64.zip`, `…-linux-x86_64.tar.gz` | makeobj — the program is byte-identical to testing-6's (testing-7 changes no makeobj file) |
| `xr32-night-prototype-r12377-testing-2-testkit.zip` | the test kit, unchanged since testing-2 |
| `xr32-night-prototype-r12377-testing-7-source.tar.gz` | the full source (commit `3f1b511`) |
| `XR32-NIGHT-PROTOTYPE-FULL.r12377-testing-7.diff` | everything XR32 changes against official r12377 |
| `SHA256SUMS.txt`, `NIGHT-PROTOTYPE-MANIFEST.md` | hashes and manifest |

The patch against testing-6, alone and in two steps, is in `patches/r12377-testing-7/` of the repository. No pakset and
no third-party art is included in any download.

## Acknowledgements

- **makie**, for spotting that corners of PAK128.german were drawn at 16 bits, for tracing it to special colours in
  `LightTexture` (naming `6b6b6b`), for the careful comparisons and the improved light map. makie's investigation led
  to this fix, and makie later noted in the forum that processing special colours in `LightTexture` serves no purpose,
  which is what testing-7 now does. **makie has not reviewed or tested this build**; nothing here implies an approval
  or a certification by makie.
- **prissi**, for the technical contributions to Simutrans and the night-image design this prototype builds on.
- The pakset artists and the Simutrans community, for their art, tests and feedback.

## Feedback

https://github.com/Dkijas/simutrans-true32-renderer/issues: give the build name, your OS and package, the pakset,
whether `SIMUTRANS_XR32=1` was set, and attach a PNG screenshot.
