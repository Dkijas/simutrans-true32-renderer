> **IMPORTANT: start the game with `SIMUTRANS_XR32=1`, or you will not see any 32-bit art.**
>
> Windows (cmd): `set SIMUTRANS_XR32=1`, then `simutrans-rgba32.exe` ·
> Windows (PowerShell): `$env:SIMUTRANS_XR32="1"`, then `.\simutrans-rgba32.exe` ·
> Linux: `SIMUTRANS_XR32=1 ./simutrans-rgba32`
>
> Without it, paks with 32-bit data still load and play, but the game draws their old 16-bit images.
> makeobj has its own switch: `STLAB_XR32_WRITE=1` writes the 32-bit data into the paks it builds.

**Experimental test build.** It is not an official Simutrans release, it is not merged into Simutrans, and it is not a
stable version. Use a separate folder and copies of your savegames. Nothing is promised about loading savegames or paks
made with this build in later test builds.

> **Warning, from the official trunk r12377 itself:** founding a city whose first road runs onto a road with a
> **private-way sign** crashes the game. This happens in both programs of this release; it was measured on these
> Windows executables. Avoid private-way signs next to places where a city may be founded. See *Known issues*.

## What this build is

- **Simutrans SVN trunk r12377**, the current development version (the game shows "125.0.2 Nightly"). Every change of
  the official trunk from r12338 to r12377 is included. Two of them matter for testers of this build:
  - **Toolbar icons** are now fitted to the theme's icon size once, when the toolbars are built, instead of on every
    frame (r12368).
  - **Windows 8 or newer** is required, because music on Windows now uses Media Foundation (r12361).
- **The XR32 32-bit renderer with the DAY + optional NIGHT image prototype** of the r12337 test build: 32-bit images
  can carry an optional night picture and explicit company colours. The prototype itself is unchanged; this build moves
  it to the newer trunk, adapts it to two trunk changes and fixes two problems with authored double-size (MRES) icons.
- **Old 16-bit paks keep the normal rendering path.** DAY/NIGHT applies only to content that provides it.

Each package has two programs: `simutrans-rgba32` (32-bit, XR32) and `simutrans-16bit` (the normal renderer, for
comparison).

## What's new since the r12337 test build

- **Moved to trunk r12377** (from r12337). The XR32 change merged onto r12377 without conflicts. Two trunk changes needed
  work in the 32-bit renderer:
  - the renderer function that r12371 added;
  - r12342's single-line text, which now draws a line break as a space (ported to the 32-bit renderer; not tested on
    its own in this build).
- **Toolbar icons and the theme:** with the r12368 change, 32-bit toolbar icons are still fitted like their 16-bit
  versions: 277 of 278 icons of Frank's pak64.german at theme-large have identical sizes, and the one other is a known
  rounding case (the second known issue below). Switching the theme while the game runs refits them
  (40 px → 60 px → 40 px).
- **Fixed: double-size (MRES) icons on Linux.** An authored double-size toolbar icon was silently replaced by the
  enlarged normal icon when the system listed the pakset's files in an unlucky order and another object had the same
  name as the menu. In Frank's pak64.german the menu and the mouse cursor are both called `GeneralTools`. This always
  happened on native Linux file systems, and never on Windows. The r12337 test build has this problem too.
- **Fixed: double-size icons after zooming the map.** After zooming the map in and out, a toolbar icon with an authored
  double-size picture showed the enlarged normal picture. It now keeps the double-size picture. The r12337 test build
  has this problem too.
- **makeobj:** the ordinary 16-bit image of a semi-transparent pixel now follows the official makeobj of r12375. A pak
  with semi-transparent pixels is therefore not byte-identical to one built with the r12337 test build; its 32-bit data
  is identical.

## How to test

1. Unpack into a new, empty folder and add a pakset. No pakset is included.
2. Start `simutrans-rgba32` with `SIMUTRANS_XR32=1`, as shown above.
3. To check that XR32 is on, start once with `-log -debug 2` added. Without the switch, `simu.log` lists
   `Skipping unknown XR32-node` for every 32-bit image; with the switch, that line does not appear.
4. Things worth looking at:
   - a pak with 32-bit art by day, at dusk, at night and back by day;
   - zooming all the way out and back to 100 %, especially over catenary;
   - the toolbar with theme-large and with a 40 px icon pakset, also switching the theme while the game runs;
   - sea and coast with a 32-bit water pak.

The test kit (`xr32-night-prototype-r12377-testkit.zip`) has ready-made test paks: DAY/NIGHT test cards, a thin-pole
object for the extreme-zoom check, and step-by-step instructions.

## makeobj

Use the makeobj of this release to build test content:

    Windows:  set STLAB_XR32_WRITE=1
              makeobj.exe pak cards.pak ./
    Linux:    STLAB_XR32_WRITE=1 ./makeobj pak cards.pak ./

A pak built this way also loads in the other builds we tested, which ignore its 32-bit data: the official Simutrans
r12377 (16-bit) and `simutrans-16bit`.

The DAY/NIGHT syntax (`night_images=1`, `X.night.png`, the company-colour images) is a prototype under review, not a
final pak format. Older makeobj versions ignore `night_images=1` with a warning.

## Known issues

From this test build (all of them were already in the r12337 test build):

- **Shared images:** when paks load, the game merges images whose 16-bit data is identical, without looking at their
  32-bit data. Two objects can therefore end up showing the same 32-bit picture. Seen: a night picture on the wrong
  vehicle, and one catenary mast in pak128.german. Which images merge depends on the order in which your system lists
  the files, so Windows and Linux can show different results here. Open.
- At some zoom levels a 32-bit image is a few pixels smaller than the 16-bit one (rounding).
- One catenary wire image of pak128.german (`catenary_120`, north-east) keeps different rows than the 16-bit renderer
  away from 100 %.
- At night, 32-bit toolbar icons and other 32-bit interface images are darkened like the map; 16-bit ones are not
  (measured again in this build: the toolbar is about a third as bright at night).
- Water tiles on sloped coasts stay 16-bit even with a 32-bit water pak; open-sea tiles use the 32-bit images.
- Night pictures cost memory: about 75–93 % more 32-bit image memory for objects that have one.

From the official trunk r12377 itself. These affect both programs of this release and are not caused by XR32. They
are reported here, and are not fixed in this build:

- **Founding a city next to a private-way sign crashes the game** (since r12339). Measured on this release's Windows
  executables, both of them; the official Windows builds made with Visual Studio happen not to crash.
- **Scenario text files come out empty** (since r12344), for example the information pages of the pak64 tutorial.
- **The chat's public tab shows private messages and hides public ones** (since r12344).
- **Music volume:** after switching the music off and on again it can play very quietly until the volume slider is
  moved (since r12362; from reading the code, not measured in this build).
- **A toolbar icon whose image is zoomable shrinks with the map zoom** and stays wrong when you zoom back (since
  r12368). This was measured with the 16-bit renderer and with `simutrans-rgba32` without `SIMUTRANS_XR32=1`, using a
  double-size test icon; with `SIMUTRANS_XR32=1` it does not happen. The icons of Frank's pak64.german are not affected.
- In `-debug` runs with sanitizers, the official r12377 reports misaligned memory in its tree, smoke and animal lists
  (since r12367). Normal builds like these run, and the automated tests pass.

Platforms: x86_64 only. Windows 8 or newer, and Linux with glibc 2.38 or newer; both with SDL2 or SDL3. Not tested:
macOS, Android, ARM.

Details: `docs/KNOWN-ISSUES-r12377.md` in the repository.

## Feedback

Please report problems at <https://github.com/Dkijas/simutrans-true32-renderer/issues> and include:

- this release's tag;
- your OS and package (Windows or Linux, SDL2 or SDL3);
- the pakset;
- whether `SIMUTRANS_XR32=1` was set;
- a PNG screenshot.

## Downloads

| File | What |
| --- | --- |
| `simutrans-xr32-night-prototype-r12377-win64-sdl2.zip` | Windows x86_64, SDL2: `simutrans-rgba32.exe` + `simutrans-16bit.exe` |
| `simutrans-xr32-night-prototype-r12377-win64-sdl3.zip` | the same with SDL3 |
| `simutrans-xr32-night-prototype-r12377-linux-x86_64-sdl2.tar.gz` | Linux x86_64, SDL2 (glibc 2.38 or newer; uses your distribution's SDL2) |
| `simutrans-xr32-night-prototype-r12377-linux-x86_64-sdl3.tar.gz` | the same with SDL3 (SDL3 3.4.12 is built in, so it also runs where the distribution has no SDL3) |
| `makeobj-xr32-night-prototype-r12377-windows-x86_64.zip` | makeobj for this build, Windows x86_64 |
| `makeobj-xr32-night-prototype-r12377-linux-x86_64.tar.gz` | makeobj for this build, Linux x86_64 |
| `xr32-night-prototype-r12377-testkit.zip` | **start here**: test paks, sources and instructions |
| `XR32-NIGHT-PROTOTYPE-FULL.r12377.diff` | the complete XR32 source change against official trunk r12377 |
| `NIGHT-PROTOTYPE-MANIFEST.md` | exact identities of everything above |
| `SHA256SUMS.txt` | SHA-256 of every file |

The Windows executables were built with one upstream build fix that is **not** part of the XR32 change:

- **What:** official r12377's `CMakeLists.txt` links `old32`, a typo for `ole32`, so no Windows CMake build of r12377
  links. The fix changes that one word.
- **Where:** `patches/r12377/upstream-build-fix/` in the repository, and inside the Windows packages.

The r12337 test build and all earlier releases stay available and unchanged.
