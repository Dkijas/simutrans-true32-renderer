# STLAB-SIMUTRANS-RGBA32-BACKEND-CLOSURE-01

## Result

    VERDICT:     A - BACKEND API COMPLETE
    CONFIDENCE:  HIGH

    BACKEND_STUBS        = 0
    BACKEND_PARTIAL      = 0
    BACKEND_DEBT         = CLOSED
    SET_LIGHT_COLOR_DEBT = CLOSED

    API_SURFACE          = INCOMPLETE   (88 of 90 - see the correction below)

All three backend entry points are real and each is proven through its
production effect: the seven system colours land in env_t at full ARGB8888
precision; a pakset light colour set through the public entry point reaches
`calc_base_pal_from_night_shift` via `set_daynight_level` and produces the
hand-derived palette entries at three night levels; the screenshot is a valid
RGB888 PNG decoded independently with exact dimensions, positions, channel
order and values, and the failure contract returns false leaving no file. Not
one previously certified framebuffer hash moved.

**A correction that outranks the closure.** Closing this cut required a
harder look at the inventory rule, and it was blind to one shape of
placeholder: a body that returns a zeroed aggregate - `return { 0,0,0 };`,
`return { 0, 0 };`. Two such functions have been counted REAL since U8B:

    simgraph32_get_color_rgb              GUI     returns {0,0,0}   legacy: special_pal / display_day_lights lookup
    simgraph32_calc_multiline_text_size   TEXT    returns {0,0}     legacy: 25-line glyph-advance walk

So `TEXT_PARTIAL = 0` in U8B and `GRAPHICAL_SURFACE = COMPLETE` in U8E were
both stated on a rule that could not see them. **GRAPHICAL_SURFACE is
retracted to INCOMPLETE** by those two functions. Neither is in this cut's
scope and neither is touched; both are pure transpositions (the font and the
palette they read are real), and they are named as the first thing to do next.

## Baseline

    live HEAD:     r12251, queried. Unchanged since U7A.
    lab base:      r12248 + U4 .. U8E
    U8E identity:  verified BEFORE any edit -
                   simgraph16.cc     55fa74a19f5e060dfe3fb3c130fa6c37
                   simgraph32.cc     479a99d4b5a4dd2e6555de80439b425d
                   blit_core.h       73e38e7a60de6d23d4c2a1d1ca07db72
                   blit_clip_core.h  bd627098336fe34a7a6902c13647228e
                   zoom_core.h       2549976efd05df8ce99984ac479a76f3
    U8E state reproduced before editing: ID1-ID4 PASS, all ten matrix owners
    at dirty=4, GSURF3 = 4, GSURF5 = 6/6.

## Initial inventory

    old rule:        TOTAL 90   REAL 87   STUB 2   PARTIAL 1
    corrected rule:  TOTAL 90   REAL 85   STUB 2   PARTIAL 3

    The corrected rule was run on the untouched U8E base as well, so the two
    extra partials are demonstrably pre-existing, not introduced here.

## Backend inventory

### env_t_rgb_to_system_colors

    status before:       STUB - empty body
    legacy counterpart:  simgraph16_env_t_rgb_to_system_colors, line 633
    callers:             simmain.cc:941 once the graphics system is up;
                         settings_frame.cc:194 after the colour settings change
    state:               reads env_t::*_color_rgb (rgb888_t, from settings.xml
                         or defaults), writes the seven env_t::*_color PIXVALs
                         used by the GUI. Startup + on settings change; not
                         per frame.
    platform dependency: get_system_color(), supplied by the backend - see
                         Platform ownership. Abstract RGB888 in, SCREEN colour
                         out; on this platform at COLOUR_DEPTH=32 that is
                         ARGB8888 with every bit kept (simsys_w.cc, U4).

### set_light_color

    status before:       STUB - empty body, parameters unnamed
    legacy counterpart:  simgraph16_set_light_color, line 4362: two array stores
    callers:             settings.cc:789, parse_simuconf, for every
                         `special_color[i]` with six ints - pakset/simuconf
                         source, at load time
    storage:             display_day_lights / display_night_lights,
                         `static` in simgraph_palette.h - so each renderer
                         owns its own copy and this writes simgraph32.cc's
    production consumers: calc_base_pal_from_night_shift reads both arrays -
                         the interpolated light entries at
                         rgbmap_day_night[0x8000+MAX_PLAYER_COUNT+i] and the
                         day colour at specialcolormap_day_night[224+i].
                         It is NOT called by the setter; simview.cc calls
                         set_daynight_level, which rebuilds. That lifecycle is
                         preserved exactly: the setter stores, nothing else.

### take_screenshot

    status before:       PARTIAL - `return false;`
    legacy counterpart:  simgraph16_take_screenshot, line 4319
    format:              raw_image_t FMT_RGB888, written by write_png: 24 bit
                         RGB, no alpha, rows top-down from y = 0
    framebuffer source:  textur, the area clipped to (0,0,disp_actual_width,
                         disp_height), row stride disp_width
    platform dependency: none - raw_image_t and libpng are common code;
                         dr_fopen for the file
    return contract:     write_png's bool; false on an unopenable file, on
                         libpng struct failure, and on a longjmp error. On the
                         fopen failure no file is created; the caller
                         (simtool.cc:8042) shows "Could not create screenshot".

## Dependency map

    settings.xml / defaults                   simuconf.tab special_color[i]
            |                                            |
            v                                            v
    env_t::*_color_rgb (rgb888)            gfx->set_light_color(i, day, night)
            |                                            |
    gfx->env_t_rgb_to_system_colors()      display_day_lights[i] / display_night_lights[i]
            |                                 (simgraph32.cc's own static copy)
            v                                            |
    get_system_color()  <---- backend  ---->  calc_base_pal_from_night_shift()
     (simsys_w: ARGB8888)                     ^  called by set_daynight_level (simview)
            |                                 |
            v                                 v
    env_t::*_color (PIXVAL)          rgbmap_day_night[] / specialcolormap_day_night[]

    textur (ARGB8888) --take_screenshot--> simgraph32_get_pixval_rgb --> raw_image_t RGB888 --> write_png

    The three are independent of one another. Two of them share
    get_system_color(); the screenshot shares nothing with either.

## Incremental diff

    files:      1 modified (simgraph32.cc, both trees)
    new files:  0
    hunks:      5
    +/-:        +36 / -4

    24 added code lines: 7 get_system_color calls, 2 array stores, the
    screenshot body (2 loops - screenshot only), 1 include, 1 decode call.
    0 RGB565 masks, 0 width assumptions. simgraph16.cc, every shared header,
    simsys_w.cc and raw_image* byte-identical to U8E.

## B1 - system colours

    inputs:         seven RGB888 values, all off the RGB565 lattice, channels
                    asymmetric so a swap shows: (18,52,86) (201,77,13)
                    (3,250,129) (77,13,201) (129,3,250) (250,129,3) (111,222,33).
                    Outputs poisoned to 0x5A5A5A5A first, so a no-op cannot
                    pass by luck.
    expected TRUE32: 0xFF000000 | r<<16 | g<<8 | b, transcribed from
                    simsys_w.cc's COLOUR_DEPTH==32 branch - 0xFF123456,
                    0xFFC94D0D, 0xFF03FA81, 0xFF4D0DC9, 0xFF8103FA,
                    0xFFFA8103, 0xFF6FDE21
    actual:         all seven exact
    RGB565 alternative: 0xFF103450 for the first (differs); channel-swapped
                    0xFF563412 (differs)
    legacy oracle:  the same probe on simgraph16 gives 0x11AA, 0xCA61, ...,
                    matching the transcribed RGB565 rule - so both renderers
                    apply the same semantic through their own format
    PASS/FAIL:      PASS

## B2 - light colour

    custom input:   light 2, day (18,52,86), night (201,77,13) - both off
                    the lattice; light 3 (red FF211D / FF211D) left untouched
                    as the control
    stored:         day {18,52,86} night {201,77,13}, read back from the
                    arrays
    consumer:       set_daynight_level(2), (0), (4) - each a real rebuild
                    through calc_base_pal_from_night_shift
    expected palette result: transcribed interpolation
                    night2 = min(n,4), day = 4-night2,
                    c = (day_c*day + night_c*night2) >> 2
                    n=2: (18*2+201*2)>>2 = 109, (52*2+77*2)>>2 = 64,
                         (86*2+13*2)>>2 = 49              -> 0xFF6D4031
                    n=0: 0xFF123456        n=4: 0xFFC94D0D
                    special map [224+2] = day colour       -> 0xFF123456 at all levels
                    light 3 at every level                 -> 0xFFFF211D
    actual:         all twelve values exact; the untouched light 3 did not move
    PASS/FAIL:      PASS

    Production-reachable: YES. The value travels public setter -> static
    array -> the same rebuild path simview.cc drives -> the tables the image
    writers read. That is what closes SET_LIGHT_COLOR_DEBT.

## B3 - light negative control

    no-op mutant:   the two array stores replaced by (void) casts, in a
                    disposable copy, removed afterwards
    gate failed:    YES - B2 FAIL: light 2 stays at the default yellow
                    0xFFFFFF53 at night=2, stored {255,255,83}; light 3 still
                    correct. B1, B4, B5 unaffected.

## B4 - screenshot TRUE32

    format:         PNG, 24 bit RGB (PIL reports mode RGB)
    dimensions:     full 24x10; sub-rect (4,2,8,4) -> 8x4; an area hanging
                    off the screen (18,6,20,20) -> clipped to 6x4, as the
                    legacy clips it
    test pixels:    (1,1) red, (5,2) green, (9,3) blue, (12,4) RGB(18,52,86),
                    (20,8) RGB(201,77,13) on a (31,67,99) background
    decoded expected: the painted RGB888, bit for bit
    decoded actual: (255,0,0) (0,255,0) (0,0,255) (18,52,86) (201,77,13) at
                    exactly those positions in the full file, (1,0)/(5,1) in
                    the sub-rect, (2,2) in the edge file; 0 wrong background
                    pixels in any of the three
    channel order:  RGB - the red pixel decodes as (255,0,0), not (0,0,255)
    vertical orientation: top-down - red at row 1, not row 8
    decoder:        PIL 12.3.0 in decode_u9.py, no renderer code involved
    PASS/FAIL:      PASS

    Legacy geometry (section 14): the same probe on simgraph16 yields files
    of identical dimensions with every test pixel at the same position; the
    only differences are the colours, and they are exactly the RGB565
    round-trip - (18,52,86) -> (16,52,82), (201,77,13) -> (205,76,8) - as
    hand-derived from the legacy pixval_to_rgb888 rule
    (5 bit * 255 / 31, 6 bit * 255 / 63). Pure red/green/blue survive intact
    in both.

## B5 - screenshot failure

    case:           destination inside a directory that does not exist
    expected:       return false, no file
    actual:         returned false, file absent
    partial file:   none - dr_fopen fails before anything is written, which
                    is the legacy path too
    PASS/FAIL:      PASS

## Platform ownership

    GDI (simsys_w.cc):     get_system_color returns ARGB8888 at
                           COLOUR_DEPTH == 32 (U4). Tested here.
    SDL2 (simsys_s2.cc):   hard-codes SDL_PIXELFORMAT_RGB565 and asserts the
                           upper 16 bits are zero - would yield RGB565 values
                           in a 32 bit PIXVAL. NOT tested here.
    SDL3 (simsys_s3.cc):   the same, through SDL_GetPixelFormatDetails.
                           NOT tested here.
    POSIX (simsys_posix.cc): returns 1 - the headless stub, by design.
    common vs platform-specific:
        set_light_color   common - renderer-local arrays, no backend
        take_screenshot   common - raw_image_t + libpng + dr_fopen
        env_t_rgb_to_system_colors
                          common CODE, platform-specific RESULT through
                          get_system_color

    Stated, not converted to PASS: on SDL2/SDL3 the system-colour conversion
    is not TRUE32 today. Those backends are not built at COLOUR_DEPTH=32 in
    this laboratory (U4 finding), so the entry point is correct on every
    platform the candidate actually runs on, and wrong on two it does not yet
    build for. Recorded as SDL_SYSTEM_COLOUR_DEBT, not fixed - that is a
    backend change outside the three scoped functions (section 17).

## Pixel freeze

    previous framebuffer hashes unchanged: YES
        U8E image matrix: 19 of 19 hashes identical
        U8D nine-patch:    4 of 4 hashes identical, dirty still 4 / 6/6
        (both reran on the U9 candidate and diffed against the frozen
        U8E outputs)

## MULTI_THREAD

    default build:    PASS - all gates
    alternate build:  PASS - simgraph32.cc compiles clean with MULTI_THREAD
                      undefined (.text 97 792 vs 100 544); the probe object is
                      genuinely different (141 258 vs 145 841 bytes) and B1,
                      B2, B4, B5 pass in it
    THREADING_DEBT:   OPEN - untouched, unclaimed

## Cost

    simgraph32 .text before:  99 904
    after:                    100 544   (+640)
    calls before:             202
    after:                    214       (+12: 7 get_system_color, 2 in the
                                        screenshot writer, 3 raw_image_t)
    new recurring work in blitters: none. The seven conversions run at
    startup and on a settings change; the palette work runs only inside the
    rebuild that already existed; the screenshot runs only when asked.

## Memory

    persistent delta:      0
    screenshot temporary:  one raw_image_t of w*h*3 bytes for the clipped
                           area - 720 bytes for the 24x10 probe, ~7.1 MB for
                           a 3440x1440 screen - allocated by the historical
                           writer, which needs RGB888 rows, and freed on
                           return. Same as the legacy renderer.
    full-screen persistent buffer: NO

## EOL

    modified files:  simgraph32.cc, both trees
    EOL preserved:   LF
    mixed EOL:       NO - CR count 0, lone LF 3532

## Previous TRUE32 gates

    T1-T4:  PASS
    T5-T9:  PASS (+ PRECISION)
    zoom:   Z1-Z4, SENTINEL, SPECIAL, LIFECYCLE PASS; legacy zoom probe
            sha 32d9328552fee8000e75340ef2b905ea, unchanged since U8C
    U7B:    ORACLE, B1, B2, R1, O1, O2, PRECISION, TINT, ZOOMED PASS
    U7C:    DETECT, E1, DELEGATION, E2, E3 PASS
    U8A:    P1-P5, COVERAGE PASS
    U8B:    text probe 20 of 21 lines identical, the difference being the banner
    U8C:    GS1-GS5, LIFETIME PASS
    U8D:    GSURF1-5 PASS, hashes unchanged
    U8E:    ID1-ID4 PASS, all ten owners at dirty=4, hashes unchanged

## Legacy

    29 scenarios:   4a758dc6c6612f7be4d49a46a2825d6dec93a9799ed12884ea864806748a6b39
    LOW_LEVEL:      4a758dc6c6612f7be4d49a46a2825d6dec93a9799ed12884ea864806748a6b39
    302/302:        PASS, "Tests completed successfully", 0 error markers
    20/20:          PASS
    object identity notes:
                    16 bit lane 361 of 361 byte-identical to U8E - both
                    lanes were built on the same day, so the __DATE__
                    objects that differed in U8E coincide here.
                    32 bit lane 360 of 361, the one being simgraph32.o.

## Width/state guards

    STORED:    2
    SCREEN16:  2
    SCREEN32:  4
    SAVED:     4
    WIRE:      2
    flag bits: 0 inside SCREEN32
    0x73FE:    zoom_core.h byte-identical, 4 occurrences all in the zoom
               core, 0 in the added lines

## Formats

    pak:      unchanged
    makeobj:  unchanged
    save:     unchanged
    network:  unchanged
    MOTD:     r12247 fix intact

## Final API inventory

    corrected rule:  TOTAL 90   REAL 88   STUB 0   PARTIAL 2

    BACKEND_STUBS:    0 / 3
    BACKEND_PARTIAL:  0 / 3

    The two remaining partials, both outside this cut:
        get_color_rgb             GUI/palette read   legacy 12 lines
        calc_multiline_text_size  text measure       legacy 25 lines

    The old rule would have reported 90 / 90 / 0 / 0. It is not reported.

## Debt ledger

    IMAGE_DIRTY_DEBT:        CLOSED
    GRAPHICAL_SURFACE:       INCOMPLETE  - RETRACTED from U8E's COMPLETE:
                             one text partial and one GUI partial were hidden
                             from the inventory rule
    BACKEND_DEBT:            CLOSED
    SET_LIGHT_COLOR_DEBT:    CLOSED
    API_SURFACE:             INCOMPLETE  (88 of 90)
    THREADING_DEBT:          OPEN
    SDL_SYSTEM_COLOUR_DEBT:  OPEN  (new, recorded - SDL2/SDL3 get_system_color
                             is RGB565 regardless of COLOUR_DEPTH)

## Recommended next cut

Section 31's condition - API_SURFACE complete - is not met, so
THREADING-CLIPNUM-01 is not the recommendation yet. Recommended, in order:

    1. STLAB-SIMUTRANS-RGBA32-API-RESIDUAL-01
       get_color_rgb and calc_multiline_text_size. Two pure transpositions
       over already-real state (special_pal, display_day_lights, font_t).
       Small enough to be one short cut; without it the API count cannot be
       stated as complete.

    2. STLAB-SIMUTRANS-RGBA32-THREADING-CLIPNUM-01
       with a discovery phase FIRST: the per-thread clipping contexts, what
       CLIP_NUM actually indexes in simgraph16, which entry points are called
       concurrently, and whether CR32 can become an array without touching
       the shared traversal. That audit decides the cut's shape; it should
       not be guessed.

NOT executed.

## Safety

    exactly 3 backend functions only:  confirmed - and the two newly exposed
                                       partials were left alone, on purpose
    no threading implementation:       confirmed
    no CLIP_NUM redesign:              confirmed
    no CR32 redesign:                  confirmed
    no image changes:                  confirmed, hashes unchanged
    no GUI changes:                    confirmed
    no text/font changes:              confirmed
    no native RGBA pak:                confirmed
    no pak/makeobj changes:            confirmed
    no save/network changes:           confirmed
    mutants:                           disposable copy only (g/nc32), removed
    no SVN commit:                     confirmed
    no push:                           confirmed
    no publication:                    confirmed

## Provenance

    lane:      <lab>/_stlab-rgba32-u9
    base:      the certified U8E candidate, itself on SVN r12248
    reference: _stlab-rgba32-u8e read-only as the "before" tree
    harness:   u9_probe.cc - ONE source against BOTH renderers, expectations
               transcribed per format; decode_u9.py - PIL, independent of the
               renderer; nc_u9.sh - the light no-op mutant in a disposable
               copy; id_probe.cc and the nine-patch probe rerun for the pixel
               freeze; the carried-forward set for regression
    toolchain: MinGW-w64 g++ 16.1.0, mingw32-make; PIL 12.3.0 for decoding
    config:    BACKEND=gdi OSTYPE=mingw MSG_LEVEL=3 OPTIMISE=1 MULTI_THREAD=1,
               COLOUR_DEPTH 16 and 32; backend gates additionally run with
               MULTI_THREAD undefined
    processes: non-lab processes terminated: 0

### One probe defect, mine

The first B2 printout showed light 3 with the same value as light 2. The
comparison was on the PIXVALs and had passed; the PRINTOUT was wrong: a
four-slot rotating string buffer used six times in one printf. Widened to
eight, rerun, and the evidence above is from the corrected run. Noted because
a report is only as good as what it prints.
