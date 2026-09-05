# STLAB-SIMUTRANS-RGBA32-FOUNDATION-REBASE-01

## Result

    VERDICT:     A - RGBA32 FOUNDATION REBUILT ON SANITIZED TRUNK
    CONFIDENCE:  HIGH

One finding is reported prominently below rather than buried: reproducing the
r12244 laboratory on r12248 showed that its "ARGB8888 palette" **was not
ARGB8888**. Three bounded changes were needed to make it real, and the probe is
what caught it.

## Baseline

    live HEAD:  r12248, <svn-author>, 2026-09-04 07:33:58 +0200
    expected:   r12248 - matches. r12249 does not exist.
    lane:       <lab>/_stlab-rgba32-u4
                base/   official r12248 export, untouched
                cand/   official r12248 export + U4, built at COLOUR_DEPTH=16
                cand32/ the same source, built at COLOUR_DEPTH=32

## Previous evidence reused

    RGBA32-02:  the 32 bit renderer skeleton, the build wiring and the
                conditional type model
    RGBA32-03:  the ARGB8888 palette construction and simgraph_palette.h

What was reproduced:

  * `simgraph32.cc` / `simgraph32.h` / `simgraph_palette.h` - new files that
    never existed in trunk, carried over as the architectural evidence they are
  * the conditional flag relocation and PIXVAL / FLAGGED_PIXVAL widths
  * the renderer dispatch and the COLOUR_DEPTH build selection

What was NOT mechanically reused - because r12245-r12248 already did it:

  * `image.h`, `image.cc`, `image_reader.cc` type changes: r12245 retyped
    `image_t::data` and `get_data()` to `STORED_PIXVAL`, so nothing was needed
    in the headers at all
  * `gameinfo.h` and `server_frame.cc`: the prototype had to retype the
    thumbnail array and convert in the GUI. On r12248 the thumbnail stays a
    screen colour and converts at the wire boundary r12248 itself created, so
    both files are untouched
  * `RGB565VAL`: not reintroduced. Trunk already has `NETWORK_PIXVAL` for
    exactly this job, and the new conversions are named after it
  * the prototype's own `SAVED_PIXVAL` definition: trunk owns it since r12248

The old cumulative laboratory diff was 17 files. This one is 12, and the five
that dropped out are precisely the ones the sanitation fixed.

## Canonical widths

    STORED:   16 bit  (uint16, static_assert, unchanged in BOTH lanes)
    SCREEN16: 16 bit  RGB565
    SCREEN32: 32 bit  ARGB8888
    SAVED:    32 bit  (unchanged)
    WIRE:     16 bit  (unchanged)
    flags reserved inside SCREEN32: ZERO

Measured by the probe in the 32 bit lane:

    widths: STORED=2 SCREEN=4 FLAGGED=8 SAVED=4 NETWORK=2
    all-ones screen colour collides with a draw flag: no
    get_flagged_colour(all-ones) returns all-ones

## Architecture

    legacy .pak image                stays 16 bit, byte for byte
        |
        v   STORED_PIXVAL (uint16)
    semantic decode / palette        rgbmap_current[], specialcolormap[]
        |
        v
    screen colour                    RGB565 (16 bit lane)
                                     ARGB8888 (32 bit lane)
        |
        +---> framebuffer            2 or 4 bytes per pixel
        |
        +---> SAVED_PIXVAL   32 bit, converted at simmesg.cc
        |
        +---> NETWORK_PIXVAL 16 bit, converted at gameinfo.cc

The draw flags sit above the colour in both lanes: at bits 19-23 of a 32 bit
word when the colour is 16 bit, and 16 bits higher in a 64 bit word when the
colour is 32 bit. The U2 accessors hide that entirely - not one caller of
`get_flagged_colour` had to change.

## THE FINDING: the prototype's palette was not ARGB8888

The first run of the foundation probe on the faithfully reproduced prototype
gave this in the 32 bit lane:

    ordinary red   0x7C00 -> 0x0000F800   (r 0 g 0 b 0)
    white          0x7FFF -> 0x0000FFFF   (r 0 g 0 b 0)
    rgb888 in  r 18 g 52 b 86
    screen     0x000011AA (32 bit)
    rgb888 out r  0 g  0 b  0

Three separate causes, all real, none of them a harness artefact:

1. **`get_system_color()` is not renderer code.** It is a free function defined
   independently in `simsys_w.cc`, `simsys_s2.cc`, `simsys_s3.cc` and
   `simsys_posix.cc`, and every copy hardcodes the RGB565 formula.
   `calc_base_pal_from_night_shift()` in simgraph32.cc builds the *entire*
   palette by calling it, so the whole "ARGB8888 palette" was RGB565 values
   sitting in 32 bit slots. The screen-colour constructor is owned by the
   system layer, not the renderer, and is duplicated four times.

2. **`simgraph32_palette_lookup()` returned the index unchanged** - a stub. It
   never consulted `specialcolormap_all_day`, so player and light colours came
   back as raw indices.

3. **`simgraph32_get_pixval_rgb()` returned `{0,0,0}`** - also a stub, so no
   colour could be read back at all.

All three sit squarely in section 4.B, "renderer/table support for ARGB8888",
so they were fixed inside this cut. After the fix, the same probe gives:

    ordinary red   0x7C00 -> 0xFFFF0000   (r255 g  0 b  0)
    white          0x7FFF -> 0xFFFFFFFF   (r255 g255 b255)
    rgb888 in  r 18 g 52 b 86
    screen     0xFF123456 (32 bit)
    rgb888 out r 18 g 52 b 86
    round trip exact: yes

`get_system_color` was made depth-aware in the **GDI backend only**, which is
the backend this lane builds. Section 27 forbids touching SDL2/SDL3, so
`simsys_s2.cc`, `simsys_s3.cc` and `simsys_posix.cc` keep the RGB565 formula
and are recorded as deferred: the identical one-line change is required there
before a 32 bit SDL or headless lane can be correct. In the 16 bit lane all
four are unchanged in behaviour.

## Production changes

    files:         12   (3 new, 9 modified)
    new files:     src/simutrans/display/simgraph32.cc    1037 lines
                   src/simutrans/display/simgraph32.h       16 lines
                   src/simutrans/display/simgraph_palette.h 104 lines
    modified:      9
    hunks:         22
    +/-:           +147 / -32   (excluding the three new files)

| file | what |
|---|---|
| `simcolor.h` | conditional flag positions, conditional PIXVAL/FLAGGED_PIXVAL, conditional colour mask, and the SCREEN↔SAVE and SCREEN↔WIRE conversions |
| `simmesg.cc` | the save boundary r12248 named now converts instead of casting |
| `dataobj/gameinfo.cc` | the wire boundary r12248 named now converts instead of casting |
| `descriptor/ground_desc.cc` | 7 image-data pointers/values retyped to `STORED_PIXVAL` |
| `descriptor/reader/image_reader.cc` | 2 image-data pointers retyped to `STORED_PIXVAL` |
| `display/simgraph.h` | `SIMGRAPH_TYPE_SOFTWARE32` |
| `display/simgraph.cc` | dispatch to the 32 bit renderer |
| `sys/simsys.h` | framebuffer is `PIXVAL*`, not `unsigned short*` |
| `sys/simsys_w.cc` | framebuffer type, and the depth-aware screen-colour constructor |
| `Makefile` | `COLOUR_DEPTH` is validated and overridable; defaults unchanged |

Shared files modified: 9. Duplicated logic: the palette (see below).

### The mixed-space pointers the compiler found

`ground_desc.cc` and `image_reader.cc` still spelled image-data pointers as
`PIXVAL*` even though r12245 retyped the source of those pointers to
`STORED_PIXVAL*`. While both are 16 bit that is invisible; at COLOUR_DEPTH 32
it is a hard compile error, and the build produced exactly four of them:

    cannot convert 'STORED_PIXVAL*' to 'PIXVAL*' in initialization

That is section 6 working as intended: the widened screen type turns every
remaining mixed-space pointer into a diagnostic instead of a silent bug. No
generic mixed-space `PIXVAL*` was reintroduced anywhere.

## Legacy renderer

    simgraph16 source unchanged: YES   (byte-identical to official r12248)

    build:     0 errors, 361 objects, sim.exe 10 264 252 bytes
    warnings:  50 in 11 categories, warning SET identical to official r12248
    object identity: 0 of 361 objects differ from the official r12248 build

Every U4 edit is the identity at COLOUR_DEPTH 16 - the conditionals collapse,
the inline conversions compile away, and the `STORED_PIXVAL` retypes name the
same type - so the legacy product build is byte-identical, object for object.

## 32-bit foundation

    compile:              PASS, 0 errors
    link:                 PASS
    renderer selection:   simgraph32.o present, simgraph16.o absent
    executable:           build/default/sim.exe, 10 177 673 bytes
    screen pixel size:    4 bytes
    framebuffer:          4 bytes per pixel
    warnings:             60 in 12 categories - the 10 extra are 9
                          -Wunused-parameter and 1 -Wunused-function, all from
                          simgraph32.cc's own fail-closed stubs

    No runtime rendering certification is claimed. Section 20 forbids it and
    nothing here would support it: the image drawing API is deliberately absent.

## Recode vectors

Measured through the production renderer of each lane. RGB555 values enter via
`get_system_color`; the semantic indices are read through the production
`palette_lookup`, and every result is decoded back with `get_pixval_rgb`.

| stored | semantic class | legacy RGB565 | SCREEN32 ARGB8888 |
|---|---|---|---|
| 0x0000 | RGB555 black | 0x0000 | 0xFF000000 (r0 g0 b0) |
| 0x7C00 | RGB555 red | 0xF800 | 0xFFFF0000 (r255 g0 b0) |
| 0x03E0 | RGB555 green | 0x07E0 | 0xFF00FF00 (r0 g255 b0) |
| 0x001F | RGB555 blue | 0x001F | 0xFF0000FF (r0 g0 b255) |
| 0x7FFF | RGB555 white | 0xFFFF | 0xFFFFFFFF (r255 g255 b255) |
| 0x8000 | player colour 0 | index | 0xFF244B67 (r36 g75 b103) |
| 0x8007 | player colour 7 | index | 0xFFB0D2FF (r176 g210 b255) |
| 0x8010 | light 0 | index | 0xFF113785 (r17 g55 b133) |
| 0x8015 | light 5 | index | 0xFF4585DC (r69 g133 b220) |
| 0x8020 | class+alpha | index | 0xFF56200E (r86 g32 b14) |
| 0x83FF | class+alpha | index | 0x00000000 |

Two honest qualifications on this table:

  * The 16 bit column reads "index" for the semantic rows because in that lane
    `palette_lookup` returns a 16 bit screen colour whose value is not
    meaningful to print beside the 32 bit one; what matters is that both lanes
    classify the same stored value into the same semantic space.
  * The last row is `specialcolormap_all_day[0xFF]`, which the production code
    deliberately zeroes for indices beyond the special palette. The true
    class+alpha stored space (>= 0x8020) maps through `rgbmap_current[]` inside
    `recode_img_src_target`, which is file-static. That table is now filled
    with ARGB8888 by the same construction, but it is only reachable from the
    blitters, so exercising it end to end belongs to U5. This is stated rather
    than claimed.

## Precision

    full-precision example: rgb888 (18, 52, 86)
    RGB565 legacy:          screen 0x11AA -> back as (16, 52, 82)
    ARGB8888:               screen 0xFF123456 -> back as (18, 52, 86)

    what additional precision survives: all three channels, exactly. The 16 bit
    path loses 2 levels of red and 4 of blue on this value; the 32 bit path
    loses nothing. Historical day/night semantics are untouched - the same
    `calc_base_pal_from_night_shift` arithmetic runs in both lanes, only the
    final store keeps its precision instead of quantising.

## Storage

    STORED_PIXVAL size:  2 bytes in BOTH lanes, guarded by static_assert
    base_data widened:   NO
    pak format:          unchanged
    makeobj:             unchanged (src/makeobj byte-identical)
    image.h / image.cc:  unchanged
    image_writer.cc:     unchanged

## Persistence/wire guards

    SAVED_PIXVAL:    uint32, static_assert, unchanged
    save format:     unchanged - simmesg.cc converts at the boundary and is the
                     identity at COLOUR_DEPTH 16
    NETWORK_PIXVAL:  uint16, static_assert, unchanged
    wire width:      2 bytes per minimap pixel, unchanged
    protocol:        simversion.h, network_cmd.cc, network_cmd_ingame.cc all
                     byte-identical

## Closed-debt guards

    TD-01 / TD-01b:  STORED_PIXVAL x2, static_assert present, 13
                     colorpixcopy_*_screen/_stored references, 0 ambiguous
                     `colorpixcopy(`
    TD-02 / TD-14:   11 accessor uses, 0 raw `color_index & 0xFFFF`, 0 raw
                     `get_titlecolor()&0xFFFF`, background_visible x6,
                     0 chart sentinel
    TD-13:           simgraph16.cc byte-identical - no blend family touched
    TD-03:           SAVED_PIXVAL and its static_assert intact, used in simmesg
    TD-04:           NETWORK_PIXVAL and its static_assert intact, used in
                     gameinfo
    MOTD fix:        1/1/1, intact

    No closed boundary regressed.

## Foundation probe

    16 bit lane: PASS (0 failures) - the product control
    32 bit lane: PASS (0 failures)

## Negative control

    mutation:  every recoded screen colour pushed through an RGB565 round trip
               before it is reported, in a disposable build of the 32 bit lane
    expected:  the gate detects the loss of full-precision ARGB8888
    actual:    FAIL. rgb888 (18,52,86) comes back as (16,52,82) and the probe
               reports "32 bit screen lost channel precision".
               Note that red and white still print as 0xFFFF0000 and
               0xFFFFFFFF under the mutation - those values survive
               quantisation, which is exactly why the gate is anchored on a
               value that does not.
    restored:  yes, the mutation lives only in fp-nc.exe

## Regression

    302/302:        PASS - "Tests completed successfully.", 0 error blocks,
                    run on the 16 bit lane, which is the product control.
                    Suite provenance verified: 34 files, test_convoy_state.nut
                    present, pak-local copy stripped, markers judged only
                    after the real banner.
    20/20 blends:   PASS, 0 failures

## Memory

    framebuffer:              2 bytes/pixel (16 bit) -> 4 bytes/pixel (32 bit)
    1024x768 frame:           1 572 864 -> 3 145 728 bytes
    stored image:             2 bytes/pixel in both lanes, unchanged
    representative recode cache: 4 bytes/pixel in the 32 bit lane
    64x64 gameinfo thumbnail: 8 192 -> 16 384 bytes in memory,
                              8 192 bytes on the wire in both

    This confirms the audit conclusion rather than contradicting it: the
    framebuffer doubles by 1.5 MB, which is nothing, while every recoded image
    cache doubles. No optimisation was attempted, as directed.

## Duplication

    palette:              YES - simgraph32.cc reproduces simgraph16's palette
                          machinery (calc_base_pal_from_night_shift,
                          rgbmap/specialcolormap tables, player offsets)
    other:                `get_system_color` is duplicated FOUR times across
                          the backends, and this cut had to make one of those
                          copies depth-aware. That duplication is pre-existing,
                          not created here, but it is now load-bearing for the
                          32 bit lane.
    shared-core trigger reached: NO, but closer than before. The palette is one
                          duplicated subsystem; clipping geometry returns in
                          U5/U9 and would be the second created by this
                          programme. The `get_system_color` split is an
                          argument for revisiting the trigger early.

## Deferred

    nc/wc/pc:          U5, not started
    colored/daytime:   U5
    zoom:              not touched
    text/GUI:          not touched
    threading:         not solved, not claimed. The 32 bit lane makes no
                       thread-safety claim; clipping context belongs to U5/U9
    backends:          simsys_s2.cc, simsys_s3.cc and simsys_posix.cc keep the
                       RGB565 `get_system_color` and the `unsigned short*`
                       framebuffer signature. Both are one-line changes and
                       both are required before a 32 bit SDL or headless lane
                       is correct. Untouched here by section 27.
    performance:       not measured, not claimed
    class+alpha recode end-to-end: reachable only through the static
                       `recode_img_src_target`, exercised by the blitters in U5

## Next cut

If A:

    STLAB-SIMUTRANS-RGBA32-BLITTER-REBASE-01
    U5 - nc / wc / pc

NOT executed here.

## Safety

    no U5 blitters:              confirmed
    no colored/daytime:          confirmed
    no PIXVAL storage widening:  confirmed, static_assert in both lanes
    no savegame change:          confirmed
    no network change:           confirmed
    no flag regression:          confirmed
    no MOTD change:              confirmed
    no pak/makeobj change:       confirmed
    no SVN commit:               confirmed
    no push:                     confirmed
    no publication:              confirmed

## Provenance

    source:      svn://servers.simutrans.org/simutrans/trunk @ r12248
    base:        _stlab-rgba32-u4/base    untouched official export
    cand:        _stlab-rgba32-u4/cand    U4, built at COLOUR_DEPTH=16
    cand32:      _stlab-rgba32-u4/cand32  same source, COLOUR_DEPTH=32
    prototype:   _stlab-rgba32-03-cand    the r12244 evidence
    probe:       foundation_probe.cc, linked against the 360 production objects
                 of the lane under test; fp-nc.exe is the negative control
    pakset:      pak64 124.4 r2223, 815 entries
    toolchain:   MinGW-w64 g++ 16.1.0, mingw32-make
    config:      BACKEND=gdi OSTYPE=mingw MSG_LEVEL=3 OPTIMISE=1 MULTI_THREAD=1
    processes:   non-lab processes terminated: 0
