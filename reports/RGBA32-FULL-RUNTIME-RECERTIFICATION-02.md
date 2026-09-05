# STLAB-SIMUTRANS-RGBA32-FULL-RUNTIME-RECERTIFICATION-02

## Verdict

    B - TRUE32 PRODUCT CERTIFIED WITH DOCUMENTED ENVIRONMENT LIMITATION
    CONFIDENCE: HIGH

    RGBA32_RUNTIME_BLOCKERS = CLOSED
    RGBA32_RENDERER_STATUS  = PRODUCT_CANDIDATE

All five decisive product gates PASS on the frozen remediated candidate
(18 files over trunk r12254, which is still HEAD): internal identity
frozen, the three 32-bit backends correct on real pak64 / pak128 /
pak192.comic worlds with the window showing the framebuffer's 32-bit
values pixel for pixel, the 16-bit product identical to pure trunk, a
38-minute SDL3-32 run that reached its planned end with no crash, no clip
mismatch, no rendering failure, no save failure and a flat working set,
and a valid real-frame benchmark on which the 32-bit renderer is not
slower than trunk 16-bit (it is faster on the measured redraw workload)
with the expected ~1.9x pixel-cache memory and no leak.

The one limitation is environmental and unrelated to the renderer: in
this WSL the FreeType font loading fails (`font/cyr.bdf`, pixel size 11
for `Prop-Latin1.bdf`), so every Linux runtime - including the ASan lab
build - stops at the font-selector modal dialogue before the world loop.
Linux is therefore compile-certified only (SDL2-16, SDL2-32, SDL3-32
link clean) and the ASan runtime smoke is ENVIRONMENT_BLOCKED, not PASS.
The previous cycle's spontaneous SYSTEM_QUIT did not recur under
instrumentation that would have named its source (NOT_REPRODUCED), and
the water-image observation did not reproduce either.

No production file was changed in this mission.

## Base

live HEAD:                 r12254 (`svn info` on
                           `svn://servers.simutrans.org/simutrans/trunk` at
                           mission start, 2026-09-05 ~06:20 local)
candidate base:            trunk r12254 (the frozen remediation base)
candidate identity:        the `src/` of `_stlab-rgba32-runtime-remediation-01`
                           copied whole into this lane; `simgraph32.cc` md5
                           504b9409e6, `simsys_w.cc` md5 115b417fed (the
                           values frozen by REMEDIATION-01); every binary
                           hash in `g/provenance.txt`
candidate production files: 18 (12 modified + 6 new vs `head/` = trunk
                           r12254; the build-generated `revision.h` is not
                           counted). Unchanged from REMEDIATION-01.
rebase:                    NOT NEEDED - no trunk movement since r12254
                           (`svn log -r12255:HEAD` empty); classification
                           of trunk movement: none to classify

lane: `<lab>/_stlab-rgba32-recert2` (new, dedicated; sources,
harness and paksets copied from the remediation lane; no build tree or run
dir was inherited - every binary below was built fresh here).

## Remediation status carried in

A1:
CLOSED  (recode_img_mutex32 + re-check under the lock; carried in unchanged;
         no start of any 32-bit build crashed in this mission - 40+ starts
         across the scenarios, 4 and 12 lanes)

A2:
CLOSED  (resize epilogue + clip clamp; re-exercised here: 45/45 clip MATCH
         on the three backends including 3424x1360, see Resize)

B1:
CLOSED  (BI_RGB at 32 bpp; re-proven here: GDI window == framebuffer
         100.00 % on 7 real frames, see Cross-backend consistency)

## Build matrix

Fresh full builds, empty `build/` (`mkbuild.sh`), `mingw32-make -j12
COLOUR_DEPTH=D`, `BACKEND := gdi|sdl2|sdl3`, MULTI_THREAD + OPTIMISE, g++
16.1.0 (MinGW-w64); Linux: WSL Ubuntu g++ 15.2, `make -j12`, SDL2 / SDL3
3.4.2. Warning SETS (file + message, line numbers stripped) compared with the
certification lane's pre-remediation builds and, for GDI-16, with the pure
trunk reference `head-gdi-16`:

| Backend    | 16-bit                                       | 32-bit                                 |
|------------|----------------------------------------------|----------------------------------------|
| GDI        | rc 0, 361 objects, 35 distinct = ref = trunk | rc 0, 361 objects, 38 distinct = ref   |
| SDL2       | rc 0, 361 objects, 15 distinct = ref         | rc 0, 361 objects, 18 distinct = ref   |
| SDL3       | rc 0, 361 objects, 15 distinct = ref         | rc 0, 361 objects, 18 distinct = ref   |
| Linux SDL2 | rc 0, 27 warnings, `sim` 8.70 MB             | rc 0, 30 warnings, `sim` 8.72 MB       |
| Linux SDL3 | not built (not required)                     | rc 0, 30 warnings, `sim` 8.73 MB       |

new 0 / gone 0 in every Windows pair: no candidate-attributable warning.
Lab builds (same source + `-DSTLAB_CERT_HARNESS`): hook-{gdi,sdl2,sdl3}-{16,32}
and the pure-trunk `ref-gdi-16`, all rc 0; Linux hook-sdl2-32 and
hook-sdl2-32-asan (`-fsanitize=address`) rc 0.

## Internal identity

Compact re-run of every previously certified gate against the fresh
`cand-gdi-32` / `cand-gdi-16` objects (`gates.sh recert`, `allgates.sh
recert`, the `mkdual` / `mku8e` probes; frozen references in `g/FROZEN-*`):

inventory:     90 primitives: REAL 90, STUB 0, PARTIAL 0
T1-T4:         PASS (nc 0xDA18C545, wc 0x19D5EC05, pc 0x0F526995,
               class+alpha 0x7B4F5C85, 0 outside writes)
U6-U10:        U6 T5-T9 + PRECISION PASS; U7A Z1-Z4, SENTINEL, SPECIAL,
               LIFECYCLE PASS; U7B ORACLE, B1, B2, R1, O1, O2, PRECISION,
               TINT, ZOOMED PASS; U7C DETECT, E1, DELEGATION, E2, E3 PASS;
               U8A P1-P5 + COVERAGE PASS; U8C GS1-GS5 + LIFETIME PASS;
               U9 probe text IDENTICAL to frozen, B4 PNG decode PASS;
               UR1 (API residual) IDENTICAL to frozen;
               U10 5/5 PASS on 32 and on 16
dirty:         U8E image-dirty probe IDENTICAL to frozen
threading:     U10 lane isolation 5/5 PASS (32 and 16)
zoom:          legacy zoom preservation sha 32d9328552fee8000e75340ef2b905ea
               (58 lines) = every earlier lane
blend:         U7B/U7C as above
text:          text parity 20/21 lines identical (the differing line names
               the renderer)
nine-patch:    hashes 0x3983919D / 0xEEA9F03D / 0x8D4A7225 (4 dirty tiles) /
               0xB64EF2D9 = frozen

INTERNAL_IDENTITY:

    PASS

## Product runtime

One strong scenario per backend (`cert_view32.txt`: startup, close of the
load-report windows, three scrolls, zoom out x2 / in x2, night 3 / 6 / day,
two transparency levels (`hide_buildings` with `hide_with_transparency`),
a road built with the production two-click way tool, four rotations,
resize 800x500 / 1400x900 / 1024x640 with the clip check, screenshot after
every step, PrintWindow captures, save, reload through `karte_t::load`,
memory readings) on real worlds, LIVE, 4 display lanes, `-freeplay`:

pak64:    generated 128x128 world (13 cities) on GDI32 / SDL2-32 / SDL3-32:
          rc 0, 18 shots, road `gavel_road` built ok/ok, 3/3 clip MATCH,
          save + reload ok (128x128, 13 cities)
pak128:   `q25.sve` (192x512, 22 cities, the certification's real game) on
          GDI32 / SDL2-32 / SDL3-32: rc 0, 18 shots, road `Road_050` built
          ok/ok, 3/3 clip MATCH, save + reload ok (192x512, 22 cities)
pak192.comic (optional third pakset, locally available): generated 128x128
          world on SDL3-32: rc 0, 18 shots, road `city_road_fake` ok/ok,
          save + reload ok

GDI32:    PASS (pak64, pak128)
SDL2-32:  PASS (pak64, pak128)
SDL3-32:  PASS (pak64, pak128, pak192.comic)

scroll:        PASS (three `center` moves per scenario, frames complete)
zoom:          PASS (96 <-> 128 <-> 170 raster widths exercised)
GUI/text:      PASS (toolbars, status bar, message windows, load report;
               text parity gate identical)
day/night:     PASS (`night 3`/`6`/`0` measured on the GDI pak128 frames:
               world luminance 116.4 -> 87.9 -> 86.2 -> 92.0; the clock's
               own cycle adds `hours2night[hour]`, see Stability)
player colours: PASS (player-coloured vehicles and stops in the q25 frames
               and in the generated worlds, visually checked on the
               screenshots; the player-colour recode path is the A1 path,
               exercised at 4 and 12 lanes without a crash)
alpha/blend:   PASS (`hide 1`/`hide 2` transparent buildings and trees;
               frames 99.7 % off the RGB565 lattice; blend gates identical)
rotation:      PASS (4 x `rotate90` on every backend, frames complete)
resize:        PASS (see Resize)
screenshot:    PASS (18 per scenario, sizes follow the resize)
save/load:     PASS (save, then `karte_t::load` of that save inside the
               session, world identity confirmed by size and city count)

## TRUE32 proof

framebuffer:            sizeof(PIXVAL)=4, COLOUR_DEPTH=32 logged by the
                        hook in every 32-bit run; the production
                        `take_screenshot` PNGs carry 8-bit channels with
                        values a 565 pipeline cannot emit (below)
backend presentation:   PrintWindow of the process's own window compared
                        with the framebuffer PNG of the same frame:
                        100.00 % identical pixels on GDI32, SDL2-32 and
                        SDL3-32 (4 frames each in the consistency run, plus
                        the scenario captures) - the window shows the very
                        32-bit values, so the presentation path is 32-bit
                        end-to-end on all three backends
non-RGB565-representable pixels (value outside the sets k*255//31,
                        k*255//63 in any channel):
                        pak128 GDI32 99.7 / 99.8 / 99.7 / 99.9 %
                        (scroll, night 6, transparency, 1400x900);
                        pak128 SDL2-32 99.7 / 99.8 %; SDL3-32 99.7 / 99.7 %;
                        pak64 GDI32 99.5 / 98.9 %
                        legacy control (16-bit builds, same frames):
                        GDI16 0.0 / 0.0 %, SDL3-16 0.0 / 0.0 %

TRUE32_END_TO_END:

    PASS

## Cross-backend consistency

One paused deterministic state: `-pause -load q25.sve`, 1 lane, load-report
windows closed, `center 60 40`, wall-clock waits before every capture
(`cert_consistency2.txt`): day, night 5, zoomed out, transparent.

GDI vs SDL2:   framebuffer 100.00 % identical on all 4 frames
GDI vs SDL3:   framebuffer 100.00 % identical on all 4 frames
SDL2 vs SDL3:  framebuffer 100.00 % identical on all 4 frames
presentation:  window vs framebuffer 100.00 % on all 4 frames, all 3
               backends (major GUI/world geometry, player colours and the
               day/night state therefore identical too)

unexplained divergence:

    NONE

(A first pass with iteration-counted waits showed GDI frames 0.2-8 %
identical to SDL's: the GDI capture had been taken before the first world
redraw - the framebuffer still held the "Loading map ..." screen, and the
window showed the same. Harness timing, not renderer semantics; the
wall-clock version above is the evidence.)

## Resize

Production path (`gfx->on_window_resized`), LIVE, 1 lane, pak128, load
report open (GUI text in a window that hangs off a shrunken screen):
1024x640 -> 900x600 -> 1400x900 -> 640x400 -> 1280x800 -> 800x500 ->
1200x750 -> 700x450 -> 1024x640, then the maximise-equivalent geometry
3424x1360 -> 1024x640 -> 640x400 -> 3424x1360 -> 1024x640 (this display is
3440x1440; no GUI control here, so the maximised client extent is sent
through the same path). Plus the 3 resizes inside every product scenario
and 14 inside the stability run.

GDI32:    rc 0, 15/15 MATCH (+3/3 per scenario, no stale clip, no crash)
SDL2-32:  rc 0, 15/15 MATCH (+3/3 per scenario)
SDL3-32:  rc 0, 15/15 MATCH (+3/3 per scenario; +14/14 in the stability run)

clip checks:

    45 / 45 MATCH  (resize sequences)  +  9 / 9 (scenarios)  +  14 / 14 (stability)

No malformed GUI/text on the after-resize screenshots (visually checked on
the 1400x900 and 3424x1360 frames; newly exposed bands carry rendered
content). The separate trunk GDI DPI / window-ordering investigation was
not touched.

## Performance

benchmark methodology: frames are COMPLETED PRESENTS - every backend's
`dr_flush()` increments a counter (lab-only, `#ifdef STLAB_CERT_HARNESS`),
and each present follows one `main_view_t::display()` of the whole visible
world plus the GUI flush (`intr_refresh_display`, simintr.cc). The hook's
`bench MS` records the counter, `dr_time()` and the process CPU time
(`GetProcessTimes`) at the start and at the end of a wall-clock window,
and marks the world dirty on every loop iteration in between, so every
frame is a full redraw of the same scene and a full present. Fixed
deterministic scene: pak128 `q25.sve`, `-pause`, `center 60 40`, the
load-report windows closed, 1920x1080, 1 display lane, warm-up 3 s +
100 iterations, three 20 s windows per run, no load/save inside the
window, no modal menu, no bankruptcy (paused). Same source, same
optimisation (-O3, MULTI_THREAD), same lab flags on the three executables:
`ref-gdi-16` (pure trunk r12254 + hook), `hook-gdi-16` (candidate at 16),
`hook-gdi-32` (candidate at 32).

actual redraw count:
YES

Capped at the product's maximum frame limiter (`fps 100`): all three hold
the cap - 2000 frames per 20 s window in every one of the 15 windows per
build (min 1985), i.e. 10.0 ms/frame = 100 fps for trunk-16, candidate-16
and candidate-32 alike. (CPU time equals wall time here because the paused
loop does not sleep, so the capped CPU figure is not a render cost.)

Uncapped (`env_t::fps = 1000` through the hook, render-bound):

| Depth                | Runs / windows             | Median ms/frame | Median FPS |
|----------------------|----------------------------|-----------------|------------|
| 16 (pure trunk, ref) | 5 / 15 (machine busy*)     | 5.726           | 174.6      |
| 16 (candidate at 16) | 5 / 15 (machine busy*)     | 5.187           | 192.8      |
| 32 (candidate)       | 5 / 15 (machine quieter)   | 2.410           | 415.0      |
| 16 (pure trunk, ref) | 3 / 9, quiet, interleaved  | 4.883           | 204.8      |
| 32 (candidate)       | 3 / 9, quiet, interleaved  | 2.550           | 392.2      |

(* the first series overlapped the 1-lane stability run and two short
scenario runs; dispersion: trunk-16 5.17-6.65 ms, candidate-16 4.66-5.38,
candidate-32 2.26-4.04. The interleaved quiet series is the decisive one:
trunk-16 4.57-5.14 ms, candidate-32 2.14-3.16 ms; CPU-ms/frame equals
ms/frame in both, i.e. the loop is render-bound.)

relative overhead: none measured - the 32-bit renderer redraws this scene
in 0.52x the time of trunk 16-bit (median 2.55 vs 4.88 ms/frame, quiet
interleaved series); the candidate's own 16-bit build is within noise of
pure trunk (5.19 vs 5.73 ms in the busy series, its `simgraph16.cc` being
output-identical to trunk). Noise statement: run-to-run dispersion is
+-10 % on a quiet machine and up to +-30 % with other lab processes
running; the 16-vs-32 gap (about 2x) is far outside it. Not measured:
load time (image recode at load, ~3x in the first certification) and
lane-parallel scaling beyond the checks below.

classification:

    NORMAL

Scaling checks (2 runs x 3 windows each, uncapped, quiet machine):

| Configuration                    | 16-bit ms/frame (median) | 32-bit ms/frame (median) | 32/16 |
|----------------------------------|--------------------------|--------------------------|-------|
| GDI, 1920x1080, 1 lane           | 4.883 (trunk ref)        | 2.550                    | 0.52  |
| GDI, 1024x640, 1 lane            | 1.331 (trunk ref)        | 1.071                    | 0.80  |
| SDL2, 1920x1080, 1 lane          | 3.252 (candidate at 16)  | 2.861                    | 0.88  |
| GDI, 1920x1080, 4 lanes          | 5.671 (trunk ref)        | 5.059                    | 0.89  |

Reading: both depths scale with the pixel count; the large GDI advantage
at 32 comes mostly from presentation - at 32 bpp the DIB is BI_RGB in the
desktop's own format, whereas the 16-bit DIB is converted by GDI on every
present (the SDL2 pair, where both depths upload a texture, differs by
only 12 %). With 4 display lanes the static paused scene is slower than
with 1 (thread barriers on a scene that has nothing to parallelise) for
both depths alike, CPU-ms/frame 6.7 in both. No configuration shows the
32-bit renderer slower than the 16-bit one, and no pathological behaviour
(no super-linear growth with pixels or lanes, no stalls) was observed.

## Memory

Representative steady state: pak128 `q25.sve`, 1 lane, `-freeplay`, LIVE,
1024x640; working set read by the hook (`GetProcessMemoryInfo`) right after
the load, 20 s later, and after a zoom step (`cert_mem.txt`):

| Build          | loaded    | +20 s     | after zoom |
|----------------|-----------|-----------|------------|
| GDI 16-bit     | 487.5 MB  | 487.6 MB  | 491.3 MB   |
| SDL3 16-bit    | 522.7 MB  | 522.9 MB  | 526.5 MB   |
| GDI 32-bit     | 919.6 MB  | 919.6 MB  | 924.7 MB   |
| SDL3 32-bit    | 962.2 MB  | 962.2 MB  | 967.3 MB   |

16-bit:  487-523 MB
32-bit:  920-962 MB

growth:  x1.89 (GDI) / x1.84 (SDL3) - the per-pixel image caches doubled in
         width, as expected; the same +3.7 to +5.1 MB zoom-cache step at
         both depths; the 4-lane product scenarios end 9-10 MB above their
         start after 18 screenshots, a save and a reload (GDI32 919.7 ->
         929.0, SDL2-32 977.4 -> 985.2, SDL3-32 962.4 -> 975.3)

leak/unbounded growth:

    NO

(38-minute stability run, SDL3-32: working set 964 MB at start, 983-990 MB
from cycle 5 on, 990 MB at the end, peak 998 MB; readings every 5 cycles
oscillate within 980-990 MB with no trend - see Stability.)

## Stability

backend:   SDL3-32 (hooked lab build of the frozen candidate), 1 display lane
pakset:    pak128 `q25.sve`
freeplay:
YES

duration:  38.2 min of scripted activity (06:43:35 -> 07:22:02 wall clock
           including load and shutdown), paced by `waitms`
cycles:    52 / 52 (scroll, zoom out, zoom in, night 0/3/6, day; a resize
           every third cycle with a clip check; a save every fifth; a
           screenshot every cycle; a memory reading every fifth)
resizes:   17 (900x600 <-> 1200x800) + 104 zoom changes + 104 day/night
           changes
clip matches:  18 / 18 MATCH, 0 STALE
saves:     10 (`stab_05` .. `stab_50`, all written)
crashes:   0 (exit code 0)
rendering failures:  0 (54 screenshots written at the current size, 0
           FAILED / STALE / unknown-command lines, frames visually complete
           on the sampled cycles)

planned shutdown reached:

    YES  - "mark stability end", final clip MATCH, `s99_end.png`, then the
           harness `quit` line ("harness-initiated: env_t::quit_simutrans =
           true, presents so far 57257"); no QUIT-PROVENANCE line from any
           backend or from `karte_t::stop` in the whole run

day/night drift: none. The frame luminance alternates with the game clock
(`hours2night[hour] + daynight_level`, `simview.cc`): bright frames stay at
121-130 and dark frames at 26-42 across all four quarters of the run (the
per-quarter means of the bright and of the dark frames are flat); no
monotonic trend.

memory runaway: none (964 -> 990 MB, peak 998 MB, flat from cycle 5).

## SYSTEM_QUIT

Instrumentation (lab-only, `apply_recert_instr.py`): every place where an
OS/SDL event becomes SYSTEM_QUIT logs source, event code and timestamp
(`simsys_w` WM_CLOSE / WM_DESTROY, `simsys_s2` SDL_QUIT, `simsys_s3`
SDL_EVENT_QUIT / SDL_EVENT_WINDOW_CLOSE_REQUESTED / SDL_EVENT_TERMINATING
together with the last 8 SDL event types), the application handler that
acts on it (`modal_dialogue`) logs too, `karte_t::stop(exit_game)` logs its
argument, and the harness `quit` logs itself - each line with the present
count, written to the cert log and to stderr. Verified live: the harness
quits show only the harness line (plus `simsys_w WM_DESTROY` during the GDI
teardown after it); on Linux the runner's SIGTERM showed up as
`simsys_s2 SDL_QUIT` at exactly the timeout, i.e. the instrumentation does
see an SDL-generated quit when one happens.

unexpected recurrence:

    NO

origin:

    NOT_REPRODUCED  (the 38-minute run reached its planned end; no
                     SYSTEM_QUIT, window-close or `karte_t::stop` was
                     generated at any point; the only quit is the harness's)

certification impact: none. The REMEDIATION-01 termination at 32 min is
not reproduced with the same backend, pakset, lane count and a longer run;
had it recurred, the log would have named the SDL event and whether a
window-close request preceded it. SYSTEM_QUIT_ANOMALY = NOT_REPRODUCED.

## Sanitizer

ASan runtime:
ENVIRONMENT_BLOCKED

reason: the Linux SDL2-32 lab build with `-fsanitize=address` (rc 0, 25 MB
`sim`) starts, loads pak64 and the save (`karte_t::load(): loaded savegame
from 9/1960`), but never enters `karte_t::interactive()`: this WSL's
FreeType cannot load `font/cyr.bdf` nor set pixel size 11 for
`Prop-Latin1.bdf`, so `simu_main` finds no glyph for the test character
and shows the font-selector `modal_dialogue(..., no_font)` (simmain.cc
~1281), which waits for user input that no harness can give; the run ends
only by the runner's timeout (900 s, then 240 s in the diagnosis run), the
world is destroyed cleanly and 0 AddressSanitizer reports are printed. The
non-ASan Linux lab build behaves identically (same three font warnings,
same stop), so this is the known Linux font environment issue, not
TRUE32-specific and not touched in this mission. Not a PASS.

## Legacy freeze

29-scenario default:    `4a758dc6c6612f7be4d49a46a2825d6dec93a9799ed12884ea864806748a6b39`
29-scenario LOW_LEVEL:  `4a758dc6c6612f7be4d49a46a2825d6dec93a9799ed12884ea864806748a6b39`
(both from the pure-trunk `simgraph16.cc` of the candidate, 220 lines;
equal to the certified hash)

16-bit smoke on pak128 `q25.sve` (LIVE, 4 lanes, `-freeplay`,
`cert_smoke16.txt`: startup, scroll, zoom, night, resize 800x500 + clip,
resize back + clip, screenshot, save, reload):

GDI16:    rc 0, 7 shots, 2/2 clip MATCH, save + reload ok, frames 0.0 % off
          the 565 lattice (the legacy pipeline, as expected)
SDL2-16:  rc 0, 7 shots, 2/2 clip MATCH, save + reload ok
SDL3-16:  rc 0, 7 shots, 2/2 clip MATCH, save + reload ok, 0.0 % off-lattice

The 16-bit objects come from the same 18-file candidate; `simgraph32.cc` is
not compiled at 16 and the 16-bit `simsys_w.o` was shown byte-identical to
trunk in REMEDIATION-01. Warning sets of the three 16-bit builds equal the
references (GDI-16 equal to pure trunk).

LEGACY_STATUS:

    IDENTICAL

## Water-image observation

NOT_REPRODUCED

classification: not a defect. Observed for during the whole mission: the
consistency frames (coast at 60,40, day/night/zoom/transparent), the
scenario frames on three paksets and three backends, the pak192 world, and
the stability screenshots (52 frames over a coast-side viewport at 900x600
/ 1200x800) - no cut image fragment over water in any of them. No dedicated
campaign was run, as instructed.

## Product gates

GATE 1 internal identity:
PASS

GATE 2 product runtime:
PASS

GATE 3 legacy freeze:
PASS

GATE 4 stability:
PASS

GATE 5 performance/resources:
PASS

## Final status

RGBA32_RUNTIME_BLOCKERS:

    CLOSED

RGBA32_RENDERER_STATUS:

    PRODUCT_CANDIDATE

## Remaining known limitations

- ENVIRONMENT_LIMITATION (Linux, this WSL): FreeType cannot load
  `font/cyr.bdf` nor set pixel size 11 for `Prop-Latin1.bdf`, so every
  Linux runtime (16 and 32, plain and ASan) stops at the font-selector
  modal dialogue before `karte_t::interactive()`. Linux is compile-certified
  (SDL2-16, SDL2-32, SDL3-32 link clean, warnings unchanged); the Linux
  runtime and the ASan smoke remain unexecuted here. Not TRUE32-specific.
- Memory: the 32-bit product needs ~1.9x the working set of the 16-bit one
  on pak128 (920-962 MB vs 487-523 MB), stable and explained by the
  per-pixel image caches. A property of the design, not a defect; worth
  stating to the maintainer.
- Not measured in this mission (out of its scope): load time, which the
  first certification saw ~3x longer at 32 because every image is recoded
  at load; the per-frame benchmark here does not cover it.
- Sanitizer coverage of the renderer at runtime: none on Windows (MinGW
  ships no libasan) and blocked on Linux by the font environment.

## Recommended next action

    Prepare the maintainer / SVN integration package for the frozen
    PRODUCT_CANDIDATE (the 18 files of this lane's `src/` over r12254,
    md5 simgraph32.cc 504b9409e6 / simsys_w.cc 115b417fed).

Not executed. No SVN commit, push or publication was made or is
authorised by this report.

## Safety

- isolated recertification lane: `_stlab-rgba32-recert2`, own sources,
  builds, run dirs, WSL trees under `<wsl-home>/recert2`
- exact remediated candidate used: md5-verified, 18 files, no edit
- trunk movement classified: none (HEAD r12254 = base)
- no new production fixes: none; every addition is lab-only under
  `-DSTLAB_CERT_HARNESS` (`apply_cert_hook.py`, `apply_recert_instr.py`:
  present counter, quit provenance, hook commands)
- valid actual-redraw benchmark: frames = completed `dr_flush()` presents,
  counted in the backends, wall clock from `dr_time()`, CPU from
  `GetProcessTimes`
- -freeplay used for long stability: yes
- SYSTEM_QUIT origin observable: yes (backend event -> SYSTEM_QUIT,
  `modal_dialogue` handler, `karte_t::stop`, harness `quit`, each logged
  with timestamp and present count; SDL3 keeps the last 8 event types)
- no STORED32
- no makeobj/pak-format work
- no performance optimization
- no SVN commit
- no push
- no publication
- non-lab processes terminated: 0 (no process was terminated in this
  mission at all; every run ended through its script's `quit` or the
  runner's timeout)

## Provenance

`g/provenance.txt` (candidate md5, every binary's sha256 / size / mtime,
paksets: pak64 810 files 17 MB, pak128 96 files 408 MB, pak192.comic 3669
files 889 MB, `q25.sve` md5 b7b775328f, toolchains). Runs under
`runs/<name>/` (cert.log, sim_stdout.log, exit.rc, PNG/BMP captures, saves);
build logs in `g/builds/<tree>/build.log`; Linux logs in `g/linux/`.
