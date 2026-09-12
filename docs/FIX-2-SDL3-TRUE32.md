# FIX-2 — the SDL3 backend could not build the 32-bit renderer

**Status: confirmed absent in revised-5, adapted, built and verified on pixels.**

## What was actually missing

Not the backend. `src/simutrans/sys/simsys_s3.cc` in revised-5 already carries
every 32-bit path:

- `SDL_PIXELFORMAT_ARGB8888` with an `0xFF000000` alpha-mask check under
  `#if COLOUR_DEPTH == 32`;
- `dr_textur_init()` and `dr_textur_resize()` typed `PIXVAL*`;
- the framebuffer declared `static PIXVAL *framebuffer`, allocated as
  `pitch_pixels * height * sizeof(PIXVAL)`;
- `dr_flush()` and `dr_textur()` computing every pitch and offset with
  `sizeof(PIXVAL)`;
- `get_system_color()` returning full ARGB8888.

What was missing was the **build-system selection**. The `sdl3` branch of
`CMakeLists.txt` hard-coded

```cmake
target_sources(simutrans PRIVATE src/simutrans/display/simgraph16.cc src/simutrans/sys/simsys_s3.cc
    src/simutrans/sound/sdl3_sound.cc)
...
set(SIMUTRANS_COLOUR_DEPTH 16)
```

and never consulted `SIMUTRANS_TRUE32`. So `-DSIMUTRANS_BACKEND=sdl3
-DSIMUTRANS_TRUE32=ON` silently produced an ordinary 16-bit build — changing the
backend name was genuinely not enough.

## The change

`CMakeLists.txt` **only**, mirroring the `sdl2` branch: pick `simgraph32.cc` and
`stlab_xr32.cc` instead of `simgraph16.cc` when `SIMUTRANS_TRUE32` is on, and set
`SIMUTRANS_COLOUR_DEPTH` accordingly. The stale comment claiming "simgraph16
still draws into a CPU framebuffer" is corrected.

No renderer, no backend, and no pixel, alpha, clipping, resize or presentation
contract is touched. No event, audio or text-input migration is involved.

`patches/FIX-2-cmake-sdl3-true32.diff`, sha256
`9b78236923a98f7defbe29b5e4a41ac505d7c0f12984697f3bf48efc76df2918`.

## Identity

Because it changes code, this is a **new candidate**:
`RGBA32-CANDIDATE-REVISED-6` = revised-5 + FIX-2, sha256
`62b27b8a18e5cbb5775d40ef43d0ca2fa24a06b6c04113889bb30583b3f1a99e`, 26 files,
+6750/−820.

- r12261 + revised-6 == the tree the binaries were built from: **2164/2164 files,
  nothing differing**.
- revised-6 against revised-5, both applied to the same pristine export:
  **2164 files each, differing in exactly one file, `CMakeLists.txt`**.

It does not carry revised-4's or revised-5's hash. What it *does* inherit
unchanged is everything below `CMakeLists.txt`: revised-5's renderer, including
the black-window fix, is byte-identical here.

## Verified

Built four ways from the same tree — Windows and Linux, SDL2 and SDL3, 32 and 16
bit — all rc=0.

Read from the binaries: the SDL3 32-bit builds carry `g_simgraph32` and not
`g_simgraph16`, compile `simsys_s3.cc`, link SDL3 (`libSDL3.so.0` on Linux), and
are built with `COLOUR_DEPTH=32`; the 16-bit ones are the mirror image.

On a real (virtual) X server, non-black fraction of the screen:

| Linux, revised-6, SDL3 | at start | after resize | zoom changes | info window changes |
|---|---|---|---|---|
| 32-bit, no pakset | **0.120108** | 0.262875 | — | — |
| 16-bit, no pakset | 0.126965 | 0.126965 | — | — |
| 32-bit, pakset + save | 0.383090 | 0.656096 | 633 075 px | 634 157 px |
| 16-bit, pakset + save | 0.382424 | 0.654582 | 627 525 px | 634 326 px |

and the SDL2 control from the same checklist:

| Linux, revised-5, SDL2 | at start | after resize | zoom | info window |
|---|---|---|---|---|
| 32-bit, no pakset | 0.119981 | 0.119981 | — | — |
| 32-bit, pakset + save | 0.383062 | 0.655910 | 633 094 px | 634 374 px |

So on SDL3 the interface is painted at startup **without a resize** — the
black-window fix carries over — the world loads, and zoom and an info window
both change the screen. 0 errors and no pixel-format error in any run.

On Windows all four binaries start headless, read a pakset and enter a loaded
world with 0 errors. This machine has no GUI automation for Windows, so the
Windows packages are **not** pixel-verified; the pixel evidence above comes from
the same trees and the same backends on Linux.

## Not fixed here

The tree flicker at the map edge. Still open, still not reproduced.
