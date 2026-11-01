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
> **private-way sign** crashes the game. This happens in both programs of this release, as in testing-1. Avoid
> private-way signs next to places where a city may be founded. See *Known issues*.

## What this build is

- **The same Simutrans SVN trunk as testing-1, r12377** (the game shows "125.0.2 Nightly"). Nothing of the trunk
  changed; the official trunk's own defects listed in testing-1 are still there.
- **A consolidation of the XR32 32-bit renderer**: five fixes that make the 32-bit renderer behave like the 16-bit one,
  and **no new graphics feature**. The DAY + optional NIGHT image prototype, the pak syntax and makeobj are unchanged.
- **The 16-bit renderer is unchanged.** On Linux, `simutrans-16bit` and makeobj are byte-identical to testing-1's.

Each package has two programs: `simutrans-rgba32` (32-bit, XR32) and `simutrans-16bit` (the normal renderer, for
comparison).

## What's new since testing-1

- **Fixed: 32-bit interface images got dark at night.** Toolbar icons and other interface images keep their day
  colours at night, as 16-bit ones always did. The map's own night is unchanged. Measured with Frank's pak64.german:
  the toolbar's brightness at night was 0.34 of its brightness by day, now 0.97 (the 16-bit renderer: 0.97).
- **Fixed: 32-bit images could be smaller than the 16-bit ones when zoomed, and catenary wires thinner.** The 32-bit
  scaler now uses the 16-bit scaler's own geometry and sampling grid, and keeps its 8-bit colour. All 11,735 images of
  Frank's pak64.german have the 16-bit size and position at all 12 zoom steps (testing-1: 16–40 %), every toolbar icon
  has its 16-bit size, and the 122 catenary images of pak128.german keep the 16-bit wire pixels at every step.
- **Fixed: shared images.** When two objects have identical 16-bit images but different 32-bit pictures (day, night or
  company colour), each now shows its own 32-bit picture. Before, the one loaded last won: a night picture could appear
  on the wrong vehicle. Identical 32-bit pictures are still shared; the few extra images (one in Frank's pak64.german) cost no
  measurable memory.
- **Fixed: interface images drawn while the map is zoomed** used their 16-bit art; they now use their 32-bit art.
- **Fixed: small images that do not scale with the zoom** were placed a few pixels off; they are now where the 16-bit
  renderer puts them.
- **Not reproduced: the 3/2 zoom picture depending on the step before it** (testing-1's known issue A6). It was not
  changed; with this build and with testing-1 the pictures are identical on both routes. It stays listed as not
  reproduced, not as fixed.
- **Cost:** the first frame after a zoom change takes about **1 ms** longer (+9 % over a walk of nine zoom steps),
  because the 32-bit scaler now samples like the 16-bit one. Other frames, loading time and memory: no measurable
  difference.

## How to test

1. Unpack into a new, empty folder and add a pakset. No pakset is included.
2. Start `simutrans-rgba32` with `SIMUTRANS_XR32=1`, as shown above.
3. To check that XR32 is on, start once with `-log -debug 2` added. Without the switch, `simu.log` lists
   `Skipping unknown XR32-node` for every 32-bit image; with the switch, that line does not appear.
4. Things worth looking at:
   - the toolbar at night with a 32-bit icon pakset: it should stay as bright as by day while the map gets dark;
   - zooming all the way in and out, especially over catenary, compared with `simutrans-16bit`;
   - a pak with 32-bit art by day, at dusk, at night and back by day;
   - anything that looks different from `simutrans-16bit` with the same pakset.

The test kit (`xr32-night-prototype-r12377-testing-2-testkit.zip`) has ready-made test paks and step-by-step
instructions, now with two quick tests for the fixes (toolbar at night, zoom sizes). Its paks are byte-identical to
testing-1's.

## makeobj

Unchanged since testing-1: use the makeobj of this release to build test content.

    Windows:  set STLAB_XR32_WRITE=1
              makeobj.exe pak cards.pak ./
    Linux:    STLAB_XR32_WRITE=1 ./makeobj pak cards.pak ./

The DAY/NIGHT syntax (`night_images=1`, `X.night.png`, the company-colour images) is a prototype under review, not a
final pak format.

## Known issues

From this test build:

- Water tiles on sloped coasts stay 16-bit even with a 32-bit water pak; open-sea tiles use the 32-bit images (open,
  needs a design decision).
- Night pictures cost memory: about 75–93 % more 32-bit image memory for objects that have one (by design; testing-2
  adds nothing measurable).
- The DAY/NIGHT pak syntax is a prototype (by design).
- A zoom change costs about 1 ms more on its first frame (see above).
- Three lesser open points, none visible in play: identical 16-bit images are merged only within one pak file (16-bit
  reader behaviour, cause not established); a double-size (MRES) split of a shared image may lose its 32-bit data
  (found in the code, not reproduced); some 32-bit renderer memory is released only at exit (it does not grow).

From the official trunk r12377 itself, unchanged from testing-1. They affect both programs of this release and are not
caused by XR32:

- **Founding a city next to a private-way sign crashes the game** (since r12339).
- **Scenario text files come out empty** (since r12344), for example the information pages of the pak64 tutorial.
- **The chat's public tab shows private messages and hides public ones** (since r12344).
- **Music volume:** after switching the music off and on again it can play very quietly until the volume slider is
  moved (since r12362).
- **A toolbar icon whose image is zoomable shrinks with the map zoom** and stays wrong when you zoom back (since
  r12368), with the 16-bit renderer and without `SIMUTRANS_XR32=1`; with `SIMUTRANS_XR32=1` it does not happen.
- Sanitizer builds report misaligned memory in the tree, smoke and animal lists (since r12367); normal builds run.

**Where it was tested:** Windows 11 (SDL2 and SDL3), and Debian 13 under WSL 2 on Windows 11 (SDL2 and SDL3). Linux ran
with SDL's dummy video driver, and the SDL3 package also in WSLg's graphical session (X11 through XWayland and
Wayland, one display at scale 1.0). **WSLg is not a native Linux desktop:** a native X11 or Wayland desktop, HiDPI
above 1, the mouse wheel under Wayland and several displays were not tested. Not tested at all: macOS, Android, ARM.

Platforms: x86_64 only. Windows 8 or newer, and Linux with glibc 2.38 or newer; both with SDL2 or SDL3.

Details: `docs/KNOWN-ISSUES-r12377-testing-2.md` and the certification report `reports/XR32-CONSOLIDATION-01.md` in
the repository.

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
| `simutrans-xr32-night-prototype-r12377-testing-2-win64-sdl2.zip` | Windows x86_64, SDL2: `simutrans-rgba32.exe` + `simutrans-16bit.exe` |
| `simutrans-xr32-night-prototype-r12377-testing-2-win64-sdl3.zip` | the same with SDL3 |
| `simutrans-xr32-night-prototype-r12377-testing-2-linux-x86_64-sdl2.tar.gz` | Linux x86_64, SDL2 (glibc 2.38 or newer; uses your distribution's SDL2) |
| `simutrans-xr32-night-prototype-r12377-testing-2-linux-x86_64-sdl3.tar.gz` | the same with SDL3 (SDL3 3.4.12 is built in) |
| `makeobj-xr32-night-prototype-r12377-testing-2-windows-x86_64.zip` | makeobj for this build, Windows x86_64 |
| `makeobj-xr32-night-prototype-r12377-testing-2-linux-x86_64.tar.gz` | makeobj for this build, Linux x86_64 |
| `xr32-night-prototype-r12377-testing-2-testkit.zip` | **start here**: test paks, sources and instructions |
| `XR32-NIGHT-PROTOTYPE-FULL.r12377-testing-2.diff` | the complete XR32 source change against official trunk r12377 |
| `NIGHT-PROTOTYPE-MANIFEST.md` | exact identities of everything above |
| `SHA256SUMS.txt` | SHA-256 of every file |

The source change is also in the repository, as one full diff and as five steps on top of testing-1:
`patches/r12377-testing-2/` (`APPLY-ORDER.txt`).

The Windows executables were built with the same upstream build fix as testing-1, which is **not** part of the XR32
change: official r12377's `CMakeLists.txt` links `old32`, a typo for `ole32`; the fix changes that one word
(`patches/r12377/upstream-build-fix/`, also inside the Windows packages).

testing-1 and all earlier releases stay available and unchanged.
