> **IMPORTANT: start the game with `SIMUTRANS_XR32=1`, or you will not see any 32-bit art.**
>
> Windows (cmd): `set SIMUTRANS_XR32=1`, then `simutrans-rgba32.exe` ·
> Windows (PowerShell): `$env:SIMUTRANS_XR32="1"`, then `.\simutrans-rgba32.exe` ·
> Linux: `SIMUTRANS_XR32=1 ./simutrans-rgba32`
>
> Without it, paks with 32-bit data still load and play, but the game draws their old 16-bit images.
> makeobj has its own switch: `STLAB_XR32_WRITE=1` writes the 32-bit data into the paks it builds.

**Experimental test build.** Not an official Simutrans release, not merged into Simutrans, not revised-12 and not a
stable version. Use a separate folder and copies of your savegames. Nothing is promised about loading savegames or paks
made with this build in later test builds.

## What this build is

- **Simutrans SVN trunk r12337**, the current development version. It includes r12336's extra zoom-in levels (9/8,
  5/4, 11/8 and 7/4; the zoom is now shown in the display settings) and r12337's network fix (a socket is no longer
  reused after a failed connection).
- **The XR32 32-bit renderer with the DAY + optional NIGHT image prototype** of the previous test build (r12322):
  32-bit images can carry an optional night picture and explicit company colours. The prototype works as before; this
  build moves it to the newer trunk and fixes three display problems.
- **Old 16-bit paks keep the normal rendering path.** DAY/NIGHT applies only to content that provides it.

Each package has two programs: `simutrans-rgba32` (32-bit, XR32) and `simutrans-16bit` (the normal renderer, for
comparison).

## What's new since the r12322 test build

- **Moved to trunk r12337** (from r12322).
- **40 px toolbar icons** (for example Frank's pak64.german) get 40 px buttons.
- **32-bit toolbar icons follow `icon_scaling`.** With theme-large (150 %), a 40 px icon is shown at 60 px in a 60 px
  button. At 200 %, an icon that comes with its own 2× picture uses that picture.
- **Thin 32-bit images at strong zoom-out** (reported by makie). From zoom 3/8 down, catenary poles were drawn at their
  full size and beside the track. An image that becomes less than one pixel wide or high is now left out, as the 16-bit
  renderer does.

## How to test

1. Unpack into a new, empty folder and add a pakset. No pakset is included.
2. Start `simutrans-rgba32` with `SIMUTRANS_XR32=1`, as shown above.
3. To check that XR32 is on, start once with `-log -debug 2` added. Without the switch, `simu.log` lists
   `Skipping unknown XR32-node` for every 32-bit image; with the switch, that line does not appear.
4. Things worth looking at:
   - a pak with 32-bit art by day, at dusk, at night and back by day;
   - zooming all the way out and back to 100 %, especially over catenary;
   - the toolbar with theme-large, and with a 40 px icon pakset;
   - sea and coast with a 32-bit water pak.

The test kit (`xr32-night-prototype-r12337-testkit.zip`) has ready-made test paks: DAY/NIGHT test cards, a thin-pole
object for the extreme-zoom check, and step-by-step instructions.

## makeobj

Use the makeobj of this release to build test content:

    Windows:  set STLAB_XR32_WRITE=1
              makeobj.exe pak cards.pak ./
    Linux:    STLAB_XR32_WRITE=1 ./makeobj pak cards.pak ./

A pak built this way also loads in the other builds we tested, which ignore the 32-bit data: the official Simutrans
r12322 and r12337 and `simutrans-16bit`.

The DAY/NIGHT syntax (`night_images=1`, `X.night.png`, the company-colour images) is a prototype under review, not a
final pak format. Older makeobj versions ignore `night_images=1` with a warning.

## Known issues

- **Shared images:** the game merges images whose 16-bit data is identical without looking at their 32-bit data, so two
  objects can end up showing the same 32-bit picture (seen: a night picture on the wrong vehicle; one catenary mast in
  pak128.german). Open.
- At some zoom levels a 32-bit image is a few pixels smaller than the 16-bit one (rounding). Not new.
- One catenary wire image of pak128.german (`catenary_120`, north-east) keeps different rows than the 16-bit renderer
  away from 100 %. Not new.
- At night, 32-bit toolbar icons and other 32-bit interface images are darkened like the map; 16-bit ones are not.
  Not new.
- Water tiles on sloped coasts stay 16-bit even with a 32-bit water pak; open-sea tiles use the 32-bit images.
- Night pictures cost memory: about 75–93 % more 32-bit image memory for objects that have one.
- x86_64 only (Windows SDL2/SDL3, Linux SDL2 with glibc 2.38 or newer). Not tested: macOS, Android, ARM.

Details: `docs/KNOWN-ISSUES-r12337.md` in the repository.

## Feedback

Please report problems at <https://github.com/Dkijas/simutrans-true32-renderer/issues> and include:

- this release's tag;
- your OS and package (Windows SDL2 / SDL3, Linux);
- the pakset;
- whether `SIMUTRANS_XR32=1` was set;
- a PNG screenshot.

## Downloads

| File | What |
| --- | --- |
| `simutrans-xr32-night-prototype-r12337-win64-sdl2.zip` | Windows x86_64, SDL2: `simutrans-rgba32.exe` + `simutrans-16bit.exe` |
| `simutrans-xr32-night-prototype-r12337-win64-sdl3.zip` | the same with SDL3 |
| `simutrans-xr32-night-prototype-r12337-linux-x86_64-sdl2.tar.gz` | Linux x86_64, SDL2 (glibc 2.38 or newer) |
| `makeobj-xr32-night-prototype-r12337-windows-x86_64.zip` | makeobj for this build, Windows x86_64 |
| `makeobj-xr32-night-prototype-r12337-linux-x86_64.tar.gz` | makeobj for this build, Linux x86_64 |
| `xr32-night-prototype-r12337-testkit.zip` | **start here**: test paks, sources and instructions |
| `XR32-NIGHT-PROTOTYPE-FULL.r12337.diff` | the complete source change against official trunk r12337 |
| `NIGHT-PROTOTYPE-MANIFEST.md` | exact identities of everything above |
| `SHA256SUMS.txt` | SHA-256 of every file |

The r12322 test build and all earlier releases stay available and unchanged.
