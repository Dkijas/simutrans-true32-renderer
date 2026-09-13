**Experimental prototype for testers. Not integrated into Simutrans Standard, not an official Simutrans release, not a stable version.**

## What is fixed

- **The minimap drew outside its own window when you changed the zoom.** It painted over the toolbar and the map view for a single frame per zoom step, which is why it never showed up in a screenshot.
- **Image sizes and origins were reported as zero to the GUI**, which misaligned icons — in the factory window, and anywhere else small images sit next to text.
- **The Windows packages also pick up the earlier font fix.** The previous Windows pre-releases (revised-5 and revised-6) could start with panels and buttons but no text at all; these cannot.

Thanks to **Yona-TYT** for the report and, above all, for the video. The minimap defect lasted one frame at a time and the recording is what made it findable.

Both defects are in the 32-bit renderer only. A 16-bit build of the same sources has neither, so neither comes from Simutrans trunk.

**The tree flicker at the map edge is still open.** The image-offset fix touches two places that are not GUI code — one decides whether an object near the map edge needs the background redrawn, the other measures how far a tree rises above its tile — so it is a **lead**. It has not been reproduced here and is **not** declared fixed. If you can still see it on this build, or can no longer see it, both are worth knowing.

## Downloads

| platform | backend | download |
|---|---|---|
| Linux x86_64 | SDL2 | `simutrans-rgba32-revised8-r12261-linux-x86_64-sdl2.tar.gz` |
| Windows x64 | SDL2 | `simutrans-rgba32-revised8-r12261-win64-sdl2.zip` |
| Windows x64 | SDL3 | `simutrans-rgba32-revised8-r12261-win64-sdl3.zip` |

Each package contains the 32-bit build to test **and** a 16-bit build of the same sources for side-by-side comparison, plus the Simutrans data directory, the licences, the patches and `README-TESTERS.txt`. No pakset, no savegames, no configuration.

Also attached: `RGBA32-CANDIDATE-REVISED-8.diff` (the full patch against r12261), `FIX-4` and `FIX-5` as separate patches for review, and `SHA256SUMS.txt`.

```
0cac9d0944ccf1f86117e13f02a7a6bd3615a7ef93a7fbd7dc91e4606d5bd680  simutrans-rgba32-revised8-r12261-linux-x86_64-sdl2.tar.gz
731b840e562e0286a22f8107efaa508cd7a7a2dc524918f22d185b20a8930bd6  simutrans-rgba32-revised8-r12261-win64-sdl2.zip
1e794f2f22559e1eefe0f251c4deeff6c1a9bddffe9b0bec388c9ec95d30da16  simutrans-rgba32-revised8-r12261-win64-sdl3.zip
bdc0a3262b0b8168d148d643ce418256fdf08254128934883662db7794bc64ea  RGBA32-CANDIDATE-REVISED-8.diff
6ce28be3975a2bff4b9495a53ba665531dbf7b5c84b74b186079efd5181f1291  FIX-4-simgraph32-draw-array-clip.diff
3c7ce45c7d6907c9e8ed65a628c5fa5c6e3c52be22a92498d379459f29fbc890  FIX-5-simgraph32-image-offsets.diff
```

## Running it

Extract into **a folder of its own**, not over your normal installation. Put your own pakset inside it (`pak/` for pak64, `pak128/`), and **copies** of any savegames you want to try under `save/`. Then:

```
./simutrans-rgba32 -singleuser        # the build to test
./simutrans-16bit  -singleuser        # the same world, ordinary renderer
```

On Windows, `simutrans-rgba32.exe` and `simutrans-16bit.exe`. `-singleuser` keeps settings, saves and screenshots inside that folder instead of your home directory or user profile. Please use **copies** of savegames you care about.

Reporting differences against `simutrans-16bit` from the same package is far more useful than against memory of another version.

## Linux build

Compiled on **Debian 13**. Measured on these exact bytes:

```
glibc      2.38 or newer
libstdc++  GLIBCXX_3.4.29, CXXABI_1.3.9

libSDL2-2.0.so.0  libpng16.so.16  libz.so.1       libfontconfig.so.1
libfreetype.so.6  libzstd.so.1    libstdc++.so.6  libgcc_s.so.1
libm.so.6         libc.so.6
```

bzip2 and miniupnpc are linked statically and are not asked for at all — they are the two libraries whose name differs between distributions. `THIRD-PARTY-STATIC.txt` in the archive carries their notices.

**Tested** by extracting and running this same archive, with no symlink created anywhere, on **Debian 13, Fedora 43 and Fedora 44**: 0 unresolved libraries on every one, text rendered, pakset and savegame loaded, no errors. Those runs are under **Xvfb — a real X server, but a virtual one.** They exercise the X11 path, font resolution and drawing. They are not a Wayland session and not anyone's desktop, and no claim is made about Wayland, compositors, GPUs or drivers.

## Windows build

Compiled with MSYS2 MinGW-w64, Release, multithreaded, SDL2 and SDL3. SDL, libstdc++ and libwinpthread are linked statically: both executables import only Windows system DLLs and no DLL travels with the package.

**What was checked on Windows, exactly:** all four executables were started headlessly, read a pakset and entered a world with zero errors, and the renderer and colour depth each was built with were verified in the build. There is **no GUI automation for Windows on the build machine**, so the pixel measurements behind the two fixes are Linux measurements from the same sources — they are not claimed as Windows visual tests. The Windows binaries are what most needs a tester's eyes.

## Identity

- **candidate** `RGBA32-CANDIDATE-REVISED-8` = revised-7 + FIX-4 + FIX-5, carrying FIX-1, FIX-2 and FIX-3 as well
- **base** official SVN trunk **r12261** = git `ecca332dca25025833f5ce6641491f8886a3ef9f`
- revised-8 differs from revised-7 in **exactly one file**, `src/simutrans/display/simgraph32.cc`
- the full sources these binaries were built from are in this repository, under `source/rgba32-revised-8-r12261/` at the tag

## Known inherited defects

Unchanged, and not claimed harmless: the left shift of a negative value in the clipping code, the unlocked read of an image's recode flags in the main viewport, and the one-element overread when a **ground** image contains a semi-transparent pixel. All three reproduce in unmodified trunk.

XR32 is unchanged: it needs a 32-bit build, `SIMUTRANS_XR32=1` in the environment, and a pakset that contains the nodes. No public pakset does, and makeobj is not part of this candidate.

## Reporting

https://github.com/Dkijas/simutrans-true32-renderer/issues — there is a "Bug / regression" template. Please include your distribution or Windows version, X11 or Wayland, the pakset and version, the steps from a fresh start, the zoom level and which window was open, a screenshot, and the log from `-debug 3`. **If something flashes, a short screen recording is worth far more than a screenshot.**
