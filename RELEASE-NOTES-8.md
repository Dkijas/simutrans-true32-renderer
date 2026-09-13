**Experimental prototype for testers. Not integrated into Simutrans Standard, not an official Simutrans release, not a stable version.**

Linux **and** Windows. The Windows packages here are the first ones to carry the font fix from revised-7 (FIX-3) as well as the two new fixes below — the earlier `rgba32-revised-5-…-win64-sdl2.zip` and `rgba32-revised-6-…-win64-sdl3.zip` have none of the three.

## What is fixed

Both were reported by **Yona-TYT** on revised-7, and both are defects of the 32-bit renderer alone: a 16-bit build of the same sources has neither, so neither comes from Simutrans trunk. They are two independent bugs with two independent causes, not one bug seen twice.

- **The minimap painted over the toolbar and the map view when zoomed.** When you zoom the minimap, `map_frame_t::draw()` first tells the minimap which part of the map it is showing and then re-centres the view by moving the scrollbars, so for one frame the bitmap is handed to the renderer at a position partly outside the map window. The 16-bit renderer clips it against the window and nobody notices; the 32-bit `draw_array` clipped against the **screen** and painted the map across everything else. It flashes rather than sticks, because the next frame repaints — which is why it never showed up in a screenshot and why Yona's video is what made it findable. (FIX-4.)
- **Icons misplaced in the factory window — and in a lot of other places.** The two functions that tell the GUI how big an image is and where its origin sits were still the **headless renderer's stubs** in the 32-bit renderer, returning an empty rectangle for every image: all 6962 of them with pak64. Layout columns therefore got no width, and every icon was drawn displaced by its own offset. Fourteen call sites read those two functions, including buttons, the convoy list, the vehicle list, the depot lists and inline images in help texts. (FIX-5.)

The **tree flicker** at the map edge is **still open and still not reproduced.** Two of those fourteen call sites are not GUI code — one decides whether an object near the map edge needs the background redrawn, the other measures how far a tree rises above its tile — so both behaved differently in the 32-bit build, and both are now fixed. That is a lead, not a diagnosis: nothing here claims it was the cause.

## How it was checked

Each defect has its own check, and each check was shown able to fail.

| | minimap | icons |
|---|---|---|
| revised-7, 32-bit | 31 frames of 5386 paint above the window, worst 28.10 % | icons 4 px right and 4 px low |
| 16-bit, same sources | 0 | correct |
| **revised-8** | **0** | **identical to the 16-bit build, to the pixel** |
| only the other fix applied | still fails | still fails |
| 16-bit renderer given the 32-bit code | 31 frames, worst 28.11 % | wrong, by the same 4 px |

The minimap check records 90 s of screen at 60 fps with the world paused and counts, in every frame, how much of the area above the map window got repainted. A screenshot cannot see this defect at all.

The 16-bit binary is **byte-identical** with and without these fixes — both live in `simgraph32.cc`, which a 16-bit build does not compile.

## Downloads

| file | what |
|---|---|
| `simutrans-rgba32-revised8-r12261-linux-x86_64-sdl2.tar.gz` | **Linux x86_64**, SDL2, built on Debian 13. glibc 2.38 or newer. |
| `simutrans-rgba32-revised8-r12261-win64-sdl2.zip` | **Windows x86_64**, SDL2. Self-contained, no DLLs. |
| `simutrans-rgba32-revised8-r12261-win64-sdl3.zip` | **Windows x86_64**, SDL3. Self-contained, no DLLs. |
| `RGBA32-CANDIDATE-REVISED-8.diff` | the full patch against r12261 these binaries were built from |
| `FIX-4-…`, `FIX-5-…` `.diff` | the two new changes on their own, for review |
| `*.sha256` | hashes |

Each package contains the 32-bit build to test **and** a 16-bit build of the same tree for comparison, plus the Simutrans data directory. No pakset, no savegames, no configuration.

```
simutrans-rgba32-revised8-r12261-linux-x86_64-sdl2.tar.gz  0cac9d0944ccf1f86117e13f02a7a6bd3615a7ef93a7fbd7dc91e4606d5bd680
  simutrans-rgba32                                         723e0f3562773f2de923657678a8cad2a842027538cdd67e2d79985e09119897
  simutrans-16bit                                          4d5c0b5d9a147967efe5ffb94cdab2fca29c409b28eab083a80b93cb2572ed62

simutrans-rgba32-revised8-r12261-win64-sdl2.zip            3210195ab1d3faa2574651484026140916efabdef9f1a6f6110ee8095ceb10d1
  simutrans-rgba32.exe                                     5561599f9a9fe5d83ff7872fd0fb824d0162284c5298704698cc19a67a757eae
  simutrans-16bit.exe                                      0d04f1fef8ccc1a92e5f486ad1e026163610164f473ffef6d6399635a97d3b06

simutrans-rgba32-revised8-r12261-win64-sdl3.zip            bc0ca0ce2cf5d091c8fb67be007ec5e9d9f8de2d1d68a61c095a16bfba5272e4
  simutrans-rgba32.exe                                     09687803366a84a3d3ecf9bd360b3ad7c225b23bb07dee486bfd9a74fac448a8
  simutrans-16bit.exe                                      780f457ac068b310967569e449eda9c97a326e2eb3161747169c31b76c6d5a4a

RGBA32-CANDIDATE-REVISED-8.diff                            bdc0a3262b0b8168d148d643ce418256fdf08254128934883662db7794bc64ea
FIX-4-simgraph32-draw-array-clip.diff                      6ce28be3975a2bff4b9495a53ba665531dbf7b5c84b74b186079efd5181f1291
FIX-5-simgraph32-image-offsets.diff                        3c7ce45c7d6907c9e8ed65a628c5fa5c6e3c52be22a92498d379459f29fbc890
```

## Identity

- **candidate** `RGBA32-CANDIDATE-REVISED-8` = revised-7 + FIX-4 + FIX-5, and it carries FIX-1, FIX-2 and FIX-3 as well
- **base** official SVN trunk **r12261** = git `ecca332dca25025833f5ce6641491f8886a3ef9f`
- revised-8 differs from revised-7 in **exactly one file**, `src/simutrans/display/simgraph32.cc`
- **Linux build** Debian 13, GCC 14.2.0, CMake 3.31.6, Release, multithreaded, SDL2 — bzip2 and miniupnpc linked statically, as in revised-7
- **Windows builds** MSYS2 MinGW-w64, Release, multithreaded, SDL2 and SDL3 — SDL, libstdc++ and libwinpthread linked statically, no DLL in the package

## Requirements, measured on these exact bytes

Linux:

```
glibc      2.38 or newer
libstdc++  GLIBCXX_3.4.29, CXXABI_1.3.9

libSDL2-2.0.so.0  libpng16.so.16  libz.so.1       libfontconfig.so.1
libfreetype.so.6  libzstd.so.1    libstdc++.so.6  libgcc_s.so.1
libm.so.6         libc.so.6
```

bzip2 and miniupnpc are linked statically and are not asked for at all — they are exactly the two libraries whose name diverges between distributions. `THIRD-PARTY-STATIC.txt` in the archive carries their notices.

Windows: both executables import only Windows system DLLs.

## Where this was actually run

The same Linux archive, extracted and run, with **no symlink created anywhere**:

| distribution | glibc | unresolved libraries | text in the pakset window | pakset + savegame | errors |
|---|---|---|---|---|---|
| **Debian 13** | 2.41 | **0** | yes (0.331663, same as 16-bit) | loads | 0 |
| **Fedora 43** | 2.42 | **0** | yes (0.332569, same as 16-bit) | loads | 0 |
| **Fedora 44** | 2.43 | **0** | yes (0.350964, same as 16-bit) | loads | 0 |

The font fallback was exercised separately by pointing fontconfig at a configuration with no fonts in it — the situation that produced the blank window before revised-7. There the 32-bit build renders text from the bundled `font/cyr.bdf` with exactly the same ink as the 16-bit build, 0.299406, on all three distributions.

**These are Xvfb runs: a real X server, but a virtual one.** They exercise the X11 path, the font resolution and the drawing. They are not a Wayland session and not anyone's desktop, and no claim is made about Wayland, compositors, GPUs or drivers.

All four Windows binaries were started headlessly, read the pakset and entered a world with zero errors. There is no GUI automation for Windows on the build machine, so the pixel measurements above are Linux measurements from the same sources.

## Running it

Extract into a directory of its own, **not** over your normal installation, put a pakset in `./pak` or `./pak128` and **copies** of your saves in `./save`, then run `simutrans-rgba32` (or `.exe`) with `-singleuser`, and `simutrans-16bit` beside it for comparison. Full instructions are in `README-TESTERS.txt` inside each package.

XR32 is unchanged: it needs a 32-bit build, `SIMUTRANS_XR32=1` in the environment, and a pakset that contains the nodes. No public pakset does, and makeobj is not part of this candidate.

## Known inherited defects

Unchanged, and not claimed harmless: the left shift of a negative value in the clipping code, the unlocked read of an image's recode flags in the main viewport, and the one-element overread when a **ground** image contains a semi-transparent pixel. All three reproduce in unmodified trunk.

## Reporting

https://github.com/Dkijas/simutrans-true32-renderer/issues — there is a "Bug / regression" template. Please include this tag, your distribution or Windows version, X11 or Wayland, the pakset and version, the steps from a fresh start, the zoom level and which window was open, a screenshot, and the log from `-debug 3`. **A short screen recording is worth a great deal for anything that flashes**: the minimap defect above lasted one frame at a time and was found by going through Yona's video frame by frame.
