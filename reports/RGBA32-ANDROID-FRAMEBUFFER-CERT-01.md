# SIMUTRANS-RGBA32-ANDROID-PUBLIC-UPDATE-AND-CERTIFICATION-01

Phase 2 of the brief: certification of TRUE32 pixel precision inside the
Android application's own framebuffer, captured before SDL / GLES
presentation, with the 16-bit build as the decisive control and a
disposable negative control. Phase 1 (publication of the Android build
findings) and Phase 3 (publication of this result) are recorded in the
report's last section.

## Verdict

**A - TRUE32 FRAMEBUFFER CERTIFIED ON THE ANDROID EMULATOR (x86_64).**

    ANDROID_TRUE32_FRAMEBUFFER    = CERTIFIED (emulator, x86_64)
    ANDROID_TRUE32_SUPPORT        = CERTIFIED ON EMULATOR; physical device / ARM NOT RUN
    RGBA32_RENDERER_STATUS        = PRODUCT_CANDIDATE (unchanged)
    Canonical patch               = true32-r12254-v2.diff, UNCHANGED
    Android default depth         = 16, UNCHANGED
    Production change             = NONE

The raw ARGB8888 words the 32-bit build hands to the SDL3 backend at its
present boundary carry colours that cannot exist in RGB565: 99.90 % of the
visible pixels of the certification frame are outside the RGB565 grid, the
decisive vector RGB(18,52,86) = 0xFF123456 is present in 336,294 pixels
exactly, and its RGB565 round trip would have been 0xFF103452. The same
scene rendered by the 16-bit build, captured by the same code at the same
boundary, has 0.00 % of its pixels outside the grid and 0 pixels of the
vector. The negative control (the 32-bit build with its capture forced
through RGB565) is reported below.

## Base and candidate

    Source base       SVN trunk r12254 (export, LF, revision.h = 12254)
    Candidate         TRUE32 v2 = 18 renderer files + 5 build files
                      (patches/true32-r12254-v2.diff of the public repository)
    Lab tree          base + v2 + revision.h            = 24 files differ from base
    Instrumented tree lab tree + 5 instrumented files + stlab_cert_hook.h
                      (simwin.cc, simsys_s2.cc, simsys_s3.cc, simsys_w.cc,
                      simworld.cc), all under #ifdef STLAB_CERT_HARNESS

The candidate is not modified by this cut. The instrumented files exist
only in the laboratory tree; every added line is compiled out unless
`STLAB_CERT_HARNESS` is defined, and the production APK of
ANDROID-BUILD-DEPTH-01 (built without it) is the same source as the
candidate.

## Capture design

Capture point: `dr_flush()` in `simsys_s3.cc`, immediately after
`gfx->flush_framebuffer()` and before `SDL_UpdateTexture` /
`SDL_RenderTexture` / `SDL_RenderPresent`. That is the last moment the
words are the renderer's own; everything after it belongs to SDL and to the
Android GLES presentation path, which the previous cut showed to filter and
scale the texture of both builds alike.

Format: a 32-byte header (magic `SLAB`, COLOUR_DEPTH, screen width and
height, framebuffer pitch, framebuffer height, sizeof(PIXVAL), present
counter) followed by `pitch * height` words of `sizeof(PIXVAL)` bytes,
written with a single `fwrite` of the framebuffer - no conversion, no
copy through any image library.

Trigger: the hook script command `fbdump NAME` sets a request that the
next present fulfils. The hook script lives in the app's external files
directory and is read at startup; without it the instrumented build
behaves as the production one.

Oracle (`fb_analysis.py`): each word is converted to RGB888 (32: the
byte lanes; 16: the game's own expansion `(v*0xFF)/0x1F`, `(v*0xFF)/0x3F`),
encoded to RGB565 as `r>>3, g>>2, b>>3` and expanded back with the same
formulas. A pixel is "RGB565-representable" when the round trip returns it
unchanged. The RESULT line is CONTROL-OK only for depth 16 with zero
non-representable pixels, TRUE32-PROVEN only for depth 32 with more than
5 % non-representable pixels, FAIL otherwise.

Negative control: the same 32-bit source built with `-DSTLAB_NC_QUANTISE`,
where the capture pushes every word through RGB565 (with the game's
expansion) before writing it. If the oracle is honest it must report FAIL
for that build although its renderer, framebuffer and scene are identical
to the certified one.

## Builds (three, one hooked source, x86_64)

    NDK 27.0.12077973, SDK platform 35, Gradle 8.13, JDK 21,
    `./gradlew assembleDebug`, abiFilters x86_64, backend sdl3,
    Gradle CMake arguments of the project + the lab arguments below.

    arm     CMake arguments                                          simgraph32  simgraph16  stlab_ symbols
    hook32  -DCOLOUR_DEPTH=32 -DCMAKE_CXX_FLAGS=-DSTLAB_CERT_HARNESS         89           0     16
    hook16  -DCOLOUR_DEPTH=16 -DCMAKE_CXX_FLAGS=-DSTLAB_CERT_HARNESS          0          89     16
    nc32    -DCOLOUR_DEPTH=32 -DCMAKE_CXX_FLAGS="-DSTLAB_CERT_HARNESS
            -DSTLAB_NC_QUANTISE"                                             89           0     16

Symbol counts are from the unstripped `libsimutrans.so` of each CMake
configuration directory (`llvm-nm -C --defined-only`). Identity chain from
configuration to APK: the GNU build-id of the library inside each APK
equals the build-id of the unstripped library of the configuration whose
CMakeCache carries the arguments above.

    arm     build-id (first 20 hex)   library in APK (bytes)   CMakeCache COLOUR_DEPTH / CXX_FLAGS
    hook32  1ca23e63f323f8b34df7      16,556,744               32 / -DSTLAB_CERT_HARNESS
    hook16  21e606e96c5654136b28      16,549,320               16 / -DSTLAB_CERT_HARNESS
    nc32    57670457bc54aa469bf7      16,556,984               32 / -DSTLAB_CERT_HARNESS -DSTLAB_NC_QUANTISE

APK SHA256 values are in `evidence/android-framebuffer/hashes.txt`. The
APKs are not redistributed.

## Runtime environment

    Emulator      Android SDK emulator, AVD android-35 x86_64 (Google APIs),
                  1080x2400, `-gpu swiftshader_indirect -no-window -no-audio
                  -no-snapshot`, device emulator-5554
    App surface   493x1097 logical pixels (the game's window; density scaled),
                  framebuffer pitch 496
    Window        SDL/APP log: pixel format wanted SDL_PIXELFORMAT_RGBA8888,
                  got SDL_PIXELFORMAT_RGBA8888 - in both builds (the Android
                  window surface, independent of COLOUR_DEPTH)
    Pakset        pak128 (the project's bundled pakset), selected by tap
    Scene         hook script: wait 60 frames, close all windows, new map
                  64x64 (map number 33 of the default settings, 2 cities),
                  view at (0,0), daynight level 0, background colour set to
                  RGB(18,52,86) through the system-colour path, pause,
                  fbdump, then the game's own screenshot of the same frame

The scene is driven only by the hook script placed in the app's external
files directory and by three `adb input tap` events (pakset button, New
Game, Start Game), identical for the three runs.

## Results

### hook32 - the certified build

    file frame1.fb  sha256 3c98ede72489370e...
    COLOUR_DEPTH=32  screen 493x1097  pitch 496  fb_height 1097  sizeof(PIXVAL)=4
    captured at present #2846
    pixels analysed 540,821
      RGB565-representable          558   (0.10 %)
      NOT representable         540,263   (99.90 %)
      distinct non-representable colours 1,047
    decisive vector RGB(18,52,86) = 0xFF123456: 336,294 pixels;
      its RGB565 round-trip would be 0xFF103452
    examples  0xFF72B2DD -> 0xFF73B2DE   0xFF355367 -> 0xFF315062
              0xFFC0C0C0 -> 0xFFC5C2C5   0xFFF8F8F8 -> 0xFFFFFAFF
              0xFF4E7A97 -> 0xFF4A7994   0xFF233745 -> 0xFF203441
    RESULT: TRUE32-PROVEN

Alpha / special sanity: 540,524 of the 540,821 visible words have alpha
0xFF; the remaining 297 are 0x00000000, the black text of the date bar
(rows 2-16), so black reaches the framebuffer with an empty alpha lane
through the text colour path while system colours carry 0xFF (see
`get_system_color`). All three backends ignore the alpha lane (ARGB8888
texture without blending, BI_RGB DIB), so this has no visible effect;
the 16-bit build stores the same pixels as 0x0000. Recorded as an
observation, not a gate; no change proposed. The 3 padding columns beyond
the visible width are not part of the analysis. The most frequent words
after the background
are the night-shaded water colours (0xFF161931, 0xFF14162D, 0xFF111429,
...), i.e. the colour-multiplied special path produces full 8-bit
results. The derived PNG of the dump and the game's own screenshot of the
same paused frame (`take_screenshot`, the production path) are
pixel-identical (difference bounding box: none).

### hook16 - the decisive control

    file frame1.fb  sha256 184101297e056736...
    COLOUR_DEPTH=16  screen 493x1097  pitch 496  fb_height 1097  sizeof(PIXVAL)=2
    captured at present #2872
    pixels analysed 540,821
      RGB565-representable      540,821   (100.00 %)
      NOT representable               0   (0.00 %)
      distinct non-representable colours 0
    decisive vector RGB(18,52,86): 0 pixels; the background is the RGB565
      word 0x11AA (= RGB 16,52,82 after expansion) in 336,294 pixels
    RESULT: CONTROL-OK

The 16-bit derived PNG and the 16-bit game screenshot are pixel-identical
as well. The two builds rendered the same scene: the same 336,294
background pixels, and across the whole map area (rows 22-1049) the
per-channel difference between the 16-bit expansion and the 32-bit words
never exceeds 8, the RGB565 quantisation step. The only larger
differences are in the bottom toolbar, whose icon layout differs by one
row between the two builds (a GUI layout matter, not a pixel-precision
one).

### nc32 - negative control

    file frame1.fb  sha256 e90e4a1f51dbf234...
    COLOUR_DEPTH=32  screen 493x1097  pitch 496  fb_height 1097  sizeof(PIXVAL)=4
    captured at present #2867
    pixels analysed 540,821
      RGB565-representable      540,821   (100.00 %)
      NOT representable               0   (0.00 %)
      distinct non-representable colours 0
    decisive vector RGB(18,52,86) = 0xFF123456: 0 pixels
    RESULT: FAIL (unexpected population for this depth)

The same renderer, framebuffer, scene and capture code as hook32, with the
words pushed through RGB565 on the way out: the oracle reports FAIL. It
therefore cannot be satisfied by a 32-bit build whose colours are
RGB565-quantised, and the PASS of hook32 is not a property of the depth
field or of the analysis but of the words themselves.

    CONTROL (16-bit build)          -> CONTROL-OK (0.00 % outside)
    NEGATIVE CONTROL (quantised 32) -> FAIL
    CANDIDATE (32-bit build)        -> TRUE32-PROVEN (99.90 % outside)

### Presentation continues

In all runs the application was alive 10 s after the capture (same pid),
the emulator screen capture taken after the dump shows the scene being
presented, and logcat has no FATAL / SIGSEGV entry. The present counter
advanced by exactly one between the request and the dump (the world was
paused, so only the requested redraw was presented), which also shows the
capture did not stall the loop.

### Instrumented versus normal behaviour

The instrumented builds start, select the pakset, show the welcome world
and enter a new game through the same sequence as the production APK of
ANDROID-BUILD-DEPTH-01, with the same `pixel format ... RGBA8888` log line;
the hook only acts when its script file exists. No perturbation of the
renderer is possible by construction: the capture reads the framebuffer
after the renderer has finished the frame and writes it to a file; it
does not write to the framebuffer or to any renderer state.

## What this proves and what it does not

Proved: on Android (x86_64, emulator), the TRUE32 build's framebuffer is a
genuine ARGB8888 image whose values are not RGB565-quantised, at the last
point before SDL takes it; the 16-bit build captured identically is fully
RGB565; the oracle rejects a quantised 32-bit capture.

Not proved and not claimed:

- Physical devices: NOT RUN.
- ARM ABIs (armeabi-v7a, arm64-v8a): NOT RUN (x86_64 only).
- What the display finally shows after SDL's GLES texture upload and the
  compositor - the previous cut showed the emulator screen is filtered and
  cannot serve as an oracle; a physical device with a lossless capture
  would be needed.
- Performance and battery at 32 bits on Android: not measured.

## Safety

    SVN commit / push / PR / forum post     none (the forum text below is a draft)
    Production files changed                0
    Android default depth                   16, unchanged
    Release / signing / versioning          unchanged; the debug signing of the
                                            project was used as is; no keystore
                                            value appears in any artefact
    Frozen renderer files                   untouched (byte-identical to v2)
    Negative-control build                  disposable copy of the Android project
                                            only; the certification tree carries no
                                            quantisation code
    Processes                               emulator started and stopped by this
                                            mission (its own PID); no name-wide kill

## Artefacts (lane)

    android-builds.sh          the three builds
    apply_android_hook.py      hook additions for Android (script path, fbdump, pause)
    apply_fbdump.py            the capture at dr_flush()
    fb_analysis.py             the oracle
    stlab_cert.txt             the scene script (18 commands)
    emu-fb.ps1                 install, drive, capture, pull, verify still running
    out/simutrans-{hook32,hook16,nc32}-x86_64.apk
    runs/fb-hook32/, runs/fb-hook16/, runs/fb-nc32/
        frame1.fb (raw dump), frame1.png (derived), fb_shot.png (game screenshot),
        stlab_cert.log, logcat.txt, chooser/menu/after-capture/still-running screens
    gradle-{hook32,hook16,nc32}.log

Published in the public review repository (Phase 3):
`reports/RGBA32-ANDROID-FRAMEBUFFER-CERT-01.md` (this file, sanitised),
`evidence/android-framebuffer/` (statistics, hashes, the two derived
frames and the RGB565-grid masks); the raw dumps and the APKs are not
published.

## Phases 1 and 3 (publication)

Phase 1: the Android build-path findings of ANDROID-BUILD-DEPTH-01 were
published as commit `3304f00` ("docs: document Android TRUE32 build
validation"): README status block, docs/BUILDING.md section 5,
docs/CERTIFICATION.md status matrix and Android section,
docs/LIMITATIONS.md Android section, and the sanitised report.

Phase 3: this certification is published as the commit named in the final
report; the status lines change from `UNVERIFIED` to `CERTIFIED
(emulator, x86_64)`; physical device / ARM stay `NOT RUN`. No new patch
version: the renderer candidate and the canonical patch are unchanged.
