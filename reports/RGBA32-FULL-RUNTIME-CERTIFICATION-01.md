# STLAB-SIMUTRANS-RGBA32-FULL-RUNTIME-CERTIFICATION-01

## Verdict

    C - PRODUCT CERTIFICATION BLOCKED BY TRUE32 DEFECT
    CONFIDENCE: HIGH

    RGBA32_RENDERER_STATUS = NOT_READY

The real application exposed three defects the laboratory gates could not,
because none of them lives in pixel arithmetic: two are transposition
fidelity gaps in the 32-bit renderer's *lifecycle* code (a missing lock, a
missing clip reset on resize) and one is the GDI backend presenting a 32-bit
DIB with 16-bit masks. Each was reproduced deterministically, traced to its
smallest cause in source, and - per the defect policy - left unpatched. One
remediation cut closes all three; it is named at the end.

Everything else that could be measured around the defects passed: the six
Windows and four Linux builds, the pure-trunk legacy hash, startup on all
three backends, real rendering of pak64 and pak128 through scroll, zoom, day/
night cycles, four rotations, resize, screenshot, save and reload, ARGB8888
precision end-to-end (85-99.6 % of real pixels carry values a 16-bit pipeline
cannot emit), the 16-bit legacy runtime, and cross-backend consistency of the
framebuffer.

## Base

    live HEAD:              r12254 (<svn-author>, 2026-09-05 02:45:31 +0200)
    certified renderer base: r12248 + U4..U10 + SDL-01/02
    upstream movement r12249..r12254, classified against a binary-clean
    export of r12248 (a first pass through `svn cat` in PowerShell re-encoded
    every non-ASCII byte and reported all 16 files as overlapping - discarded):
        r12249  base.tab, tool/simtool.cc (translation text)   unrelated
        r12250  base.tab                                        unrelated
        r12251  base.tab                                        unrelated
        r12252  pakset_downloader.cc, network_file_transfer.*  unrelated
        r12253  pakset_downloader.cc                            unrelated
        r12254  cmake/SimutransInstall.cmake                    packaging-only
    overlap with the candidate footprint: NONE
    rebase:                 mechanical - trunk r12254 + the 18 candidate files
    candidate identity (vs r12254):
        12 modified  +346 / -806, 40 hunks
            gameinfo.cc +2/-2, ground_desc.cc +7/-7, image_reader.cc +2/-2,
            simgraph.cc +8/-0, simgraph.h +3/-2, simgraph16.cc +143/-762
            (55fa74a19f5e), simcolor.h +104/-8, simmesg.cc +2/-2,
            simsys.h +2/-2, simsys_s2.cc +35/-6, simsys_s3.cc +30/-8,
            simsys_w.cc +8/-5
        6 new        blit_clip_core.h 216, blit_core.h 171, simgraph32.cc 3589
                     (918f4aa9a1ea), simgraph32.h 16, simgraph_palette.h 104,
                     zoom_core.h 604
    identity gates on the rebased tree:
        inventory 90/90/0/0; 29-scenario legacy hash
        4a758dc6c6612f7be4d49a46a2825d6dec93a9799ed12884ea864806748a6b39 in
        BOTH the candidate and the PURE trunk r12254 simgraph16.cc, default
        and LOW_LEVEL - the refactored legacy file is output-identical to
        trunk, not merely to its lab ancestors.
    one lab correction: the cand32 lab tree carried a stale pre-SC-02
        simgraph16.cc that the 32-bit build never compiles; the certification
        tree takes the legacy-frozen file from the 16-bit tree.

## Build matrix

    | Platform      | Backend | Depth | MT      | Result                         |
    |---------------|---------|-------|---------|--------------------------------|
    | Windows MinGW | GDI     | 32    | on      | sim.exe, 361 obj, 0 err        |
    | Windows MinGW | GDI     | 16    | on      | sim.exe, 361 obj, 0 err        |
    | Windows MinGW | SDL2    | 32    | on      | sim.exe, 361 obj, 0 err        |
    | Windows MinGW | SDL2    | 16    | on      | sim.exe, 361 obj, 0 err        |
    | Windows MinGW | SDL3    | 32    | on      | sim.exe, 361 obj, 0 err        |
    | Windows MinGW | SDL3    | 16    | on      | sim.exe, 361 obj, 0 err        |
    | Windows MinGW | GDI     | 16    | trunk   | reference (warning baseline)   |
    | Linux WSL g++15 | SDL2  | 32    | on      | sim, 360 obj, 0 err            |
    | Linux WSL g++15 | SDL3  | 32    | on      | sim, 360 obj, 0 err            |
    | Linux WSL g++15 | SDL2  | 16    | on      | sim, 360 obj, 0 err            |
    | Linux WSL g++15 | SDL2  | 32 +ASan/UBSan | on | sim (228 MB), 0 err      |

    compiler: MinGW-w64 g++ 16.1.0 / Ubuntu g++ 15.2.0; x86-64; MULTI_THREAD
    compiled in everywhere, lane count chosen at runtime with -threads.
    Candidate warnings in the touched backend files: 0 in every build.
    Certification-hook builds (-DSTLAB_CERT_HARNESS, lab only) for GDI-32,
    SDL2-32, SDL3-32, GDI-16, SDL2-16 and Linux SDL2-32: without the define,
    simworld.o is byte-identical to the plain source.

## Startup

    GDI32:        real window, ARGB8888 framebuffer, pak64 and pak128, world
                  drawn - but the WINDOW shows the framebuffer through RGB565
                  masks (defect B1 below); and with the default 12 lanes the
                  process crashes at startup in 6 of 12 runs (defect A1)
    SDL2-32:      real window, exact presentation (window == framebuffer
                  100.0 %), same A1 crash rate at default lanes
    SDL3-32:      real window, exact presentation, same A1
    Linux SDL2-32: window under WSLg, no crash in 20 s; the shipped font
                  (font/cyr.bdf, size 11) fails to load on this Linux
                  FreeType in BOTH the 16- and 32-bit builds, so the first
                  dialog renders without text and the world is never
                  reached - environment, not renderer
    Linux SDL3-32: built; not run (same environment limitation)

## Pixel formats

    GDI:   framebuffer ARGB8888 (screenshots prove it); DIB header
           BI_BITFIELDS + RGB565 masks at 32 bpp - WRONG (B1)
    SDL2:  SDL_PIXELFORMAT_ARGB8888 texture, exact
    SDL3:  SDL_PIXELFORMAT_ARGB8888 texture, exact

## Paksets

    pak64:      simupak64-124-4 (cached zip) - load PASS, map visible,
                vehicles/buildings visible, 20/20 certification screenshots on
                all three backends
    pak128:     the local pak128 (102 entries, 408 MB) with q25.sve - load
                PASS on GDI-32 (20/20 shots); SDL3-32 crashed at shot 7 with
                4 lanes (A1)
    additional: pak192.comic prepared; not run once the blockers were proven

## Runtime scenarios

Driven by a lab-only hook that calls production entry points (viewport,
win_change_zoom_factor, karte_t::rotate90, env_t::daynight_level,
env_t_rgb_to_system_colors, gfx->on_window_resized, gfx->take_screenshot,
karte_t::save, karte_t::init); no window was driven from outside.

    map generation:     newmap command available; not exercised - blocked by
                        A1 at the lane counts the generator's first draw uses
    save/load:          PASS - save from the run, restart with -load of that
                        save, identical scene statistics
    GUI:                windows render (welcome screen, toolbars, dialogs);
                        interactive widget states not driven (no GUI control)
    text:               renders on Windows (welcome text, labels, GUI);
                        Linux blocked by the font environment
    zoom:               PASS - 40/48/64/85 raster widths, images stable
    scroll:             PASS - viewport jumps across the map
    resize:             production path exercised 800x500 / 1400x900 /
                        1024x640 - PASS on GDI-32 and, with pak64, on
                        SDL2/SDL3-32; with pak128 and an open text window,
                        SDL2/SDL3-32 crash deterministically after the first
                        shrink (defect A2)
    day/night:          PASS - levels 0/3/6 and four full cycles, no drift
                        (shot after cycles == shot before, 100.0 % on SDL3)
    player colours:     rendered in the loaded worlds; not isolated
    alpha/transparency: rendered (pak64 alpha ground transitions are the very
                        path A1 crashes in); no halos or opaque holes seen in
                        the analysed shots
    screenshot:         PASS - production PNG path, decoded with PIL,
                        RGB order correct, no alpha misinterpretation
    construction:       not driven (no GUI control; the scripted scenario
                        route was not reached before the blockers)
    rotation:           PASS - four rotate90 steps return to orientation 0
                        with 99.6-99.7 % identical pixels to the start

## Threading

    single-thread:  -threads 1: no A1 crash in 20 runs (but A2 still
                    crashes, single-lane, on resize)
    MT:             -threads 4: crashes in 4 of ~20 runs; -threads 8: 0 of 2;
                    -threads 12 / default (12 on this 16-core machine): 8 of
                    18 runs crash within seconds of first display
    contamination:  none observed in the paused cross-backend shots
    crashes:        A1, backend-independent (GDI and SDL2 alike), never in
                    the 16-bit builds (0 of 6 at 12 lanes)

## Sanitizers

    ASan:    Windows MinGW: no libasan/libubsan shipped - not available.
             Linux SDL2-32 hooked build with -fsanitize=address,undefined
             (via CXX override; LDFLAGS on the command line clobbers the
             Makefile's libraries - lesson recorded): built, ran 240 s,
             0 reports - but only the startup and first dialog ran, because of
             the Linux font environment. INCONCLUSIVE for world rendering.
    UBSan:   same session, 0 reports, same caveat
    relevant findings: none in what executed

## Memory

    16-bit baseline:  487 MB working set, steady over 45 s (pak128 q25.sve,
                      1024x640, GDI, 1 lane)
    32-bit:           919 -> 930 MB over 45 s, same scene
    delta:            +432..443 MB
    expected:         the framebuffer accounts for ~1.3 MB of that; the rest
                      is the per-pixel image caches of every drawn pak128
                      image at twice the width - explainable in magnitude,
                      not measured to the byte
    leak/growth:      +11 MB in the first 45 s; the 40-minute trend could not
                      be taken because the stability run crashed (A2)

## Performance

    method:      wall time of `-load q25.sve -until 1930.3` (pak128, load +
                 one simulated month + exit), 3 runs per configuration
    16-bit:      -threads 4: 1.51 / 1.13 / 1.21 s   -threads 1: 1.16 / 1.13 / 1.09 s
    32-bit:      -threads 4: 3.47 / 5.09 / 5.26 s   -threads 1: 3.15 / 3.11 / 3.09 s
    delta:       about 2.8x (1 lane) to 3.5x (4 lanes) longer
    classification: MATERIAL on this proxy - but the proxy is dominated by
                 startup (first recode of every pak128 image into 32-bit
                 caches, twice the bytes), not by per-frame rendering. A
                 per-frame benchmark was attempted through the hook and is
                 NOT valid: the hook counts interactive-loop iterations, not
                 redraws ("601 frames in 0 ms"). Per-frame render cost is
                 unmeasured here and must be part of the follow-up.

## Stability

    duration:     5 minutes of a planned 40 (hook-sdl3-32, pak128, 1 lane)
    backend:      SDL3-32
    result:       CRASH at the first resize of cycle 3 (defect A2)
    memory trend: not obtained (sampler quoting failed; the 45-s samples
                  above stand)

## Cross-backend consistency

Paused world (identical simulation state), pak64, framebuffer screenshots:

    GDI vs SDL2:  85-99.9 % identical pixels, mean |diff| 0.15-7.5
    GDI vs SDL3:  92-100.0 % identical pixels, mean |diff| 0.01-3.7
    SDL2 vs SDL3: same band
    pak128 GDI vs SDL3: 96.8-99.9 % on the shots both produced

    The sub-10 % differences are timing (a shot taken a frame before a
    redraw completed - one 0.2 % outlier is exactly that), not colour: the
    same off-lattice share, colour counts and means on every backend. Live
    (unpaused) runs differ more because vehicles move between processes.

## Legacy 16-bit

    GDI:   build PASS; -load q25.sve -until 1930.3 exits cleanly; 0 of 6
           startup crashes at 12 lanes; hooked GDI-16 resize run PASS
    SDL2:  build PASS; hooked view run 20/20 shots, resize PASS, save PASS;
           screenshots contain 0.0 % values outside the RGB565 expansion
    SDL3:  build PASS; -load/-until exits cleanly
    byte identity: simsys_s2.o / simsys_s3.o at 16 byte-identical to trunk
           (SDL-02); simgraph16 output hash identical to pure trunk

## Defects

### A1 - TRUE32 renderer: recode_img has no lock (startup race)

    symptom:   SIGSEGV within seconds of first display, GDI and SDL2 alike,
               32-bit only; 6/12 at the default 12 lanes with pak64, 4/4
               with pak128, also seen at 4 lanes; 0/12 in the 16-bit build
    where:     readers of the recoded image cache - colorpixcopy_screen32
               (simgraph32.cc:395), blit_core_clipped (blit_core.h:163),
               reached from draw_img_aux, draw_rezoomed_img_alpha, the
               wc/nc paths - the source buffer freed or reallocated under
               the reader
    cause:     simgraph16::recode_img takes recode_img_mutex and re-checks
               player_flags inside it; simgraph32::recode_img (U6) has
               neither. Two lanes recode the same image at once; rezoom
               frees data[] meanwhile.
    proof:     a DISPOSABLE build with only the legacy lock + re-check
               restored: 0 of 12 (pak64) and 0 of 4 (pak128) crashes at the
               default lanes, against 3 of 18 and 4 of 4 for the candidate
               in the same batches; the candidate's cumulative rate at
               default lanes today: 8 of 18.

### A2 - TRUE32 renderer: on_window_resized does not reset the clip

    symptom:   deterministic SIGSEGV in simgraph32_draw_text_clipped_n while
               a GUI window's text is drawn after the first shrink, on
               SDL2-32 and SDL3-32, single lane, paused or live, pak128 with
               q25.sve (an open text window); never on GDI-32, never at 16
    cause:     simgraph16_on_window_resized ends with
               set_clip_rect(0, 0, disp_actual_width, disp_height) and
               mark_screen_dirty(); simgraph32_on_window_resized only calls
               dr_textur_resize and dirty_state_alloc. The clip rectangle
               keeps the old 1024x640, the text writer trusts it, and writes
               past SDL's exact-size framebuffer. GDI's DIB is allocated at
               the maximum window size, so the same overrun lands inside
               memory and only corrupts silently.
    proof:     reproduced 3 of 3 on SDL3-32 and 1 of 1 on SDL2-32 with a
               four-line script; 0 of 3 on GDI-32/GDI-16/SDL2-16 with the
               same script; backtrace through gui_flowtext -> draw_text_
               clipped_n; the two functions side by side.

### B1 - GDI backend: 32 bpp DIB with RGB565 masks

    symptom:   the whole window pink/magenta with garbled icons and
               yellow/green text (the user's screenshot) while every
               production screenshot of the same frames is correct
    cause:     simsys_w.cc sets biBitCount = COLOUR_DEPTH but leaves
               biCompression = BI_BITFIELDS with masks 0xF800/0x07E0/0x001F
               - unchanged since trunk, never made depth-aware
    proof:     a standalone GDI program with that exact header: RGB(18,52,86)
               presents as (49,138,181), red as black, green as (255,227,0) -
               each equal to the hand prediction from the 16-bit masks;
               BI_RGB presents every vector exactly. SDL2's window equals its
               framebuffer 100.0 %, GDI's does not.

### environment/tooling

    Linux font:        font/cyr.bdf size 11 not loadable by this FreeType on
                       Ubuntu, 16 and 32 alike - blocks the Linux runtime
                       lanes; not a renderer matter
    Windows sanitizers: not available on MinGW-w64 16.1
    frame timing:      the hook's frame counter is a loop counter; per-frame
                       render timing not obtained
    PrintWindow:       reliable for the first frame only (SDL-02 finding);
                       used only for the presentation proof

## Debt

    KNOWN_RENDERER_DEBT:     A1 (recode lock), A2 (resize clip reset) - both
                             transposition fidelity gaps, both tiny
    SDL32_BACKEND_DEBT:      CLOSED (SDL-02) - and confirmed at runtime here
    SDL_SYSTEM_COLOUR_DEBT:  CLOSED
    GDI32_PRESENTATION_DEBT: OPEN (new, B1)

## Product status

    RGBA32_RENDERER_STATUS = NOT_READY

## Recommended next action

    STLAB-SIMUTRANS-RGBA32-RUNTIME-REMEDIATION-01

Exactly the three proven causes, nothing else:
  1. simgraph32.cc recode_img: the legacy recode_img_mutex + re-check
     (already demonstrated in the disposable build);
  2. simgraph32.cc on_window_resized: the legacy epilogue - clip reset to
     the new size, mark_screen_dirty, the min-16 / 64 guards;
  3. simsys_w.cc: at COLOUR_DEPTH == 32, BI_RGB (or the ARGB8888 masks) in
     the DIB header.
Then re-run THIS certification's gates on the remediated candidate - the
same scripts and analysers are in the lane - including a valid per-frame
benchmark and the 40-minute stability run. Do NOT integrate first.

## Safety

    live HEAD checked:                 r12254, all movement classified
    certified candidate identity preserved: yes - the certification tree is
                                       trunk + the unchanged candidate files;
                                       the isolation build with the recode
                                       lock is a DISPOSABLE copy under
                                       g/builds/proof-gdi-32 and is not the
                                       candidate
    no unrelated renderer redesign:    confirmed - nothing in the candidate
                                       was edited in this mission
    no STORED32 work:                  confirmed
    no makeobj format changes:         confirmed
    no native RGBA pak format changes: confirmed
    processes:                         the runs' own sim.exe instances were
                                       ended by PID after memory sampling;
                                       the make/cmake processes of another
                                       lab seen earlier were left alone.
                                       non-lab processes terminated: 0
    no SVN commit:                     confirmed
    no push:                           confirmed
    no publication:                    confirmed

## Provenance

    lane:      <lab>/_stlab-rgba32-cert1 (fresh)
    trees:     head/ = svn export r12254; g/base12248/ = svn export r12248;
               src/ = head + candidate overlay; g/builds/<cfg>/ per build;
               WSL <wsl-home>/cert/<cfg>/ for Linux
    harness:   stlab_cert_hook.h + apply_cert_hook.py (lab-only,
               -DSTLAB_CERT_HARNESS), cert_view.txt / cert_menu.txt /
               cert_resize.txt / cert_stability.txt / cert_fps.txt,
               runcert.sh, analyze_shots.py (PIL), dibproof.cc,
               runs/<name>/ with cert.log, PNGs, saves and stdout for every
               session quoted above
    paksets:   pak64 124-4, pak128 (local), pak192.comic (prepared)
    saves:     tutorial64.sve, q25.sve; cert_saved.sve produced by the runs
    toolchain: MinGW-w64 g++ 16.1.0, Ubuntu g++ 15.2.0 under WSL with WSLg,
               PIL 12.3.0

### Harness slips, mine, all corrected before the numbers above

  * `svn cat` through a PowerShell pipe re-encoded bytes - replaced by a
    binary-clean `svn export`;
  * the first Linux build chain died with its WSL session (background jobs
    do not survive) - rebuilt in the foreground of one session;
  * `LDFLAGS=` on the make command line clobbered the Makefile's libraries -
    the sanitizer build uses a CXX override instead;
  * the ARGB witness was first written as "not a multiple of 8/4", which the
    16-bit screenshot expansion also violates - replaced by the exact
    expansion sets (16-bit: 0.0 %, 32-bit: 85-99.6 %);
  * the memory sampler for the long run failed on quoting - only the 45-s
    samples exist;
  * `bc` is not installed - timings redone with integer arithmetic;
  * the per-frame benchmark counted loop iterations - reported as invalid.
