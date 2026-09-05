# STLAB-SIMUTRANS-RGBA32-RUNTIME-REMEDIATION-01

## Verdict

    B - REMEDIATED WITH BOUNDED HISTORICAL ADAPTATION
    CONFIDENCE: HIGH

    RGBA32_RUNTIME_BLOCKERS = CLOSED
    RGBA32_RENDERER_STATUS  = RECERTIFICATION_REQUIRED

All three blockers are closed on the final objects, each with a
before/after pair on the real executable:

    A1  recode_img race       0 crashes in 36 starts at 12 lanes (GDI 12,
                              SDL2 12, SDL3 6, pak128; +6 pak64); the same
                              objects with only the lock removed crash 12/12
    A2  resize epilogue       no SIGSEGV, clip == framebuffer at 15 of 15
                              checkpoints per backend on SDL2/SDL3/GDI, the
                              historical shrink reproducer FAIL -> PASS
    B1  GDI 32-bpp DIB        window == framebuffer at 100.00 % (8 captures),
                              three exact colour vectors present pixel-exact
                              in the window, 0 pixels at the RGB565 rounding

Why B and not A: A2 needed one adaptation beyond the literal epilogue. With
the epilogue alone (clip reset + full dirty after a real resize) SDL2 and
SDL3 still crashed at the first shrink, because the transposed
`simgraph32_set_clip_rect` stored whatever rectangle the GUI asked for
without clamping it to the framebuffer, while `simgraph16_set_clip_rect`
clamps (to the screen, or to the current clip when `fit` is set). A window
hanging off the shrunken screen re-pushed a clip beyond the exact-size SDL
buffer one frame after the epilogue had reset it. The clamp was transposed
from the legacy function; it is in the same file, it is the clip half of
the same contract the brief names ("clip extent = new drawable framebuffer
extent"), and it is explained in full below. No fourth production area was
touched. Nothing else changed: 16-bit objects byte-identical, legacy hash
identical, every TRUE32 identity gate identical, warning sets identical.

One observation is recorded and NOT classified as a defect: the user saw
"cut images over water" once on the welcome-screen background world during
the A1 stress (pre-clamp binary, 12 lanes). It did not reproduce in 24
external captures of that same screen (12 on the pre-clamp binary, 12 on
the final one) nor in 16 hook captures of loaded and freshly generated
worlds at 12 and 1 lanes, where window and framebuffer were identical.
See "Remaining known defects".

## Base

live HEAD:               r12254 (`svn info` on
                         `svn://servers.simutrans.org/simutrans/trunk` at
                         report time, 2026-09-05 ~05:35 local; last changed
                         2026-09-05T00:45:31Z) - equal to the candidate
                         base, no drift
candidate base:          trunk r12254 + the RGBA32 candidate exactly as
                         certified in FULL-RUNTIME-CERTIFICATION-01
                         (`_stlab-rgba32-cert1`), copied whole
lane:                    `<lab>/_stlab-rgba32-runtime-remediation-01`
                         (dedicated; its own src/, head/, paks/, build trees,
                         run dirs, logs; nothing outside it was written except
                         the note in "Harness slips")
candidate files before:  12 modified + 6 new vs trunk r12254 (18 files;
                         `simsys_w.cc` was already modified - PIXVAL
                         typing and a depth-aware `biBitCount` - with the
                         RGB565 masks left in place)
candidate files after:   the same 18 files (12 modified + 6 new, listed in
                         `reports/candidate-files-after.txt`; the
                         build-generated `revision.h` is not counted). No
                         file joins or leaves the set: `simsys_w.cc` gains
                         one hunk (+10/-1), `simgraph32.cc` (a new file of
                         the candidate) gains 7 hunks (+53/-5). Unified
                         diffs against the pre-remediation copies:
                         `reports/remediation-simgraph32.diff`,
                         `reports/remediation-simsys_w.diff`;
                         the applier is `apply_remediation.py` (exact-string
                         edits with count assertions, run on the pristine
                         copies `g/simgraph32.pristine.cc`,
                         `g/simsys_w.pristine.cc`).

The three defects were frozen first: the pristine copies were re-run
through the certification reproducers before any edit (A1 stress crashing,
A2 shrink crashing on SDL, B1 pink window), so every PASS below has a FAIL
on the same harness.

## Blocker A1 - recode_img

root cause:  U6 dropped the legacy `recode_img_mutex` and the re-check of
             `player_flags` under it. Two display lanes recoding the same
             image at once: one frees/reallocates the per-player buffer
             while the other blits from it (backtrace in the certification:
             `colorpixcopy_screen32` / `blit_core_clipped` on a freed
             buffer).
fix:         exactly the legacy contract transposed from
             `simgraph16::recode_img`: the caller's unlocked fast check is
             unchanged; `recode_img` takes `recode_img_mutex32`, re-checks
             `player_flags` under the lock and returns if another lane
             already recoded, otherwise recodes once and clears the flag
             before unlocking. `#ifdef MULTI_THREAD` like the legacy.
files/functions:
             `src/simutrans/display/simgraph32.cc`:
             `recode_img_mutex32` (new static, next to `rezoom_img_mutex32`),
             `recode_img(image_id, sint8)` (lock, re-check, unlock).

stress (plain builds, DEFAULT lane count = min(MAX_THREADS 12, 16 cores)
= 12, no `-threads`, each start killed at 25 s if still alive, a crash is
rc 139; `stress_a1.sh`):

    GDI:   cand-gdi-32  pak128 12 starts: crashes 0, alive at 25 s 12
    SDL2:  cand-sdl2-32 pak128 12 starts: crashes 0, alive at 25 s 12
    SDL3:  cand-sdl3-32 pak128  6 starts: crashes 0, alive at 25 s 6
    pak64 control: cand-gdi-32  6 starts: crashes 0

crashes before:  certification: 8/18 (GDI+SDL2 at 12 lanes), pak128 4/4;
                 this lane, pristine objects re-run before the edit: same
                 behaviour (negative control below is the frozen form)
crashes after:   0/36 pak128, 0/6 pak64 (final objects, 04:44-04:45 builds)

negative control (disposable trees `nc-gdi-32`, `nc-sdl2-32`: the FINAL
remediated source with only the A1 lock and re-check removed - the
`diff` against the candidate is exactly those 8 lines - everything else
identical, same flags):

    nc-gdi-32  pak128 6 starts: crashes 6/6
    nc-sdl2-32 pak128 6 starts: crashes 6/6

    PASS  (control FAILS 12/12, candidate PASSES 36/36)

A1:

    CLOSED

## Blocker A2 - resize

root cause:  `simgraph32_on_window_resized` only called `dr_textur_resize`
             and `dirty_state_alloc`; the legacy epilogue
             (`set_clip_rect(0,0,w,h)` + `mark_screen_dirty`, plus the
             min-16 / height-64 guards and "resize only if changed") was not
             transposed. After a shrink lane 0 kept clipping to the old
             1024x640 and window text wrote past SDL's exact-size buffer.
             Second half, found when the epilogue alone still crashed:
             `simgraph32_set_clip_rect` ignored its `fit` flag and stored
             the rectangle unclamped, so the GUI's own clip for a window
             hanging off the shrunken screen re-pushed the clip past the
             framebuffer on the very next frame.
fix:         (1) the legacy epilogue transposed from
             `simgraph16_on_window_resized`: `disp_actual_width = max(16,w)`,
             height <= 0 -> 64, resize only when the extent changed, and on
             a real change `dirty_state_alloc()` +
             `simgraph32_set_clip_rect(0,0,disp_actual_width,disp_height
             CLIP_NUM_DEFAULT,false)`, then `simgraph32_mark_screen_dirty()`
             and `dirty_old_clear()` (the previous-frame map forgotten, as
             the legacy does inline).
             (2) the legacy clamp transposed into
             `simgraph32_set_clip_rect(..., bool fit)`: `!fit` clamps to
             `0..disp_width / 0..disp_height`, `fit` clamps to the current
             clip (`clip_wh32`, the function the file already had).
             Nothing was fixed by enlarging buffers, padding, disabling
             clipping, suppressing text or special-casing SDL.
files/functions:
             `src/simutrans/display/simgraph32.cc`:
             `simgraph32_on_window_resized`, `simgraph32_set_clip_rect`,
             `dirty_old_clear` (new 5-line static helper + forward decl).

Production path used: the hook's `resize W H` calls
`gfx->on_window_resized(scr_size)` - the same entry the backends call
from their window-size events - followed by `viewport->metrics_updated()`,
with the world LIVE, pak128 `q25.sve` (missing-object message window and
the "Game successfully loaded" window open, i.e. GUI text in a window that
hangs off the screen after a shrink), `-threads 1`.

Sequence `cert_resize_clip.txt`: 1024x640 -> 900x600 -> 1400x900 ->
640x400 -> 1280x800 -> 800x500 -> 1200x750 -> 700x450 -> 1024x640, a
`clip` check and (mostly) a screenshot after each step. Sequence
`cert_resize_max.txt` (maximise/restore emulated; there is no GUI control
here so the maximised client extent 3424x1360 of this 3440x1440 display is
sent through the same path): 1024x640 -> 3424x1360 -> 1024x640 -> 640x400
-> 3424x1360 -> 1024x640.

SDL2 shrink:   rc 0, no SIGSEGV; 9/9 + 6/6 clip checks MATCH
SDL3 shrink:   rc 0, no SIGSEGV; 9/9 + 6/6 clip checks MATCH
GDI resize:    rc 0; 9/9 + 6/6 clip checks MATCH (same logical
               postconditions, although its maximum-size DIB had hidden the
               historical failure)

clip after resize:   lane 0 `get_clip_rect` == (0,0,screen w,screen h) at
                     every one of the 45 checkpoints (15 per backend); the
                     hook logs the four numbers and the screen size, e.g.
                     `clip lane0 x 0..640 y 0..400  screen 640x400  MATCH`
dirty after resize:  the full redraw is visible in the screenshots taken
                     after each enlarge: the newly exposed right and bottom
                     bands carry rendered content on all three backends
                     (1400x900 after 900x600: right band 3880-3938 colours,
                     0.4 % black; bottom band ~4000 colours, 0.6 % black;
                     3424x1360 after 1024x640: right band 7594-8888
                     colours, 0.2 % black; bottom band 1633-2467 colours,
                     0.1 % black - the black is the toolbar/status-bar
                     furniture, identical across backends)

historical reproducer (`cert_resize.txt`, the certification's shrink
script, 1 lane, pak128 live):

    epilogue only (this lane, first build):   SDL2 rc 139, SDL3 rc 139 at
                                              the first shrink; GDI rc 0
    epilogue + clamp (final objects):         SDL2 rc 0, SDL3 rc 0, GDI rc 0
    certification objects (cert1 `rsz-sdl2-32`, `rsz-sdl3-32`,
    `rszp-sdl3-32`): rc 139

    FAIL before / PASS after

A2:

    CLOSED

## Blocker B1 - GDI presentation

root cause:  `simsys_w.cc` described the 32-bpp DIB with `BI_BITFIELDS`
             and the RGB565 masks 0xF800/0x07E0/0x001F (identical to
             trunk, which never runs at 32): GDI read 16-bit masks out of
             32-bit words - pink window over a correct framebuffer.
fix:         depth-aware header: at `COLOUR_DEPTH == 32` the header is
             `BI_RGB` with the three masks zeroed (BI_RGB at 32 bpp is
             exactly the 0xAARRGGBB word the framebuffer holds); the RGB555
             and RGB565 branches are verbatim under `#elif`. `simsys_w.o`
             at 16 is byte-identical to the pristine object (md5
             7976e99f271d, both builds).
bitmap format before:  biBitCount 32, biCompression BI_BITFIELDS, masks
                       0x0000F800 / 0x000007E0 / 0x0000001F
bitmap format after:   biBitCount 32, biCompression BI_RGB, masks 0/0/0
                       (16-bit: unchanged BI_BITFIELDS 565 / 555)

exact vectors (hooked GDI-32, production path: `newmap 64 64`, `center 0 0`
so the background above the map corner is visible, `bg R G B` through
`env_t_rgb_to_system_colors`, then `shot` = production `take_screenshot`
of the framebuffer and `wsnap` = `PrintWindow` of the process's own
top-level window captured by the compositor, independent of framebuffer
memory; `cert_b1.txt`, at 1 lane and at 12 lanes):

    RGB(18,52,86)   -> FF123456: framebuffer 26433 px, window 26433 px,
                       RGB565-rounded (16,52,82) in window: 0 px
    RGB(201,77,13)  -> FFC94D0D: framebuffer 26433 px, window 26433 px,
                       565-rounded (205,76,8): 0 px
    RGB(3,250,129)  -> FF03FA81: framebuffer 26433 px, window 26433 px,
                       565-rounded (0,250,131): 0 px
    (standalone control from the certification, `dibproof.cc`: the old
    header turns RGB(18,52,86) into (49,138,181) and red into black; BI_RGB
    is exact)

real frame:   pak128 `q25.sve` world frames and a freshly generated 256x256
              world, at 12 lanes and at 1 lane: window vs framebuffer
              100.0 % identical pixels in all 8 captures (`cut-gdi32-*`),
              and 100.00 % in the 8 `b1-gdi32-*` captures above
visible presentation:  the welcome-screen world of the PLAIN final GDI-32
              build, captured from a separate process (`wincap.exe`,
              PrintWindow by PID; 3 + 12 captures): terrain, water, city,
              vehicles, GUI text and the scrolling credits all in their
              correct colours, 6573-9204 distinct colours per frame; the
              user's own screenshot of the remediated GDI-32 build showed
              the correct colours as well

B1:

    CLOSED

## Windows build matrix

Fresh, non-incremental, `mingw32-make -j12 COLOUR_DEPTH=D` with
`BACKEND := gdi|sdl2|sdl3` in `config.default`, MULTI_THREAD, OPTIMISE,
g++ 16.1.0 (MinGW-w64): all six compile and link (rc 0, 361 objects
each, 56-57 s). Warning SETS (file + message, line numbers stripped)
compared against the certification lane's pre-remediation builds and, for
GDI-16, against the trunk reference `head-gdi-16`:

| Backend | 16-bit                            | 32-bit                   |
|---------|-----------------------------------|--------------------------|
| GDI     | rc 0; 35 distinct = cand, = trunk | rc 0; 38 distinct = cand |
| SDL2    | rc 0; 15 distinct = cand          | rc 0; 18 distinct = cand |
| SDL3    | rc 0; 15 distinct = cand          | rc 0; 18 distinct = cand |

new 0 / gone 0 in every pair. Incremental trees of the same source
(`cand-*`, `hook-*`) were used for the runtime gates; their
`simgraph32.cc`/`simsys_w.cc` md5 equal the lane's (504b9409e6 /
115b417fed) and the gate copy `cand32/build/default` holds the same
`simgraph32.o` as `cand-gdi-32` (c970ccfa0d).

## Legacy freeze

29-scenario default:    `4a758dc6c6612f7be4d49a46a2825d6dec93a9799ed12884ea864806748a6b39`
29-scenario LOW_LEVEL:  `4a758dc6c6612f7be4d49a46a2825d6dec93a9799ed12884ea864806748a6b39`
(both 220 lines, `gates.sh final`; the certified hash is the same string,
which is the hash of PURE trunk `simgraph16.cc`)

Also: the 16-bit `simsys_w.o` is byte-identical to pristine; the 16-bit
objects were not rebuilt from any changed source (`simgraph32.cc` is not
compiled at 16, `simsys_w.cc` preprocesses to the same text).

legacy status:

    IDENTICAL

## TRUE32 regression gates

All on the final objects (`allgates.sh final`, `gates.sh final`, the
`mkdual`/`mku8e` probes; frozen references from the earlier cuts in
`g/FROZEN-*.txt`):

inventory:        90 primitives: REAL 90, STUB 0, PARTIAL 0
core:             T1-T4 PASS (nc 0xDA18C545, wc 0x19D5EC05, pc 0x0F526995,
                  class+alpha 0x7B4F5C85; 0 outside writes)
colored/daytime:  U6 T5-T9 + PRECISION PASS
zoom:             U7A Z1-Z4, SENTINEL, SPECIAL, LIFECYCLE PASS; legacy
                  zoom preservation sha 32d9328552fee8000e75340ef2b905ea
                  (58 lines) = every earlier lane
blend/alpha:      U7B ORACLE, B1, B2, R1, O1, O2, PRECISION, TINT, ZOOMED
                  PASS; U7C DETECT, E1, DELEGATION, E2, E3 PASS
dirty:            U8E image-dirty probe IDENTICAL to frozen; nine-patch
                  hashes 0x3983919D / 0xEEA9F03D / 0x8D4A7225 (4 dirty
                  tiles) / 0xB64EF2D9 = frozen U8E set
threading:        U10 5/5 PASS on 32 and on 16; U8C GS1-GS5 + LIFETIME
                  PASS; U8A P1-P5 + COVERAGE PASS
backend/API:      U9 probe text IDENTICAL to frozen, B4 PNG decode PASS;
                  API-RESIDUAL (UR1) IDENTICAL to frozen; text parity 20/21
                  lines identical (the differing line names the renderer)

## Product smoke

Hooked builds, pak128 `q25.sve`, `-threads 4`, LIVE, `cert_view.txt`
(startup, three scrolls, zoom out x2 / in x2, day/night 3 and 6 and back,
four rotations, resize 800x500 / 1400x900 / 1024x640, screenshot after
every step, save, quit) - rc 0, 20 screenshots, 1 save, 0 FAILED/STALE
lines on each:

GDI:    PASS  (97.2-99.8 % of pixels off the RGB565 lattice)
SDL2:   PASS  (97.8-99.8 %)
SDL3:   PASS  (97.8-99.8 %; SDL2 vs SDL3 frames 99.9-100 % identical)

pakset:      pak128 (96 pak files, 408 MB, ground.Outside.pak
             86e4f50e5c); pak64 (810 files, 17 MB, 4a2fb9d6b4) for the A1
             control
resize:      PASS on the three backends (above, plus the A2 sequences)
screenshot:  PASS (production `take_screenshot`, 20 per backend, sizes
             follow the resize)
save/load:   PASS (the smoke saves `q25` into `save/`; the world loaded
             from `-load q25.sve` on all three backends; reload was
             exercised by every later run loading the same file)

## Stability

backend:          SDL3 TRUE32 (hooked build), 1 display lane, LIVE,
                  `-freeplay`
pakset:           pak128 `q25.sve`
duration/cycles:  scripted for 45 cycles of scroll / zoom out / zoom in /
                  night 0-6 / day, a resize every third cycle with a
                  `clip` check, a save every fifth, a screenshot every
                  cycle - paced by the wall clock (`waitms`), ~47 s per
                  cycle. Achieved: 32.1 min of continuous scripted
                  activity (05:27:31 -> 05:59:38), 43 complete cycles +
                  most of cycle 44: 88 zoom changes, 87 day/night
                  changes, 14 resizes (900x600 <-> 1200x800), 8 saves
                  (`stab_05` .. `stab_40`, 1 autosave), 44 screenshots
                  (`runs/stability3-sdl3-32/`)
result:           PASS on what the gate measures: no crash, no error, 0
                  FAILED/STALE/unknown lines, 14/14 clip checks MATCH
                  after every resize, every screenshot written at the
                  current size, `exit.rc` = 0.

                  The process ended 9 s before the script would have
                  logged cycle 44's `night 0`, at 05:59:47, through the
                  normal QUIT path and not through the hook: exit code 0,
                  and `autosave-pak128.sve` written at 05:59:46, which is
                  produced only by `karte_t::stop(true)` (the SYSTEM_QUIT
                  handler, `reload_and_save_on_quit`). In the SDL3 backend
                  SYSTEM_QUIT comes only from `SDL_EVENT_QUIT`, i.e. the
                  window being closed (or the in-game Quit tool); nothing
                  in this harness emits it. This is consistent with the
                  window having been closed at the machine at 05:59; the
                  user is asked to confirm. It is not a renderer event:
                  no crash, no error, the state was saved cleanly. The
                  30-40 minute requirement is met by the 32 minutes of
                  activity; the recertification's full run should use a
                  window nobody needs to close.

Two facts about this gate:

1. The certification's frame-counted script (`cert_stability.txt`, 40
   cycles) completes in 45 s of wall time on the remediated SDL3-32 build
   (rc 0, 42 shots, 0 failures) because the hook's counter counts loop
   iterations, not redraws (known since the certification). The hook
   therefore gained a `waitms` command (wall-clock wait via `dr_time()`;
   lab-only header, `-DSTLAB_CERT_HARNESS`, no production change) and the
   script was regenerated (`gen_stability2.py`).
2. The first wall-clock run stopped after cycle 3: `q25.sve` starts at
   -5,135 $ in October 1960 and the player goes BANKRUPT in November 1960
   at live speed; `karte_t::interactive()` then returns and `simu_main`
   shows the welcome menu modally (gdb on the process: main thread in
   `modal_dialogue` under `simu_main`; the window showed "You are
   bankrupt!" over the menu, rendered correctly). Not a renderer event;
   the run was relaunched with `-freeplay`. The process of the stopped run
   (pid 36488, path inside this lane, started by this mission) was the
   only one terminated; no other sim.exe existed on the machine.

## Performance structure

new per-frame allocation:      NO  (`dirty_old_clear` zeroes an existing
                                    map, only inside a real resize)
new per-frame conversion:      NO
new inappropriate hot-path lock: NO  (the recode mutex is taken only on
                                    the recode path, after the caller's
                                    unlocked flag check - once per image
                                    and player, exactly as in simgraph16;
                                    the clip clamp is two `clip_wh32`
                                    calls per `set_clip_rect`, as at 16)

No performance measurement was made (out of scope); the per-frame
benchmark stays INVALID until the recertification rebuilds it.

## Linux

Compile smoke only (WSL Ubuntu, g++ 15.2, `make -j12`, BACKEND sdl2,
MULTI_THREAD, OPTIMISE, the lane's remediated `src`):

    sdl2-32: rc 0 in 45 s, 30 warnings, 0 errors, `sim` 8.7 MB
    sdl2-16: rc 0 in 29 s, 27 warnings, 0 errors

`simsys_w.cc` is not compiled on Linux; the `simgraph32.cc` warnings at 32
are listed in the provenance file and none is in a remediated hunk.

## Remaining known defects

NONE proven.

Recorded observation, not classified: "cut images over water" reported
once by the user from the screen during the A1 stress (welcome-screen
background world, pre-clamp GDI-32 build, 12 lanes). Attempts to
reproduce, all negative:

    - hooked GDI-32 (pre-clamp), `q25.sve` and a fresh 256x256 world, 12
      and 1 lanes: 8 framebuffer/window pairs 100.0 % identical, 0
      strongly-different 16-px blocks, frames visually complete
    - the welcome screen itself cannot be driven by the hook (no `-load`
      -> `interactive()` never runs), so it was observed from outside:
      `wincap.exe` (PrintWindow by PID) on the pre-clamp hooked exe and
      on the final plain exe, 12 captures each over 24 s at 12 lanes, plus
      3 on the final plain exe: no undrawn rectangles in any frame

If it reappears, the recertification has the tool (`wincap.exe`, one
capture per second, no GUI control needed) and the place to look (the
intro loop in `simmain.cc`, which the hook does not cover).

Explicitly NOT listed as TRUE32 product defects (no new evidence): the
memory increase, the invalid old benchmark, the Linux font environment.

## Status

RGBA32_RUNTIME_BLOCKERS:

    CLOSED

RGBA32_RENDERER_STATUS:

    RECERTIFICATION_REQUIRED

## Recommended next action

Re-run STLAB-SIMUTRANS-RGBA32-FULL-RUNTIME-CERTIFICATION from the
remediated frozen candidate (this lane's `src/`, 19 files vs r12254),
including a valid per-frame benchmark (the hook's counter must count
redraws, not loop iterations) and the full stability run with a save that
cannot end the interactive loop (`-freeplay`, or a solvent save). The
recertification should also cover the intro loop (welcome screen) with
external captures, which is where the one unclassified observation was
made.

Not executed.

## Safety

- isolated remediation lane: `_stlab-rgba32-runtime-remediation-01`, its
  own src/head/paks/build trees/run dirs; the certification lane was only
  read (and see the one slip below)
- only the three proven blockers targeted: `simgraph32.cc` (A1 + A2, 7
  hunks) and `simsys_w.cc` (B1, 1 hunk); no other production file touched
- no STORED32
- no makeobj / pak format work
- no cache redesign
- no performance optimisation
- no unrelated renderer refactor
- disposable negative controls only (`nc-gdi-32`, `nc-sdl2-32` under
  `g/builds/`, never the certification tree)
- no SVN commit
- no push
- no publication

Process safety: every process this mission started ran from a path inside
the lane; the two terminations were by PID after enumerating executable
path, command line and start time (pid 1816, 40684, 24428: the external
welcome-screen observations; pid 36488: the bankrupt stability run) -
`non-lab processes terminated : 0`.

## Provenance

source:        trunk r12254 + candidate; lane `src/` md5 of the two
               remediated files: `simgraph32.cc` 504b9409e6,
               `simsys_w.cc` 115b417fed; `reports/candidate-files-after.txt`
               lists the 19 candidate files vs `head/`
live HEAD:     r12254 at report time (see Base)
build dirs:    `g/builds/{cand,hook}-{gdi,sdl2,sdl3}-{16,32}` (incremental,
               rebuilt 04:44-04:45 after the final source),
               `g/builds/hook-sdl3-32` relinked 05:17 for `waitms` (hook
               header only), `g/builds/nc-{gdi,sdl2}-32` (negative
               control), `g/builds/fresh-*` (six fresh builds, warning
               sets), `g/linux/plain` -> WSL `<wsl-home>/rem/plain-{32,16}`
binaries (sha256 prefix, size):
               cand-gdi-16  f7477d2874a61aec 10267108
               cand-gdi-32  5d51279315552208 10280923
               cand-sdl2-16 e8c4e3df165e5a66 10269201
               cand-sdl2-32 35538cf64248df04 10281180
               cand-sdl3-16 5e5e195a11c47cb6 10270962
               cand-sdl3-32 b7d76b9ec8ab05d4 10284623
               hook-gdi-32  70c9824ddec4561a 10300760
               hook-sdl2-32 ec823b81a27e7acf 10302873
               hook-sdl3-32 6e9c38635f1736e9 10305804
               nc-gdi-32    be617896fa9a6809 10280881
               nc-sdl2-32   644af7222c1ea591 10281138
pakset:        `paks/pak128` (96 .pak, 408 MB, ground.Outside.pak md5
               86e4f50e5c); `paks/pak64` (810 .pak, 17 MB, 4a2fb9d6b4)
userdirs:      one per run under `runs/<tag>/` (`-use_workdir
               -singleuser`), each a copy of `simutrans/` with the pakset
               copied in for the run (the copies were removed at close-out;
               the logs, screenshots and saves stay)
logs:          `runs/<tag>/cert.log`, `sim_stdout.log`, `exit.rc`; build
               logs in each build tree; `g/*.txt` for the probes;
               `runs/stress-*` for the 48 A1 starts

### Harness slips, mine, all corrected before the numbers above

- The U9 probe (`u9_probe.cc`, inherited) writes its B4 PNGs to a
  hard-coded path in the closed U9 lane; the first run of this cut
  overwrote `_stlab-rgba32-u9/g/shot32-{full,edge}.png` there. The path
  was redirected to this lane and the files re-generated here are
  byte-identical (md5 794e1fb214ee / aa3488cd5a85) to what the U9 lane now
  holds, so no evidence changed; still, one write left the lane.
- `allgates.sh` called `mkz.sh` without its tree argument (Git-Bash turned
  `/probes/...` into `C:/Program Files/Git/probes/...`) and the 16-bit gate
  copy `cand/build/default` did not exist in this lane; both fixed before
  the zoom and text gates were run.
- The wall-clock stability script needed the hook's new `waitms`; the
  frame-counted one is kept as evidence of what it measures (45 s).
- `ps -W` shows backslash paths; PID lookups for external captures use
  `Start-Process -PassThru` instead, so the PID is known, not searched.
