# Certification summary

Authoritative report:
[RGBA32-FULL-RUNTIME-RECERTIFICATION-02](../reports/RGBA32-FULL-RUNTIME-RECERTIFICATION-02.md)
(verdict B - TRUE32 PRODUCT CERTIFIED WITH DOCUMENTED ENVIRONMENT
LIMITATION; `RGBA32_RENDERER_STATUS = PRODUCT_CANDIDATE`). This page
summarises it; the measured facts are in the report and in
[`../evidence/`](../evidence/).

Environment: Windows 11, 16-core machine, one 3440x1440 display, MinGW-w64
g++ 16.1.0 (`-O3`, `MULTI_THREAD`), SDL2 / SDL3 3.4.2 from MSYS2; WSL
Ubuntu g++ 15.2 for the Linux builds. Base trunk r12254.

## Internal identity

    PASS

Every laboratory gate of the development programme was re-run against
freshly built objects of the frozen candidate and came out identical to
its frozen reference: the 29-scenario legacy hash of `simgraph16.cc`
(default and `LOW_LEVEL`, equal to pure trunk), the core blitter probes
(T1-T4), coloured/day-night, zoom preservation, blend/alpha/outline,
remaining image paths, drawing primitives, GUI state, nine-patch, image
dirty contract, backend closure, API residual (90 primitives real, 0
stubs), and threading lane isolation.

## Real runtime

    PASS

Backends: GDI32, SDL2-32, SDL3-32 (all three certified).
Paksets: pak64 (generated 128x128 world), pak128 (a real 192x512 game with
22 cities), pak192.comic (generated world, SDL3).

Runtime features exercised on every backend, live game, 4 display lanes:
startup, world load or generation, scroll, zoom in/out, GUI windows and
text, player colours, day/night levels, transparency (hidden buildings and
trees drawn translucent), real road construction with the two-click way
tool, four map rotations, window resize, screenshot, save and reload
inside the session.

Resize/clip: 68 / 68 clip checks MATCH (45 in dedicated resize sequences
including a 3424x1360 maximise-equivalent geometry, 9 in the scenarios,
14 in the stability run); no crash, no stale clip, no malformed text.

Window/framebuffer: the window contents captured through the compositor
(PrintWindow) are 100 % identical to the framebuffer screenshot on GDI,
SDL2 and SDL3 - the presentation path is 32-bit end to end.

TRUE32 evidence: 99.5-99.9 % of the pixels of real frames carry channel
values that RGB565 cannot represent; the 16-bit control frames have 0.0 %.
(See [`../evidence/true32-precision.png`](../evidence/true32-precision.png).)

Backend consistency: one paused world state rendered through GDI32,
SDL2-32 and SDL3-32 is 100.00 % identical across the three (day, night,
zoomed out, transparent).

## Legacy

    PASS

The 16-bit product built from the candidate tree: renderer hash identical
to trunk, warning sets identical to trunk, runtime smoke (startup, scroll,
zoom, night, resize, screenshot, save, reload) on GDI16, SDL2-16 and
SDL3-16.

## Stability

    PASS

SDL3-32, pak128, `-freeplay`, 1 display lane: 52 cycles of scroll, zoom,
day/night, resize (every third cycle) and save (every fifth), 38.2 minutes
of scripted activity, 0 crashes, 18/18 clip checks, 10 saves, 54
screenshots, memory 964 MB -> 990 MB (peak 998 MB, flat from cycle 5),
planned shutdown reached. A spontaneous clean termination seen once in the
previous cycle did not recur under quit-provenance instrumentation.

## Performance

    classification NORMAL

No blocking regression. On a real-frame benchmark (completed presents of
a full redraw, 1920x1080, one lane) the 32-bit build was not slower than
the trunk 16-bit build in any configuration; details and caveats in
[PERFORMANCE.md](PERFORMANCE.md).

## Memory

About 1.9x the 16-bit working set in the measured workload (pak128:
487 MB -> 920 MB on GDI, 523 MB -> 962 MB on SDL3), explained by the
doubled per-pixel image caches; stable over time, no leak demonstrated.

## Limitation

Linux runtime not certified: the laboratory's WSL FreeType cannot load the
BDF fonts, which blocks the 16-bit runtime in the same way. See
[LIMITATIONS.md](LIMITATIONS.md).

## Build-system integration (CMAKE-MSVC-01, after the renderer certification)

Scope: build files only - `CMakeLists.txt`, the three GUI `.vcxproj` and
the GDI `.filters` (5 files, +60/-20). The 18 renderer files were verified
identical to the certified candidate before and after (raw SHA256), and no
renderer file was touched.

    CMake + MinGW      GDI-16 PASS (simgraph16 only)   GDI-32 PASS (simgraph32 only)   SDL3-32 PASS
                       COLOUR_DEPTH=24: configuration stops with a clear message; default = 16
    CMake + VS 2022    GDI-16 PASS   GDI-32 PASS   24 rejected   (vcpkg x64-windows-static; generated
    generator          simutrans.vcxproj lists simgraph32.cc and COLOUR_DEPTH=32)
    MSBuild hand       16 PASS   32 PASS   default(16) PASS   24 rejected by the new target
    projects           (pristine r12254 builds the same way with the same environment workaround)
    Makefile control   GDI-16 and GDI-32 rebuilt after the change: PASS, same executable sizes as certified

Same renderer through the new path: T1-T4 linked against the CMake gdi-32
objects reproduce the frozen hashes (nc 0xDA18C545, wc 0x19D5EC05,
pc 0x0F526995, class+alpha 0x7B4F5C85); width guards STORED 2 / SCREEN32 4
/ SAVED 4 / WIRE 2, no flag bit inside the colour word (7/7 at 32, 6/6 at
16); the TRUE32 product scenario (scroll, zoom, night, transparency, road
tool, rotation, resize with 3/3 clip MATCH, screenshot, save + reload) on
the CMake-built GDI-32 and SDL3-32 executables with pak128, 99.7 % of
pixels off the RGB565 lattice; the legacy smoke on the CMake-built GDI-16
executable, 0.0 % off-lattice; the MSVC-built executables (hand project
and CMake generator, 32) run the same world with 97.7 % of window pixels
off the RGB565 lattice, the MSVC 16-bit control 0.0 % off GDI's 16-bit
lattice. Warning categories: CMake-16 identical to the unmodified tree;
CMake-32 adds only the categories the certified Makefile-32 build already
had (from the candidate source), none from the selection. `cmake --install`
places the executable and the data directory as before (the install rules
never name a renderer object); its NSIS step fails in this environment for
the unmodified tree and the candidate alike.

## Status matrix

    Windows GDI32              PRODUCT CERTIFIED  (RECERTIFICATION-02)
    Windows SDL2-32            PRODUCT CERTIFIED
    Windows SDL3-32            PRODUCT CERTIFIED
    Build systems (Makefile / CMake / MSBuild)   CERTIFIED (CMAKE-MSVC-01)
    Linux compile (SDL2-16/32, SDL3-32)          PASS
    Linux runtime                                ENVIRONMENT_BLOCKED (fonts)
    Android compile 32 (x86_64)                  PASS      (ANDROID-BUILD-DEPTH-01)
    Android package 32 (APK)                     PASS
    Android emulator run 32                      PASS
    Android internal framebuffer precision       CERTIFIED (emulator, x86_64)  (ANDROID-FRAMEBUFFER-CERT-01)
    Android physical device / ARM                NOT RUN
    GDI resize-event delivery (auxiliary patch)  CERTIFIED FIX (GDI-RESIZE-EVENT-DELIVERY-CERT-01), NOT IN SVN
    GDI DPI WindowSize units (auxiliary patch)   CERTIFIED FIX (GDI-DPI-WINDOWSIZE-CERT-01), NOT IN SVN;
                                                 forum 23805 root cause PROVEN, the topic stays OPEN

## GDI resize-event delivery (GDI-RESIZE-EVENT-DELIVERY-CERT-01, separate from the renderer)

A Windows backend / shared event-queue defect isolated while testing the TRUE32 backends: the
newest client size could be lost (the single GDI pending-event slot overwritten by the mouse move
that follows a size change) or reverted (an obsolete resize stored and re-queued by successive
loading screens applied after the real one). Certified two-file fix
(`patches/gdi-resize-event-delivery-r12254.diff`, `simsys_w.cc` + `simevent.cc`, base r12254):
GDI maximize after loading PASS 3 / 3 (pure trunk FAIL 3 / 3), during loading pak64 and pak128
PASS, resize storm 155 WM_SIZE with the final 1212x818 applied, restore PASS, 369 ordered
applications with 0 backward ones, mutants A and B each reproduce their historical path, SDL2 and
SDL3 PASS, official suite 302 / 302 on base and candidate. Not in SVN; maintainer approval
required. It does not fix the DPI WindowSize unit defect (topic 23805), which stays OPEN. Composition with TRUE32 v2: CLEAN_COMMUTATIVE - the two application orders (v2 then GDI fix, GDI fix then v2) produce byte-identical trees (one order needs a 12-line offset in simsys_w.cc, no fuzz, no rejects); a GDI32 build of the composed tree (COLOUR_DEPTH=32) compiles clean (361 objects, simgraph32 only) and passes one maximize/restore smoke (3440x1369 painted in full, restore to 800x600) and one resize-storm final-state check (final 1212x818 painted in full). Documented for compatibility only; no consolidated candidate is created.
Report: `reports/GDI-RESIZE-EVENT-DELIVERY-CERT-01.md`; evidence: `evidence/gdi-resize-event-delivery/`.

## GDI DPI WindowSize units (GDI-DPI-WINDOWSIZE-CERT-01, separate from the renderer)

The Windows backend keeps the client size in `WindowSize`, whose contract is physical client
pixels: `dr_os_open` writes it that way and `WM_PAINT` consumes it that way, both as the blit
destination and as the divisor that rewrites the DIB height. `dr_textur_resize` wrote the logical
framebuffer size instead. At 100 % the two units coincide numerically; at scaling other than
100 % the next repaint paints a two-thirds rectangle and shrinks the DIB height, after which
partial blits clip their rows - the "content at two thirds, black or stale remainder" of forum
topic 23805. Certified one-hunk fix (`patches/gdi-dpi-windowsize-r12254.diff`, +8/-2 in
`sys/simsys_w.cc`, base r12254), certified **on top of the resize-event delivery fix**, which is
a prerequisite of that configuration:

    contract oracle 150 %   136 / 136 observations LOGICAL without the fix (131 harmful DIB
                            height rewrites) -> 136 / 136 PHYSICAL with it (0 harmful)
    contract oracle 100 %   93 / 93 PHYSICAL in both configurations (control, not proof)
    visible 150 %           maximize FAIL 6 / 6 without the fix (909 of 1369 rows, persistent)
                            -> 0 failures in 3 on-screen runs with it
    negative control        old-unit mutant reproduces mismatch and corruption
    official suite          302 / 302 with and without the fix, identical script output
    SDL2 150 % control      PASS (no WindowSize in SDL2; the defect is GDI-specific)
    composition             TRUE32 v2 + event fix + this patch: clean apply, GDI32 build,
                            maximize / resize-stream / restore smoke PASS

Status: `WINDOWS_GDI_DPI_UNIT_DEBT = CERTIFIED_FIXED at candidate level`,
`FORUM_23805_ROOT_CAUSE = PROVEN`, `FORUM_23805 = OPEN`, `SVN_INTEGRATION = NOT_AUTHORISED`.
Report: `reports/GDI-DPI-WINDOWSIZE-CERT-01.md`; evidence: `evidence/gdi-dpi-windowsize/`.

## Android (ANDROID-BUILD-DEPTH-01)

Build chain Gradle -> CMake -> Simutrans root CMakeLists; `COLOUR_DEPTH`
default 16, opt-in 32 through the project's gradle CMake arguments;
`src/android/AndroidBuild.sh` and `AndroidAppSettings.cfg.in` are not on
that chain. TRUE32 x86_64 build: compile PASS, link PASS, APK PASS,
`simgraph32.cc.o` only, 89 simgraph32 / 0 simgraph16 symbols; default
build: `simgraph16.cc.o` only. Emulator (android-35 x86_64, swiftshader):
startup, pak128 selection, welcome world and a new 256x256 game rendered;
SDL3 presentation surface RGBA8888. Width contracts (STORED 2, SCREEN32 4,
SAVED 4, NETWORK 2) hold under the NDK. Pixel precision of the internal
framebuffer: certified by the next cut, below.

## Android framebuffer (ANDROID-FRAMEBUFFER-CERT-01)

Raw capture of the framebuffer at the SDL3 present boundary (after
`flush_framebuffer()`, before `SDL_UpdateTexture`), written verbatim by a
lab-only hook compiled under `STLAB_CERT_HARNESS`; the same hooked source
built at 16 and at 32 bits, x86_64, run on an android-35 emulator with
the same 18-command scene (new 64x64 map, view at the origin, background
colour RGB(18,52,86) through the system-colour path, paused). Oracle: the
canonical RGB565 round trip with the game's own expansion.

    build    pixels    outside RGB565 grid    RGB(18,52,86) = 0xFF123456    result
    32-bit   540,821   540,263 (99.90 %)      336,294 pixels                TRUE32-PROVEN
    16-bit   540,821   0 (0.00 %)             0 (background is 0x11AA)      CONTROL-OK
    nc32     540,821   0 (0.00 %)             0 pixels                      FAIL (as required)

nc32 is the negative control: the 32-bit build whose capture is forced
through RGB565, which the oracle must and does reject. In both real builds
the derived frame equals the game's own screenshot of the same paused
frame pixel for pixel; across the map area the 16-bit and 32-bit frames
differ by at most the RGB565 quantisation step (8 per channel); the
application kept presenting after the capture. Physical devices and ARM
ABIs: NOT RUN. Report: `reports/RGBA32-ANDROID-FRAMEBUFFER-CERT-01.md`;
evidence: `evidence/android-framebuffer/`.

## Earlier certification and remediation

The first full certification
([RGBA32-FULL-RUNTIME-CERTIFICATION-01](../reports/RGBA32-FULL-RUNTIME-CERTIFICATION-01.md),
verdict C) found three lifecycle defects that the unit gates could not
see: a missing recode lock (startup crash at 12 display lanes), a missing
resize epilogue / clip clamp (crash after shrinking the SDL window), and
the GDI DIB described with 16-bit masks at 32 bpp (pink window). They were
closed in
[RGBA32-RUNTIME-REMEDIATION-01](../reports/RGBA32-RUNTIME-REMEDIATION-01.md)
(verdict B) with before/after evidence and negative controls, and the
recertification above was run on that frozen result without further
change.
