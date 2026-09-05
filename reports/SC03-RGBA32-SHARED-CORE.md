# STLAB-SIMUTRANS-RGBA32-SHARED-CORE-SC03

## Result

    VERDICT:     A - SHARED NC/WC/PC CORE COMPLETE
    CONFIDENCE:  HIGH

Both renderers now decode image runs, walk images and intersect spans through
one implementation, for all three blitters. Every certified hash is unmoved,
the historical LOW_LEVEL asymmetry is preserved by construction rather than by
discipline, and the MULTI_THREAD no-op trap is closed functionally, not just by
a successful compile.

A note on the A/B boundary, so the judgement is visible rather than buried:
reaching A required giving each renderer a **second pixel policy** for the
clipped blitters. Section 5 of the brief anticipates exactly this ("expose
separate policy operations... names are not mandated, semantics are"), and
"LOW_LEVEL asymmetry preserved" is an A criterion, so this is the means by
which A is met, not a deviation from it. Everything a reader would need to
call it B instead is stated below.

## Baseline

    SVN:    r12248
    U4:     TRUE ARGB8888 foundation
    U5:     nc / wc / pc + class+alpha
    SC-01:  shared run decoding / traversal
    SC-02:  shared clipping geometry

Candidate identity verified on this lane BEFORE any edit - not inherited from
the SC-02 report:

    simgraph16.cc     1028260a39b71650b5f3b54efe7f8261
    simgraph32.cc     142fb3fa4e4d29b74d391ee32e2ea257
    blit_core.h       6df62a41f4ebb3cde2e0d306f1222674
    blit_clip_core.h  bd627098336fe34a7a6902c13647228e

    T1 0xDA18C545  T2 0x19D5EC05  T3 0x0F526995  T4 0x7B4F5C85
    legacy16 4a758dc6...  (default and -DLOW_LEVEL)

## SC-03 diff

    files:   3 modified, 0 new
    hunks:   blit_core.h    1
             simgraph16.cc  2
             simgraph32.cc  2
    +/-:     blit_core.h    +77 /  -0
             simgraph16.cc  +51 / -75
             simgraph32.cc  +33 / -62

Both renderers again lost more lines than they gained. No unrelated
formatting; `blit_clip_core.h` and `clip_num.h` are byte-identical to SC-02.

## Final shared architecture

    shared traversal:  blit_core.h
                         blit_core_nc<Ops>        unclipped
                         blit_core_clipped<Ops,Span>  wc and pc
                       Run decoding, the clear-run step, the TRANSPARENT_RUN
                       test, span intersection, destination stepping and pitch
                       all live here once.

    shared clipping:   blit_clip_core.h, unchanged from SC-02
                       xrange, clip_line_t, the two span helpers

    PixelOps16:        RGB565, nc only. Carries the LOW_LEVEL unrolled
                       two-pixels-per-uint32 copy.
    PixelOps16c<r>:    RGB565, wc and pc. screen_t = PIXVAL,
                       source_t = pixcopy_src<r>, so plain reads the recoded
                       screen cache and colored/daytime read stored data.
                       Copy is the plain pixcopy() these blitters have always
                       used.
    PixelOps32:        ARGB8888, nc only. Keeps the -DNC_POLICY16 control
                       branch, so that control still means what it meant.
    PixelOps32c:       ARGB8888, wc and pc.

    renderer-local state:
                       simgraph16  clipping_info_t clips[], selected by
                                   CLIP_NUM; CR unchanged
                       simgraph32  the single CR32 context, unchanged
                       The shared core never sees a clipping_info_t, a
                       CLIP_NUM or a clip rectangle. It asks a caller-supplied
                       span object for each row: span_fixed_t for the clip
                       rectangle, span_poly16_t / span_poly32_t (defined inside
                       their own renderers) for the polygon case.

## LOW_LEVEL semantics

    nc:       PixelOps16::copy - the unrolled copy, with all three inner
              guards (LOW_LEVEL / SIM_BIG_ENDIAN / _MSC_VER)
    wc:       PixelOps16c<plain>::copy  -> templated_pixcopy<plain> -> pixcopy()
    pc:       PixelOps16c<r>::copy      -> templated_pixcopy<r>
    changed:  NO

This is not asserted from reading the source. Two mutants, each corrupting one
policy in a disposable tree, show the two paths are genuinely disjoint:

    corrupt PixelOps16::copy   (nc policy)     -> scenarios touched: nc
    corrupt PixelOps16c<>::copy (wc/pc policy) -> scenarios touched:
                                                  wc, pc_plain, pc_colored,
                                                  pc_daytime

Neither mutant reaches the other's scenarios. wc and pc cannot silently acquire
the unrolled copy, because it is not in their policy at all.

## TRUE32

### T1 nc
    hash:      0xDA18C545
    pixels:    32
    PASS/FAIL: PASS

### T2 wc
    hash:           0x19D5EC05
    pixels:         16
    outside writes: 0
    inside wrong:   0
    PASS/FAIL:      PASS

### T3 pc
    hash:              0x0F526995
    pixels:            16
    exact shape:       y=2  .....#######....................
                       y=3  .......#####....................
                       y=4  .........###....................
                       y=5  ...........#....................
    exact pixel set:   identical
    T3 != T2:          true (secondary guard only)
    PASS/FAIL:         PASS

### T4 class+alpha
    hash:      0x7B4F5C85
    result:    18 pixels, wrong=0, opaque-alpha=yes,
               stored 0x808C -> cache 0x0000BACB -> 0xFF406080
    PASS/FAIL: PASS

    Whole probe output before/after: IDENTICAL, byte for byte.

## Polygon canonical NC

    mutation:        clip_line_t::inc_y, r_xmin = (r.sx >> 16) + 1
                     (the canonical clipping NC established in SC-02),
                     applied in a disposable copy of the tree
    correct hash:    0x0F526995
    mutant hash:     0xEAB18E15
    correct pixels:  16
    mutant pixels:   12
    T3 != T2 still true under mutant:  YES
    exact gate caught mutation:        YES

The correction from SC-02 holds against the migrated pc: the probe still
printed its `differs from wc` verdict under the mutant. Only the exact hash and
pixel set caught it. pc is certified here on the hash, the pixel set and the
shape - never on the inequality.

## MULTI_THREAD trap

    branch 1 (MULTI_THREAD):     simgraph32_add_poly_clip / clear_all_poly_clip
                                 / activate_ribi_clip - real bodies, all three
                                 write CR32
    branch 2 (no MULTI_THREAD):  the same three - real bodies, same logic,
                                 same CR32 writes
    both real:                   YES
    no-op risk:                  CLOSED

Closed functionally, not by inspection alone. The probe hard-coded the
threaded call shape, so it was rebuilt through the CLIP_NUM macros and run in
both configurations:

    with MULTI_THREAD:     T1 0xDA18C545  T2 0x19D5EC05  T3 0x0F526995  T4 0x7B4F5C85
    without MULTI_THREAD:  T1 0xDA18C545  T2 0x19D5EC05  T3 0x0F526995  T4 0x7B4F5C85

The MULTI_THREAD run matches the certified figures, so the reshaped probe is
faithful; the non-threaded run then proves the second branch does not merely
compile, it draws, and it draws the same pixels. Both renderers also compile
clean in the alternate configuration:

    simgraph32.cc without MULTI_THREAD:  COMPILES, 9 warnings (unchanged), .text 7296
    simgraph16.cc without MULTI_THREAD:  COMPILES, 0 warnings (unchanged), .text 78400

That last one matters for this cut in particular: span_poly16_t is brace-
initialised with CLIP_NUM_VAR, which is an empty token without MULTI_THREAD.

## Legacy16

    29 scenarios:     certified set, same recipe on both trees
    default:          4a758dc6c6612f7be4d49a46a2825d6dec93a9799ed12884ea864806748a6b39
    LOW_LEVEL:        4a758dc6c6612f7be4d49a46a2825d6dec93a9799ed12884ea864806748a6b39
    byte differences: 0, whole file, 220 lines, in both builds

The two builds agreeing is the load-bearing part: SC-03 did not extend
LOW_LEVEL semantics to the clipped blitters, and if it had, the -DLOW_LEVEL
dump is where it would show.

## Type safety

    policy swap:       standalone translation unit, blit_core_clipped
    compile:           HARD COMPILE ERROR, twice, on two independent axes
    expected failure:  yes

    SCREEN16 policy pointed at an ARGB8888 framebuffer:
        no matching function for call to
        'blit_core_clipped<Ops16>(int, uint32[64], const uint16[8], ...)'
    SCREEN32 policy fed a 16 bit run stream:
        no matching function for call to
        'blit_core_clipped<Ops32>(int, uint32[64], const uint16[8], ...)'

    Control build with matched types compiles, so the failures are the type
    discipline and not a broken test. screen_t and source_t are checked
    separately - the second error is a source-space mismatch with a correct
    framebuffer. No void*, no PIXVAL* reinterpretation, no width-based cast.

## Hot-path

### simgraph16.o
    .text before:  79 488 bytes
    .text after:   79 680 bytes    (+192)
    calls before:  248
    calls after:   249             (+1)

### simgraph32.o
    .text before:   7 360 bytes
    .text after:    7 296 bytes    (-64)
    calls before:   25
    calls after:    25             (+0)

    dynamic per-pixel dispatch: NO

The +1 call was identified, not waved through. The set of distinct call targets
in simgraph16.o is unchanged at 143 before and after; the only difference is
that `recode_img` is now called from 7 sites instead of 6. `recode_img` takes a
mutex and rebuilds one image's screen cache; it is called once per image from
the image-registration paths, never from a blitter and never per pixel. Nothing
became a function pointer and no new call target appeared anywhere.

Measurement note carried forward: these blitters are `static` and carry no
symbol, so counting them by name in a disassembly returns zero and means
nothing. The figures above are whole-object `.text` size and total
call-instruction count, and the rig was proven in SC-02 by recompiling the
renderer standalone and getting a byte-identical object.

Cumulative across the shared-core cuts, for context:

    simgraph16.o  SC-01 79 104 -> SC-02 79 488 -> SC-03 79 680   (+576)
    simgraph32.o  SC-01  8 320 -> SC-02  7 360 -> SC-03  7 296   (-1 024)

## Builds

    16:                    compile PASS, link PASS, 0 errors, 361 objects,
                           sim.exe built
    32:                    compile PASS, link PASS, 0 errors, 361 objects,
                           sim.exe built
    alternate MULTI_THREAD: compile PASS, both renderers
    warnings:              clean, non-incremental builds after `make clean`
                             16-bit  50 in 11 categories - SET identical to
                                     SC-02, SC-01 and official r12248
                             32-bit  60 in 12 categories - SET identical to
                                     SC-02, SC-01 and U5
                           Nothing silenced, no unrelated warning touched.

    Object identity: 360 of 361 objects byte-identical in each lane; the one
    that differs is the renderer whose source changed. blit_core.h is included
    only by the two renderers, so it forced no unrelated rebuild.

## Regression

    302/302:  PASS - 302 numbered tests, "Tests completed successfully.",
              no error marker after the banner
    20/20:    PASS - canonical blend vectors, 0 failures

## Width guards

    STORED:     2   static_assert present
    SCREEN16:   2
    SCREEN32:   4
    SAVED:      4   static_assert present
    WIRE:       2   static_assert present
    flag bits:  0   draw flags live above the colour (shifted <<16 at 32 bit),
                    so no colour bit is reserved and the all-ones screen colour
                    still collides with no flag
    (measured by the production probe: STORED=2 SCREEN=4 SAVED=4 NETWORK=2)

## Formats

    pak:      unchanged
    makeobj:  unchanged - src/makeobj byte-identical to official r12248
    save:     unchanged - simversion.h, loadsave.cc, objversion.h,
              image_writer.cc, simimg.h, image.h, image.cc byte-identical
    network:  unchanged - network_cmd.cc, network_cmd_ingame.cc, gameinfo.h
              byte-identical
    MOTD:     r12247 fix intact

## Threading

    clip_num parity:  UNCHANGED
    status:           simgraph16 keeps per-thread clipping state selected by
                      CLIP_NUM; simgraph32 keeps its single CR32 context and
                      still accepts and ignores the CLIP_NUM parameter. That
                      limitation is unchanged and is NOT claimed to be fixed:
                      no thread safety is claimed for simgraph32.

    clip_num.h:       byte-identical
    CLIP_NUM_* uses in simgraph16.cc: 134 -> 135. The single new use is one
    CLIP_NUM_VAR - the clip number handed to span_poly16_t. Every other macro
    use count is identical; the shape of the plumbing did not change.

## Duplication after SC-03

    run decoding:        SINGLE SOURCE. The idiom `runlen & ~TRANSPARENT_RUN`
                         now appears 0 times in simgraph32.cc (was 2) and 2
                         times in blit_core.h (was 1: nc, now nc + clipped).
    traversal:           SINGLE SOURCE for nc, wc and pc, both renderers.
    rectangle geometry:  shared shape (span_fixed_t), renderer-local rectangle.
    polygon geometry:    SINGLE SOURCE, blit_clip_core.h, unchanged.
    pixel semantics:     deliberately separate - PixelOps16 / PixelOps16c<r> /
                         PixelOps32 / PixelOps32c.

    Four occurrences of the decode idiom remain inside simgraph16.cc. Named
    rather than hidden: recode_img_src_target, the zoom path, the blend/alpha
    image family and display_color_img_wc. None of them is duplication BETWEEN
    the renderers - they are legacy-only functions with no 32 bit counterpart
    yet. Chasing them was out of scope here and would not reduce cross-renderer
    duplication, only file size.

    File sizes:  simgraph16.cc 4 843 -> 4 819
                 simgraph32.cc 1 341 -> 1 312
                 blit_core.h      88 ->   165
                 blit_clip_core.h 216 (unchanged)

## Shared-core status

    SHARED_CORE_NC_WC_PC:  COMPLETE

    nc shared traversal              yes
    wc shared traversal              yes
    pc shared traversal              yes
    shared clipping geometry         yes
    separate pixel semantics         yes
    separate renderer clipping state yes

## U6 readiness

    READY

Reason, and it is demonstrated rather than predicted: `blit_core_clipped` is
already parameterised on `Ops::source_t` **separately from** `Ops::screen_t`,
and the 16 bit renderer already drives three different source semantics through
it - `plain` reading a recoded screen cache, `colored` and `daytime` reading the
original stored image data. The NC-B mutant above proves those three really do
run through the shared traversal today. So adding colored/daytime to the 32 bit
renderer is writing two more policies with the same shape as PixelOps32c, not
building a second image walker. The blend-family contracts from FOUNDATION-00B
stay where they are, in the pixel policies.

## Next cut

    STLAB-SIMUTRANS-RGBA32-COLORED-DAYTIME-01

    scope: port colored/daytime semantics onto the shared core, preserve the
    FOUNDATION-00B blend-family contracts, no new traversal duplication.

NOT executed here.

## Safety

    wc/pc migration only:      confirmed
    LOW_LEVEL asymmetry:       preserved, and proven by two disjoint mutants
    no clipping-state merge:   confirmed - CR, CR32 and clip_num.h untouched
    no clipping geometry change: confirmed - blit_clip_core.h byte-identical
    no optimization change:    confirmed - no unrolling added, no batching
                               altered, no SIMD, no alpha arithmetic touched,
                               no branch reordered for speed
    no threading redesign:     confirmed
    no colored/daytime cut:    confirmed - the existing 16 bit routines were
                               migrated, no new colour semantics were written
    no zoom:                   confirmed
    no palette refactor:       confirmed
    no backend normalization:  confirmed
    no save/network change:    confirmed
    no pak/makeobj change:     confirmed
    mutants:                   disposable copies only, removed after use
    no SVN commit:             confirmed
    no push:                   confirmed
    no publication:            confirmed

## One behavioural difference, stated explicitly

The shared clipped traversal carries pc's guard, `xmin < xmax && ...`. The old
`display_img_wc` did not have that first test, because for the clip rectangle
it is normally implied. It cannot change any output: `pixcopy` and
`colorpixcopy_screen` are both `while (src < end)` loops, so on an empty
rectangle the old code computed a `src > end` pair and wrote nothing, exactly
what the guard now short-circuits. Every wc gate above - T2 and the legacy16
wc scenarios - is byte-identical, and the guard is one comparison per run, not
per pixel.

## Provenance

    lane:      <lab>/_stlab-rgba32-sc03
    base:      the certified SC-02 candidate, itself on SVN r12248
    reference: _stlab-rgba32-sc02 read-only as the "before" tree;
               _stlab-rgba32-u5/base (official r12248) for the format guards
    harnesses: legacy_preserve.cc (29 scenarios), built twice per tree -
               default and -DLOW_LEVEL; blitter_probe.cc (T1-T4) linked against
               359 production objects, built in both MULTI_THREAD
               configurations; three mutants (nc policy, clipped policy,
               canonical polygon); a standalone type-swap translation unit;
               vector_gate.cc (20 blend vectors)
    toolchain: MinGW-w64 g++ 16.1.0, mingw32-make
    config:    BACKEND=gdi OSTYPE=mingw MSG_LEVEL=3 OPTIMISE=1 MULTI_THREAD=1,
               COLOUR_DEPTH 16 and 32, plus a no-MULTI_THREAD control
    processes: non-lab processes terminated: 0
