**Experimental prototype for testers. Not integrated into Simutrans Standard, not an official Simutrans release, not a stable version.**

## What is fixed

**Trees on the edge of the map no longer darken or flicker.** Where a tree stands on the border of the map, its crown and shadow hang over the grey area outside the map. That area was only repainted now and then, so on every frame the semi-transparent parts of the tree were blended over the previous frame instead of over the grey: shadows went almost black, crowns became too strong, and everything snapped back whenever something else repainted the area. The area outside the map is now repainted on every frame it is visible.

![Same crop, same frames: after a full redraw both builds draw the same translucent shadow; revised-8 darkens frame by frame until it is black, revised-9 does not change](https://github.com/Dkijas/simutrans-true32-renderer/releases/download/rgba32-revised-9-r12261-testing-1/north32-ramp-strip.png)

*revised-8 (top) and revised-9 (bottom), same savegame, same camera, world paused. Left: the frame right after a full redraw. Then 1, 3, 9 and 70 frames later.*

**The ordinary 16-bit renderer had the same defect**, fainter (the shadow stopped at a dark grey instead of black). The change is in code both renderers share, so `simutrans-16bit` is fixed too — the first fix in this series that changes the 16-bit executable.

You will see it most on the **north and west** edges of a map (upper right and upper left on screen). On the south and east edges the brown earth wall normally covers that area, unless `draw_earth_border` is switched off.

Thanks to **makie** for the report and the recordings. **This fix has not been tried on makie's own savegame, and makie has not confirmed it yet.**

## Please check

Load **your own** savegames, especially any where you saw the edge flicker, scroll to a map edge with trees and watch for a while, then scroll and zoom. Compare with `simutrans-16bit` from the same package. Both "fixed for me" and "still there" are worth reporting.

## Downloads

| platform | backend | download |
|---|---|---|
| Linux x86_64 | SDL2 | `simutrans-rgba32-revised9-r12261-linux-x86_64-sdl2.tar.gz` |
| Windows x64 | SDL2 | `simutrans-rgba32-revised9-r12261-win64-sdl2.zip` |
| Windows x64 | SDL3 | `simutrans-rgba32-revised9-r12261-win64-sdl3.zip` |

Each package contains the 32-bit build to test **and** a 16-bit build of the same sources, the Simutrans data directory, the licences, the patches and `README-TESTERS.txt`. No pakset, no savegames, no configuration.

Also attached: `RGBA32-CANDIDATE-REVISED-9.diff` (the full patch against r12261), `FIX-6-simview-repaint-outside-every-frame.diff` (the new change on its own), `north32-ramp-strip.png` and `SHA256SUMS.txt`.

```
a02611b26a3a8c1c3147f654144bd7dddd78ab26cd7ff41ee1b66d3514f1c5da  simutrans-rgba32-revised9-r12261-linux-x86_64-sdl2.tar.gz
2be28d47534665becb4f03521d1ff345c373634ce12948a9d26a8697719d9c37  simutrans-rgba32-revised9-r12261-win64-sdl2.zip
718cc0adc14c9b51c7be4f8065325cd0ee4c13d2b56e0f51d78a6d3afbaf45c7  simutrans-rgba32-revised9-r12261-win64-sdl3.zip
484a46351a43810f3b4f3ed341f23c105d566188b0cce73864e6deb97bc83243  RGBA32-CANDIDATE-REVISED-9.diff
a3a22a7b1d95901910be6d9050c8da72af975782c062df2a78ee1b56b53870a3  FIX-6-simview-repaint-outside-every-frame.diff
5132969cda77fc8258ea973309d396d242ef32c1a408f51447ffa9e2773df661  north32-ramp-strip.png
```

## How it was tested

**The fix itself (validation, reused):** Linux, SDL2, under Xvfb, one pakset (pak128.german), one generated 64×64 world, both colour depths. On a paused scene at the north edge, revised-8 drifted 14 234 pixels away from the correct image within about nine frames after any full redraw and stayed there; revised-9 changed 0 pixels over 100 frames and matched revised-8's own full redraw pixel for pixel. West edge, the south/east edges, the map interior, scrolling, zooming and all four combinations of `draw_outside_tile` and `draw_earth_border` were checked the same way.

**These packages (new, for this release):**

- **Linux package** — extracted and run as a tester would, both executables, on the north-edge scene: steady over 100 frames, no leftovers after scroll and zoom, and **pixel-identical** to the validated build at both depths.
- **Windows packages** — all four executables (SDL2 and SDL3, 32 and 16 bit), run from the extracted zips **under Wine 10.0** on the same harness: steady over 100 frames, no leftovers after scroll and zoom, and **pixel-identical** to the validated build. As a control, the published revised-8 SDL2 package shows the defect on the same harness (14 234 pixels, the same number as on Linux). Wine is not a Windows desktop: this checks the exact Windows binaries and what they draw, not Windows itself.
- **Windows, natively** — all four executables start, read the pakset, load the edge savegame and run the world with no errors (headless; there is no GUI automation for Windows on the build machine).

**Not tested:** a native Windows desktop session with a visible window, Wayland, macOS, other paksets, and makie's savegame. The Linux runtime requirements were measured and are the same as revised-8's; the Debian 13 / Fedora 43 / Fedora 44 runs belong to revised-8 and were not repeated.

A frame-rate measurement taken while developing the fix (one scene, 24.6 vs 24.7 frames per second) showed no visible cost. It was a single scene, not a benchmark, and it is not a claim of improvement.

## Packaging change

The Windows zips of revised-8 and earlier store `\` as the folder separator inside the archive. Windows Explorer and 7-Zip handle that, but some other extractors (for example on Linux or under Wine) produce one flat file per entry. The revised-9 zips use `/`, as the ZIP format specifies. The files inside are unchanged.

## Builds

- **Linux:** Debian 13, GCC 14.2.0, CMake 3.31.6, SDL2 2.32.4, Release, multithreaded; bzip2 and miniupnpc linked statically. Needs glibc 2.38 or newer, GLIBCXX_3.4.29, CXXABI_1.3.9.
- **Windows:** MSYS2 MinGW-w64, GCC 16.1.0, SDL2 2.32.10 / SDL3 3.4.12, Release, multithreaded; SDL, libstdc++ and libwinpthread linked statically — no DLL travels with the package.

Compiler warnings are the same set as revised-8's on every build.

## Identity

- **candidate** `RGBA32-CANDIDATE-REVISED-9` = revised-8 + FIX-6
- **base** official SVN trunk **r12261** = git `ecca332dca25025833f5ce6641491f8886a3ef9f`
- revised-9 differs from revised-8 in **exactly one file**, `src/simutrans/display/simview.cc`
- full patch sha256 `484a46351a43810f3b4f3ed341f23c105d566188b0cce73864e6deb97bc83243`; FIX-6 sha256 `a3a22a7b1d95901910be6d9050c8da72af975782c062df2a78ee1b56b53870a3`
- the sources these binaries were built from are in this repository under `source/rgba32-revised-9-r12261/` at the tag; commit `9a0a6d7` is the code change
- revised-8 remains available, unchanged, at `rgba32-revised-8-r12261-testing-1`

## Known inherited defects

Unchanged, and not claimed harmless: the left shift of a negative value in the clipping code, the unlocked read of an image's recode flags in the main viewport, and the one-element overread when a **ground** image contains a semi-transparent pixel. All three reproduce in unmodified trunk.

## Reporting

https://github.com/Dkijas/simutrans-true32-renderer/issues — there is a "Bug / regression" template. Please include your distribution or Windows version, X11 or Wayland, the pakset and version, the steps from a fresh start, the zoom level, a screenshot, and the log from `-debug 3`. **For anything that flickers, a short screen recording is worth far more than a screenshot.**
