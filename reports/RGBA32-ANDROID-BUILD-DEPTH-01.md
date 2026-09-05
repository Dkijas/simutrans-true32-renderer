# STLAB-SIMUTRANS-RGBA32-ANDROID-BUILD-DEPTH-01

## Verdict

    G - DIFFERENT BOUNDARY (with the real boundary answered: B-class result)
    CONFIDENCE: HIGH

The fixed `COLOUR_DEPTH=16` in `src/android/AndroidBuild.sh` is not the
Android renderer selection at all: that script belongs to the old
ndk-build path and is not executed by the current Android build. The
current path is Gradle -> `externalNativeBuild` -> the android project's
`jni/CMakeLists.txt` -> `add_subdirectory(simutrans)` -> Simutrans' own
root `CMakeLists.txt`, i.e. the CMake `COLOUR_DEPTH` rule of the v2
candidate. Through that path Android already selects 16 by default and
32 when the gradle arguments carry `-DCOLOUR_DEPTH=32`; a 32-bit x86_64
APK was built with no Simutrans-side change, runs on the emulator, loads
pak128, renders the welcome world and a newly generated game. So the
apparent hard-code is stale, dead text - not a blocker - and the meaningful
selection input already exists (verdict B's substance), located in the
external android project's `build.gradle`, not in Simutrans SVN.

    RGBA32_ANDROID_BUILD_DEPTH_DEBT = NOT_PRESENT  (the live path is parameterised by v2)
    ANDROID_TRUE32_SUPPORT          = UNVERIFIED   (builds and runs; pixel-exact proof not obtained, see Runtime)

## Base

live HEAD:               r12254 (2026-09-05T00:45:31Z), unchanged during the mission
lane:                    `_stlab-rgba32-android-build-depth-01` (`base/` = r12254 export,
                         `tree/` = base + v2); Android work in a disposable copy of the
                         existing gradle project, `~/android-sdl/simutrans-android-project-true32` (WSL)
TRUE32 candidate identity: 18/18 renderer files raw SHA256 = public package, re-verified
                         inside the android project after injection; 5/5 v2 build files
Android toolchain:       WSL Ubuntu: SDK `/opt/android-sdk-linux` (platforms 35/36,
                         build-tools 35.0.0, NDK 27.0.12077973 used by gradle, 29.0.14206865
                         present, cmake 3.31.6), Gradle 8.13 wrapper, JDK 21; Windows:
                         emulator + AVD `simutrans-phone` (x86_64, android-35, 1080x2400,
                         lcd depth 32), adb 37.0.0 - all from the earlier Android missions

## Android build path

Gradle:            `simutrans/build.gradle`: `externalNativeBuild { cmake { path 'jni/CMakeLists.txt',
                   arguments -DANDROID_APP_PLATFORM=android-24 -DANDROID_STL=c++_shared
                   -DSIMUTRANS_BACKEND=sdl3 -DSIMUTRANS_USE_ZSTD=ON -DSIMUTRANS_USE_FLUIDSYNTH_MIDI=ON
                   -DSIMUTRANS_MULTI_THREAD=ON -DSIMUTRANS_USE_OWN_PAKINSTALL=ON } }`,
                   abiFilters armeabi-v7a/arm64-v8a/x86/x86_64 (the nightly runs
                   `./gradlew assembleRelease` / `bundleRelease`)
AndroidBuild.sh:   NOT INVOKED by that chain (no reference from gradle, `jni/CMakeLists.txt`
                   or `prepareAndBuild.sh`; the earlier Android mission's notes say the same:
                   "the repo keeps AndroidBuild.sh from the old ndk-build path, it does not
                   intervene; patching it has no effect"). Its config (`config.<abi>`,
                   `COLOUR_DEPTH=16`, `BACKEND=sdl2`, `make CFG=<abi>`) is the legacy
                   pelya/ndk-build Makefile route; `AndroidAppSettings.cfg.in`
                   (`VideoDepthBpp=16`, `LibSdlVersion=2.0`) is that legacy port's app
                   config and is not consumed either (the project uses SDL's own
                   android-project Java sources).
Makefile:          not used by the current Android build
renderer selected: by Simutrans' root `CMakeLists.txt` (v2): `simgraph${COLOUR_DEPTH}.cc`
                   in the sdl3 branch, `COLOUR_DEPTH` cache default 16; the Android arm
                   of that branch links `SDL3::SDL3` from the project's SDL3 subdirectory

## Hard-coded 16 origin

classification:

    HISTORICAL_DEFAULT (and now DEAD/UNUSED)

evidence:  introduced whole with the file in r10441 (prissi, 2022-02-02, "More direct
           control on Android builds") together with `OSTYPE=linux`, `BACKEND=sdl2`,
           `USE_SOFTPOINTER=1` etc. - a transcription of the then Makefile defaults for
           the ndk-build path at a time when 16 was the only graphical depth (the
           Makefile defaults to 16 anyway, so the line never changed the outcome).
           Later commits (r10444 reorganisation, r10485, r10655 Play Store, r11182,
           r11811 16 k pages) never touched the depth. The project moved to the
           gradle/CMake chain and left the script behind.

## 32-bit build

attempted:

    YES  (faithful chain: the existing gradle project, `./gradlew assembleDebug`,
          x86_64 only for the emulator, the v2 tree injected as `jni/simutrans`,
          `-DCOLOUR_DEPTH=32` appended to the gradle cmake arguments in the copy)

simgraph32 selected:

    YES  (`.cxx/Debug/5z4i702b/x86_64/simutrans/CMakeFiles/simutrans.dir/src/simutrans/display/simgraph32.cc.o`
          present, no simgraph16 object; unstripped `libsimutrans.so` (x86_64, 73.9 MB):
          89 simgraph32-named symbols, 0 simgraph16-named, `clips32` /
          `recode_img_mutex32` present)

compile:       PASS (BUILD SUCCESSFUL in 1m 1s, NDK 27 clang, arm of the sdl3 branch)
link/package:  PASS (`simutrans-true32-x86_64.apk`, 183 MB, debug-signed, installs on the AVD)

first failure if any:  none. (The first attempt failed in 9 s on the gradle signing
                       config reading unset `SIGNING_*` environment variables - the
                       lab's non-interactive shell had not loaded `~/.profile`; not a
                       build issue.)

Control build (same project, argument removed): BUILD SUCCESSFUL in 46 s,
`simgraph16.cc.o` only, 89 simgraph16-named symbols, 0 simgraph32,
`simutrans-default16-x86_64.apk` 200 MB.

## Android backend

presentation format:  SDL3 window on Android reports `pixel format wanted
                      SDL_PIXELFORMAT_RGBA8888 (1), got SDL_PIXELFORMAT_RGBA8888 (1)`
                      (logcat, both builds); the game draws into its own framebuffer
                      and uploads it as a streaming texture - ARGB8888 at 32, RGB565 at
                      16 (`simsys_s3.cc` lines 610-636, platform-neutral) - which SDL's
                      GLES renderer composes onto that RGBA8888 surface

16-bit assumption:

    NO  (the five `__ANDROID__` blocks of `simsys_s3.cc` are DPI, auto-scale cap,
         window flags and `SDL_main.h`; none touches the pixel format, the pitch or
         the `PIXVAL` width; the candidate did not change any of them; `android.cc`
         / `android.h` (JNI) carry no pixel or buffer type; the SDL2 backend's
         Android blocks are the same kind and equally untouched)

first boundary:  none found in Simutrans' Android-specific code. The legacy
                 `AndroidAppSettings.cfg.in` (`VideoDepthBpp=16`) would have been one
                 for the old pelya SDL port, but it is not consumed by the current
                 build.

## Width contract

STORED:   2  (`static_assert(sizeof(STORED_PIXVAL) == 2)` compiled by the NDK)
SCREEN:   4 at 32 / 2 at 16 (`PIXVAL`; the 32 build compiles `simgraph32.cc`, which
          requires it, and the desktop width guards of CMAKE-MSVC-01 hold for the same
          header)
SAVED:    4  (`static_assert(sizeof(SAVED_PIXVAL) == 4)`)
NETWORK:  2  (`static_assert(sizeof(NETWORK_PIXVAL) == 2)`)

JNI/Android PIXVAL assumption:

    NONE  (`src/android/android.cc` / `.h`: JNI method lookup and the
           `SDL_GetAndroidJNIEnv` shim only)

## Runtime

environment:   Android emulator (x86_64, android-35 google_apis, swiftshader GPU,
               1080x2400, headless), `adb` from Windows; the app driven through
               `adb shell input tap` on the emulator only

startup:       PASS - both APKs start, copy their assets, show the pakset chooser
               (pak / pak.japan / pak128 bundled), stay alive (pid constant over 155 s)
render:        PASS - after choosing pak128 the TRUE32 build shows the welcome screen
               over the intro world (75 k colours in the capture), and after
               "New Game" + "Start Game" a freshly generated 256x256 world (town
               "Bradford", winter night, credits scrolling); the default build shows the
               same screens (464 k colours in its capture: its 16-bit texture is
               scaled with GPU filtering, which invents intermediate colours)

TRUE32 proof:

    NOT_AVAILABLE as a pixel-exact proof.  Screen captures go through SDL's GLES
    renderer, which scales both builds' textures with linear filtering, so the
    RGB565-lattice oracle cannot separate them (32-bit capture: 93.9 % / 99.6 % off
    the game / replication lattices; 16-bit control: 94.2 % / 64.6 %). The in-app
    framebuffer screenshot (the desktop oracle) could not be triggered from adb
    (the `c` key event and text input did not produce a file in the app's
    `files/screenshot`), and the certification hook is not part of the Android
    build. What IS proven on the device: the 32-bit executable is the one running
    (object/symbol identity of the installed APK), it renders correctly, and the
    surface it presents to is RGBA8888.

## Default

Android default remains 16:

    YES  (no `COLOUR_DEPTH` argument in the project's `build.gradle`; the CMake cache
          default is 16; the control build of the unmodified arguments compiled
          `simgraph16.cc.o` only)

## Candidate

required:

    NO  (Simutrans side). The live selection already works through the v2 CMake
        rule; enabling 32 on Android is one gradle argument in the external
        `simutrans-android-project` repository (`"-DCOLOUR_DEPTH=32"`), which this
        mission may not and does not publish. The only Simutrans-side item is the
        stale `src/android/AndroidBuild.sh` + `AndroidAppSettings.cfg.in`: dead
        code that could mislead a reader, not a functional blocker - a cleanup for
        the maintainers, out of this mission's scope and not made here.

files:    0
hunks:    0
+/-:      0

renderer files modified:

    0

backend files modified:

    0

## Debt

RGBA32_ANDROID_BUILD_DEPTH_DEBT:

    NOT_PRESENT

ANDROID_TRUE32_SUPPORT:

    UNVERIFIED  (builds, installs, runs and renders with the 32-bit renderer on the
                 x86_64 emulator; not certified: no pixel-exact framebuffer proof, no
                 ARM device, no performance or stability run, no 16-bit lattice
                 discrimination on the GPU-scaled presentation)

## Effect on next public candidate

v3 unchanged:

    YES

(no Simutrans file changes; the Android build-depth question needs nothing in v3;
the android project argument is documentation for the maintainers, not a patch)

## Recommended next action

    Tell the maintainers, in the review material, that Android selects the
    renderer through the gradle CMake arguments (`-DCOLOUR_DEPTH=32` opt-in,
    default 16), that `src/android/AndroidBuild.sh` and `AndroidAppSettings.cfg.in`
    are the unused ndk-build-era files (their `COLOUR_DEPTH=16` / `VideoDepthBpp=16`
    are dead), and that Android TRUE32 is UNVERIFIED pending a device run with an
    in-app framebuffer capture.

Not executed (no publication in this mission).

## Safety

- fresh isolated lane: yes (`_stlab-rgba32-android-build-depth-01`; the Android
  work in a disposable copy of the gradle project, the original untouched)
- live HEAD checked: yes (r12254)
- default Android remains 16: yes (project arguments unchanged; control build proves it)
- no renderer modifications: yes
- no Android backend implementation: yes
- no SVN commit: yes
- no push: yes (nothing pushed anywhere)
- no release change: yes
- no forum publication: yes
- processes: the emulator was started and stopped by this mission (pid verified
  against the android-tools path); no Simutrans or emulator process left

## Artefacts

`out/simutrans-true32-x86_64.apk`, `out/simutrans-default16-x86_64.apk`,
`gradle-32.log`, `gradle-16.log`, `runs/true32/`, `runs/true32-world/`,
`runs/default16-world/`, `runs/true32-fb/` (screen captures, logcat),
`emu-smoke.ps1`; WSL: `~/android-sdl/simutrans-android-project-true32`
(the disposable project with the v2 tree and the two `.cxx` builds).
