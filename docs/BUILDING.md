# Building

The renderer is chosen at build time by `COLOUR_DEPTH`: 16 builds
`simgraph16.cc` (RGB565, the default, identical to trunk), 32 builds
`simgraph32.cc` (ARGB8888, TRUE32). Exactly one renderer is compiled into
the executable. All three build systems of the project apply the same
rule and reject any other value; the headless (`none` / Simutrans-Server)
build keeps `COLOUR_DEPTH=0` and has no renderer.

## 1. Base tree

    svn checkout -r 12254 svn://servers.simutrans.org/simutrans/trunk simutrans-true32
    cd simutrans-true32
    svn patch --strip 1 /path/to/true32-r12254-v2.diff     # or: patch -p1 < true32-r12254-v2.diff
    svn status                                              # 17 M + 6 A expected

(`true32-r12254.diff`, v1, is the renderer alone - it needs the Makefile;
v2 adds the CMake / Visual Studio selection.)

## 2. GNU Makefile (certified: RECERTIFICATION-02)

Copy `config.template` to `config.default` and set the backend and OS.
The certified builds used exactly these non-comment lines (Windows,
MSYS2 MinGW-w64):

    BACKEND := gdi          # or sdl2, or sdl3
    OSTYPE := mingw         # linux on Linux
    MSG_LEVEL := 3
    OPTIMISE := 1
    MULTI_THREAD := 1

    make -j12 COLOUR_DEPTH=16        # legacy lane (the default)
    make -j12 COLOUR_DEPTH=32        # TRUE32 lane

One build tree per backend/depth, from a clean `build/`; on MSYS2 the make
binary is `mingw32-make`. The executable is `build/default/sim.exe`
(`build/default/sim` on Linux).

## 3. CMake (certified: CMAKE-MSVC-01)

The cache variable `COLOUR_DEPTH` (default 16, allowed 16 or 32) selects
the renderer for the `sdl2`, `sdl3` and `gdi` backends; any other value
stops the configuration with a message.

MinGW (MSYS2, MSYS Makefiles generator; the Ninja and Visual Studio
generators expect vcpkg on Windows, see `cmake/SimutransVcpkgTriplet.cmake`):

    cmake -S . -B build32 -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release -DSIMUTRANS_BACKEND=gdi -DCOLOUR_DEPTH=32
    cmake --build build32 -j12
    # -> build32/simutrans/simutrans.exe

    cmake -S . -B build16 -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release -DSIMUTRANS_BACKEND=gdi           # 16 by default
    cmake -S . -B build24 ... -DCOLOUR_DEPTH=24     # fails: "COLOUR_DEPTH='24' is not supported ..."

Visual Studio generator + vcpkg (MSVC 2022):

    set VCPKG_ROOT=C:\path\to\vcpkg
    cmake -S . -B build-vs32 -G "Visual Studio 17 2022" -A x64 -DSIMUTRANS_BACKEND=gdi -DCOLOUR_DEPTH=32
    cmake --build build-vs32 --config Release
    # -> build-vs32/simutrans/simutrans.exe ; the generated simutrans.vcxproj lists simgraph32.cc and COLOUR_DEPTH=32

Linux (pkg-config dependencies): `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSIMUTRANS_BACKEND=sdl2 -DCOLOUR_DEPTH=32`.

Note: a tree exported without `.svn` or `.git` has no `revision.h`; add
`-DSIMUTRANS_USE_REVISION=12254` in that case (a checkout does not need it).

## 4. Visual Studio projects, MSBuild (certified: CMAKE-MSVC-01)

The hand-maintained `Simutrans-GDI.vcxproj`, `Simutrans-SDL2.vcxproj` and
`Simutrans-SDL3.vcxproj` take the MSBuild property `COLOUR_DEPTH` (16 when
not given) for the preprocessor definition and the renderer source item;
a target rejects any other value before compiling.

    msbuild Simutrans-GDI.vcxproj /m /p:Configuration=Release /p:Platform=x64                       # 16-bit (default)
    msbuild Simutrans-GDI.vcxproj /m /p:Configuration=Release /p:Platform=x64 /p:COLOUR_DEPTH=32   # TRUE32
    msbuild Simutrans-GDI.vcxproj ... /p:COLOUR_DEPTH=24     # fails: "COLOUR_DEPTH='24' is not supported ..."

Inside the IDE the same property can be set in a `Directory.Build.props`
or on the command line; the projects use the vcpkg manifest as before.
The executable is `<Platform>\<Configuration>\Simutrans GDI Nightly.exe`
(the project's usual name).

## Certified outcomes

- Makefile: all six Windows combinations (GDI/SDL2/SDL3 x 16/32) and Linux
  SDL2-16 / SDL2-32 / SDL3-32 link with 0 errors; warning sets identical to
  the pre-candidate references (RECERTIFICATION-02); re-run unchanged after
  the build-file changes (CMAKE-MSVC-01).
- CMake + MinGW: GDI-16, GDI-32, SDL3-32 link; the renderer object present
  is the selected one only; 24 rejected at configure.
- CMake + Visual Studio generator + vcpkg (MSVC 2022 17.14): GDI-16 and
  GDI-32 configure and link; 24 rejected.
- MSBuild hand projects (MSVC 2022 17.14, x64 Release): 16, 32 and default
  link with 0 errors; 24 rejected; pristine r12254 builds the same way.

## 5. Android (Gradle -> CMake; validated: ANDROID-BUILD-DEPTH-01)

The Android app is built by the separate `simutrans-android-project`
(Gradle): `simutrans/build.gradle` -> `externalNativeBuild` ->
`jni/CMakeLists.txt` -> `add_subdirectory(simutrans)`, i.e. Simutrans'
own root `CMakeLists.txt` with `-DSIMUTRANS_BACKEND=sdl3`. The renderer is
therefore selected by the same `COLOUR_DEPTH` rule as any CMake build:

    default (no argument)         -> COLOUR_DEPTH=16, simgraph16.cc   (unchanged)
    arguments "-DCOLOUR_DEPTH=32" -> COLOUR_DEPTH=32, simgraph32.cc   (opt-in, TRUE32)

added to the `cmake { arguments ... }` list of that project's
`simutrans/build.gradle`. No Simutrans-side change is needed.

`src/android/AndroidBuild.sh` (which writes `COLOUR_DEPTH=16` into a
Makefile config) and `AndroidAppSettings.cfg.in` (`VideoDepthBpp=16`) are
files of the earlier ndk-build / pelya-SDL route; the current Gradle build
does not execute them, so their fixed 16 has no effect on the tested
route. They are left as they are (separate Android maintenance).

Validated (x86_64, NDK 27.0.12077973, SDK platforms 35, Gradle 8.13, JDK
21, `./gradlew assembleDebug`): the 32-bit build compiles and links only
`simgraph32.cc.o` (89 simgraph32 symbols, 0 simgraph16 in the unstripped
library), packages as an APK, installs on an android-35 x86_64 emulator,
shows the pakset chooser, loads pak128, renders the welcome world and a
newly generated 256x256 game; the default build compiles only
`simgraph16.cc.o`. See docs/LIMITATIONS.md for what is not yet proven on
Android.

## Not covered

- `COLOUR_DEPTH=0` (headless server): unchanged, not exercised by this
  work.
- macOS CMake arm: the selection code is generic, but no build was run
  there.

## Runtime

Nothing changes on the command line. Useful for testing:

    sim -use_workdir -singleuser -objects pak128 -lang en -screensize 1024x640 -load mygame.sve
    sim ... -threads 1            # one display lane (the default is min(12, cores))
    sim ... -freeplay             # avoids bankruptcy ending a long test
