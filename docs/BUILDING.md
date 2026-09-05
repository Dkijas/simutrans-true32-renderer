# Building

The candidate builds with the GNU Makefile exactly as trunk does; the only
new knob is `COLOUR_DEPTH=32`. These are the configurations that were
certified (Windows, MSYS2 MinGW-w64 g++ 16.1.0); the same Makefile rules
apply on Linux (the Linux compile smoke used g++ 15.2 with SDL2 / SDL3
from the distribution).

## 1. Base tree

    svn checkout -r 12254 svn://servers.simutrans.org/simutrans/trunk simutrans-true32
    cd simutrans-true32
    svn patch --strip 1 /path/to/true32-r12254.diff      # or: patch -p1 < true32-r12254.diff
    svn status                                            # 12 M + 6 A expected

## 2. config.default

Copy `config.template` to `config.default` and set the backend and OS.
The certified builds used exactly these non-comment lines (Windows):

    BACKEND := gdi          # or sdl2, or sdl3
    OSTYPE := mingw         # linux on Linux
    MSG_LEVEL := 3
    OPTIMISE := 1
    MULTI_THREAD := 1

## 3. Build

Legacy 16-bit lane (the trunk default; `COLOUR_DEPTH` defaults to 16):

    make -j12 COLOUR_DEPTH=16

TRUE32 lanes - one build tree per backend/depth, from a clean `build/`:

    # GDI, 32-bit               (BACKEND := gdi  in config.default)
    make -j12 COLOUR_DEPTH=32

    # SDL2, 32-bit              (BACKEND := sdl2)
    make -j12 COLOUR_DEPTH=32

    # SDL3, 32-bit              (BACKEND := sdl3)
    make -j12 COLOUR_DEPTH=32

On MSYS2 the make binary is `mingw32-make`. The executable lands in
`build/default/sim.exe` (`build/default/sim` on Linux). `COLOUR_DEPTH` is
compiled into the executable (`-DCOLOUR_DEPTH=32`) and selects
`src/simutrans/display/simgraph32.cc` instead of `simgraph16.cc`; do not
mix objects of the two depths in one `build/`.

Certified outcome (fresh, non-incremental builds of the candidate): all six
Windows combinations (GDI/SDL2/SDL3 x 16/32) link with 0 errors and with
warning sets identical to the pre-candidate references (GDI-16 identical
to pure trunk); Linux SDL2-16, SDL2-32 and SDL3-32 link with 0 errors.
Reproduced during packaging from a fresh r12254 working copy plus the
patch: GDI-16, GDI-32 and SDL3-32 (see PROVENANCE.md).

## Not covered

- CMake and Visual Studio: the project files still list `simgraph16.cc`
  explicitly; add `src/simutrans/display/simgraph32.cc` (and the new
  headers, for the IDE) to build a 32-bit target with them. Not part of
  the certified candidate.
- `COLOUR_DEPTH=0` (headless server): unchanged, not exercised by this
  work.

## Runtime

Nothing changes on the command line. Useful for testing:

    sim -use_workdir -singleuser -objects pak128 -lang en -screensize 1024x640 -load mygame.sve
    sim ... -threads 1            # one display lane (the default is min(12, cores))
    sim ... -freeplay             # avoids bankruptcy ending a long test
