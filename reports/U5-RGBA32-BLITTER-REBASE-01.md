# STLAB-SIMUTRANS-RGBA32-BLITTER-REBASE-01

## Result

    VERDICT:     A - TRUE ARGB8888 CORE BLITTER FAMILY WORKS
    CONFIDENCE:  HIGH

    SHARED_CORE_TRIGGER: YES

nc, wc and pc consume the true ARGB8888 recoded cache and write full-precision
32 bit pixels. The legacy renderer is untouched, object for object. The
duplication inventory has reached three substantial renderer subsystems, so a
shared-core design cut is recommended before U6.

## Baseline

    SVN:                  r12248
    U4 candidate identity: reproduced exactly. The six files U4 touched hash
                          identically to the U4 lane before any U5 edit, and
                          the U4 foundation probe was re-verified in this lane:
                          STORED=2 SCREEN=4 SAVED=4 NETWORK=2, and
                          rgb(18,52,86) -> 0xFF123456 -> rgb(18,52,86).

## Incremental U5 diff

    files:      1   (src/simutrans/display/simgraph32.cc)
    new files:  0
    hunks:      4
    +/-:        +412 / -4

Cumulative U4+U5 against official r12248: 12 files (3 new), and the only file
U5 added to is the 32 bit renderer itself.

## Pipeline

    STORED16:    image_t::data, STORED_PIXVAL (uint16), untouched by every
                 blitter. No SCREEN pointer ever points at base_data - the
                 recode allocates a separate PIXVAL cache and imd32 keeps the
                 two in different members with different types.
    cache:       imd32::data[player], PIXVAL (uint32). Holds the historical run
                 encoding widened: run counts, ARGB8888 opaque colours, and
                 class+alpha codes for TRANSPARENT_RUN runs.
    framebuffer: PIXVAL* textur, 4 bytes per pixel.

## T1 nc

    input RGB:          (18, 52, 86)  - cannot survive RGB565
    expected SCREEN32:  0xFF123456
    actual:             0xFF123456 in all 32 drawn pixels, wrong=0
    framebuffer hash:   0xDA18C545, changed=32 of 512
    PASS

The full framebuffer was compared, not sampled: 32 pixels changed and 480 still
carry the 0xFF204060 background.

## T2 wc

    clip:            rectangle x in [6,10), full height
    written pixels:  16, exactly the intersection of the sprite with the clip
    outside writes:  0
    hash:            0x19D5EC05
    PASS

## T3 pc

    polygon:         one clip line, the ray (4,2) -> (12,6), ribi 1
    wc comparison:   wc writes 32 pixels, hash 0xDA18C545
    written pixels:  16, hash 0x0F526995
    shape:           y=2 .....#######
                     y=3 .......#####
                     y=4 .........###
                     y=5 ...........#
    PASS

The polygon result differs from the rectangular one for the intended geometric
reason - the diagonal removes a triangle the clip rectangle cannot express -
so this is a decisive pc test, not a wc test in disguise.

## T4 class+alpha

    source class:      stored 0x808C = 0x8020 + class 3 * 31 + alpha step 15
    production path:   recode_img_src_target resolves the transparent player
                       colour and re-emits a class+alpha code; the blitter's
                       colorpixcopy_screen32 decodes it and blends through
                       rgbmap_day_night
    rgbmap_current:    the recode reads rgbmap_day_night for the player entry
                       and the blitter reads it again for the foreground -
                       exactly as simgraph16 does
    cache word:        0x0000BACB  (class 484, alpha 16)
    foreground:        0xFF6080A0
    result:            0xFF406080 in all 18 drawn pixels, wrong=0
    PASS

The alpha model is the historical one: alpha 1..32 against the destination,
arithmetic identical to colors_blend_alpha32 in simgraph16 but carried out at
8 bits per channel so nothing is quantised on the way. No new alpha semantics
were invented.

Note the cache word is not the stored word: the recode rewrote the transparent
*player* colour into a class+alpha code, which is the historical behaviour and
is why this test had to go through the production recode rather than a
hand-made cache.

## Negative controls

    precision NC:  the recoded cache pushed through an RGB565 round trip before
                   blitting, in a disposable build.
                   T1 expected 0xFF123456, got 0xFF103452, wrong=32 -> FAIL
                   T2 also FAIL, for the same colour reason
                   T3 and T4 still PASS, which is correct: the mutation touches
                   opaque colour precision only, not geometry or alpha
                   Restored: the mutation lives only in bp-nc.exe

    pc/wc NC:      NOT RUN. Section 10 asks for it only if needed to prove pc
                   coverage, and T3 already produces a different pixel set and
                   a different hash from wc on the same geometry. Adding a
                   mutant that makes pc behave like wc would prove something
                   the measurement already shows.

### One thing I got wrong on the way

The first pc run produced exactly the wc result. That was my patch, not the
algorithm: simgraph32.cc declares the poly-clip entry points twice, once under
`#ifdef MULTI_THREAD` and once without, and I had filled in only the
non-threaded branch while the build uses MULTI_THREAD. With the threaded
variant filled in, T3 clips as designed. Recorded because a green run after a
silent no-op is exactly the kind of result that deserves saying out loud.

## Legacy lane

    simgraph16 unchanged:  YES, byte-identical to official r12248
    objects identical:     361 of 361 identical to the official r12248 build
    build:                 0 errors, 361 objects, sim.exe 10 264 252 bytes
    warnings:              50 in 11 categories, warning SET identical to
                           official r12248
    302/302:               PASS - "Tests completed successfully.", 0 error
                           blocks. Suite provenance verified: 34 files,
                           test_convoy_state.nut present, pak-local copy
                           stripped, markers judged after the real banner.
    20/20:                 PASS, 0 failures

Nothing U5 added is visible at COLOUR_DEPTH 16: every line lives in
simgraph32.cc, which that lane does not compile.

## 32-bit lane

    compile:   PASS, 0 errors, 361 objects
    link:      PASS, sim.exe 10 181 834 bytes
    renderer:  simgraph32.o present, simgraph16.o absent
    warnings:  60 in 12 categories on a clean build - the same total as U4.
               simgraph32.cc contributes 9 of them, down from 10: implementing
               the blitters retired one unused-function warning naturally.
               Nothing was silenced.
    remaining fail-closed stubs: 43 functions in simgraph32.cc still have an
               empty body (measured, not estimated). That is the rest of the drawing API - colored and
               daytime, blend, alpha, text, rectangles, lines, zoom - and it is
               deliberately out of U5's scope. They cannot pretend to draw
               correctly: they draw nothing at all.

## Width guards

    STORED:  2 bytes, static_assert present
    SCREEN:  4 bytes in the 32 bit lane, 2 in the 16 bit lane
    SAVED:   4 bytes, static_assert present
    WIRE:    2 bytes, static_assert present
    flag bits inside SCREEN: 0 - the U4 probe's all-ones screen colour still
             collides with no draw flag

## Formats

    pak:      unchanged
    makeobj:  unchanged (src/makeobj byte-identical)
    save:     unchanged (simversion.h, loadsave.cc, image_writer.cc,
              objversion.h all byte-identical)
    network:  unchanged (network_cmd.cc, gameinfo.h byte-identical)
    MOTD:     r12247 fix intact, 1/1/1

Also byte-identical and worth naming because a blitter cut could plausibly have
touched them: image.h, image.cc, gameinfo.h, server_frame.cc, simgraph16.cc.

## Memory

    stored sprite:   44 words x 2 bytes =  88 bytes
    SCREEN32 cache:  44 words x 4 bytes = 176 bytes
    ratio:           2.0x

Measured on the T1 sprite through the production recode. This is the cache
doubling the audit predicted; no optimisation was attempted.

## Threading

    clip_num parity: NO.
    status:          simgraph16 keeps one clipping_info_t per thread and
                     selects it with CLIP_NUM. simgraph32 keeps exactly ONE
                     context, `CR32`, and its entry points accept the CLIP_NUM
                     parameter and ignore it.
    surface area:    CR32 (clip_rect, clip_rect_swap, number_of_clips,
                     active_ribi, clip_ribi[], poly_clips[], xranges[]) and the
                     four entry points that write it: set_clip_rect,
                     add_poly_clip, clear_all_poly_clip, activate_ribi_clip.
                     Any concurrent drawing through this renderer would share
                     that state.
    deferred:        yes. No thread safety is claimed for the 32 bit renderer,
                     and U5 does not attempt it.

## Duplication inventory

    1. Palette / colour conversion        (added by U4)
       calc_base_pal_from_night_shift, rgbmap_* and specialcolormap_* tables,
       player offsets, init_colour_tables. Reproduced from simgraph16.

    2. Clipping geometry                  (added by U5)
       xrange / clip_line_t Bresenham, init_ranges, get_xrange_and_step_y and
       the clip rectangle + poly clip state. Pure integer geometry, carrying no
       colour type at all, and therefore duplicated verbatim.

    3. Run decoding / image traversal     (added by U5)
       The RLE row walker appears in nc, wc and pc, in display_img_aux's
       skip-lines loop and in the recode - now once per renderer, so twice in
       the tree.

    4. get_system_color                   (pre-existing, not created here)
       Four independent copies in simsys_w / _s2 / _s3 / _posix, one of which
       U4 had to make depth-aware.

    SHARED_CORE_TRIGGER: YES

Three substantial renderer subsystems are now duplicated between simgraph16 and
simgraph32. That is the threshold section 16 sets, and it is reached honestly:
each of the three is real logic, not boilerplate, and each will have to be
changed twice from now on. The backend one is a fourth, older argument for the
same conclusion.

## Recommendation

    next: STLAB-SIMUTRANS-RGBA32-SHARED-CORE-DESIGN-01

NOT executed here. U6 (colored/daytime) should wait for that design cut, as
section 16 directs.

## Safety

    no colored/daytime:     confirmed
    no zoom:                confirmed
    no text/GUI:            confirmed
    no backend sweep:       confirmed - SDL2, SDL3 and POSIX untouched, their
                            RGB565 get_system_color still pending
    no threading redesign:  confirmed
    no save/network change: confirmed
    no pak/makeobj change:  confirmed
    no SVN commit:          confirmed
    no push:                confirmed
    no publication:         confirmed

## Provenance

    source:    svn://servers.simutrans.org/simutrans/trunk @ r12248 + U4 + U5
    base:      _stlab-rgba32-u5/base    untouched official r12248
    cand:      _stlab-rgba32-u5/cand    U4+U5, built at COLOUR_DEPTH=16
    cand32:    _stlab-rgba32-u5/cand32  same source, COLOUR_DEPTH=32
    probe:     blitter_probe.cc - #includes the production simgraph32.cc and
               links the other 359 production objects; bp-nc.exe is the
               precision negative control
    toolchain: MinGW-w64 g++ 16.1.0, mingw32-make
    config:    BACKEND=gdi OSTYPE=mingw MSG_LEVEL=3 OPTIMISE=1 MULTI_THREAD=1
    processes: non-lab processes terminated: 0
