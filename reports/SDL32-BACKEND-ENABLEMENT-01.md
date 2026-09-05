# STLAB-SIMUTRANS-RGBA32-SDL32-BACKEND-ENABLEMENT-01

## Verdict

    B - COMPLETE WITH BOUNDED HISTORICAL ADAPTATION
    CONFIDENCE: HIGH

    SDL32_BACKEND_DEBT     = CLOSED
    SDL_SYSTEM_COLOUR_DEBT = CLOSED
    KNOWN_RENDERER_DEBT    = NONE

Both SDL backends now build, start a real window, hand the renderer an
ARGB8888 framebuffer, present it through both production upload paths with
exact 32-bit identity (read back through SDL's render target and, for the
initial frame, through the Windows compositor), survive a resize, return exact
system colours, and shut down - at COLOUR_DEPTH=32. At COLOUR_DEPTH=16 the two
backend objects are BYTE-IDENTICAL to the pristine ones.

B rather than A for one adaptation that is not a representation change:
SDL2's `dr_textur_init()` has always locked its streaming texture and thrown
the lock's pixel pointer away, handing out the surface instead; `dr_flush()`
then uploads from the surface with `SDL_UpdateTexture` while the texture is
still locked. D3D9 - the driver SDL2 picks by default on Windows - tolerates
that at RGB565 and shows a black frame at ARGB8888, measured through the
compositor. Under COLOUR_DEPTH==32 the lock is simply not taken; the 16-bit
text is verbatim. It is documented in the source and below.

## Base

    live HEAD:        r12253 - moved from r12251 during the programme.
    upstream overlap: r12252 (pakset installer HTTP->HTTPS redirects) and
                      r12253 (pakset download shell parsing) touch
                      pakset_downloader.cc and network_file_transfer.* only.
                      No renderer, backend or colour code. No base review.
    lane:             <lab>/_stlab-rgba32-sdl2, fresh copy of
                      the SDL-01 lane (itself the U10 candidate)
    identity before editing:
                      simgraph32.cc  918f4aa9a1ea203223d47e787e45cf2c
                      simsys_w.cc    fe268d60b0c515306d49718cd4fb83ee
                      simsys_s2.cc   c902cc36db8c9347ce3cbcb3c9e28ef9
                      simsys_s3.cc   4cdb81d7374070f1a92c8f2453211f8e
                      simsys.h       e9a0ae6210381e4f5f2b9e9c17e52f61
    inventory:        90 / 90 / 0 / 0

## Scope

    files:    2 - simsys_s2.cc (+35 / -6, 5 hunks), simsys_s3.cc (+30 / -8, 6 hunks)
    hunks:    11
    +/-:      +65 / -14
    headers:  none touched - simsys.h already carried the PIXVAL* contract (U4)
    renderer: simgraph32.cc, simsys_w.cc, simsys.h byte-identical to U10
    EOL:      both files CRLF, preserved (0 lone LF)

    Per backend, the four coupled items and nothing else:
      1. texture format     RGB565 at 16 / ARGB8888 at 32
      2. surface + check    the alpha mask the format must carry: 0 at 16,
                            0xFF000000 at 32; the 16-bit check line verbatim
      3. framebuffer type   PIXVAL* dr_textur_init(), dr_textur_resize(PIXVAL**)
      4. get_system_color   at 32 the expression simsys_w.cc uses, so all
                            three backends agree bit for bit; at 16 the
                            SDL_MapRGB path untouched
    plus, SDL2 only, item 3b: no texture lock under COLOUR_DEPTH==32.

## SDL2

    16-bit build:      full sim.exe, 361 objects, 0 errors, 0 backend warnings
    32-bit build:      full sim.exe, 361 objects, 0 errors, 0 backend warnings
    startup:           dr_os_init -> dr_os_open (320x200) -> dr_textur_init:
                       real window, pitch 320, framebuffer non-NULL, both depths
    texture format 16: SDL_PIXELFORMAT_RGB565  (queried from the live texture)
    texture format 32: SDL_PIXELFORMAT_ARGB8888
    surface format 16: RGB565 masks, amask 0 - historical
    surface format 32: ARGB8888 masks, amask 0xFF000000, accepted by the
                       32-bit validation
    resize:            dr_textur_resize to 500x300 -> pitch 512, new
                       framebuffer pointer, round trip exact afterwards
    frame round trip:  EXACT at 32 through BOTH production paths - the
                       whole-texture upload in dr_flush() and the dirty-tile
                       dr_textur(x,y,w,h) partial upload - on every SDL2
                       driver on this machine: direct3d (D3D9, the default),
                       direct3d11, direct3d12, opengl, software. At 16 both
                       paths present the historical RGB565.

## SDL3

    16-bit build:      full sim.exe, 361 objects, 0 errors, 0 backend warnings
    32-bit build:      full sim.exe, 361 objects, 0 errors, 0 backend warnings
    startup:           as SDL2; framebuffer is SDL3's own contiguous PIXVAL
                       allocation (pre-existing design), pitch 320
    texture format 16: SDL_PIXELFORMAT_RGB565
    texture format 32: SDL_PIXELFORMAT_ARGB8888
    surface format 16: n/a - SDL3 has no surface; the format details are
                       validated: 16 bpp, Amask 0
    surface format 32: validated 32 bpp, Amask 0xFF000000
    resize:            500x300 -> pitch 512, new pointer, exact afterwards
    frame round trip:  EXACT at 32 through both upload paths and confirmed
                       on screen by the compositor snapshot; historical at 16

## System colours

    | Backend | Depth | RGB(18,52,86) | Expected   | Result |
    |---------|-------|---------------|------------|--------|
    | GDI     | 32    | 0xFF123456    | 0xFF123456 | PASS   |
    | SDL2    | 32    | 0xFF123456    | 0xFF123456 | PASS   |
    | SDL3    | 32    | 0xFF123456    | 0xFF123456 | PASS   |
    | GDI     | 16    | 0x11AA        | 0x11AA     | PASS   |
    | SDL2    | 16    | 0x11AA        | 0x11AA     | PASS   |
    | SDL3    | 16    | 0x11AA        | 0x11AA     | PASS   |

    Also, every backend and depth: pure red / green / blue exact, C2
    RGB(201,77,13) -> 0xFFC94D0D / 0xCA61, C3 RGB(248,252,8) ->
    0xFFF8FC08 / 0xFFE1. Through the real backend objects, hand constants.

## 32-bit framebuffer vectors

Written into the real framebuffer at hand-chosen positions, uploaded by the
production path, read back through an ARGB8888 render target (SDL2) /
SDL_RenderReadPixels (SDL3), both backends, both sizes:

    red:          0xFFFF0000 -> 0xFFFF0000
    green:        0xFF00FF00 -> 0xFF00FF00
    blue:         0xFF0000FF -> 0xFF0000FF
    0xFF123456:   0xFF123456 -> 0xFF123456
    mixed:        0xFFC94D0D -> 0xFFC94D0D   (RGB(201,77,13))
    background:   0xFF1F4363 -> 0xFF1F4363
    R/B swap:     NO
    quantization: NO - 0x12, 0x56, 0xC9, 0x4D, 0x0D are all off the RGB565
                  lattice and came back intact
    pitch:        correct - vectors at x up to 27 on pitch 320 and 512 land
                  where written

Compositor ground truth (PrintWindow, PW_RENDERFULLCONTENT): the initial
frame on SDL2/D3D9, SDL2/D3D11 and SDL3 shows exactly these values on screen.
After a resize the snapshot reads the background at the vector positions on
every driver, D3D11 included, while the render-target readback stays exact -
so that is the snapshot's stale client mapping, not the backends; the readback
is the authoritative round trip after resize.

## 16-bit freeze

    SDL2:  simsys_s2.o at COLOUR_DEPTH=16 BYTE-IDENTICAL to the pristine
           object (35 195 bytes, md5 80199047839b...)
    SDL3:  simsys_s3.o at COLOUR_DEPTH=16 BYTE-IDENTICAL to the pristine
           object (33 898 bytes, md5 3902ee1b85cd...)
    historical vectors: 0x11AA / 0xCA61 / 0xFFE1 through both real objects;
           the 16-bit round trips present RGB565 through both upload paths on
           both backends, identically for the pristine and the candidate
           source (measured side by side)
    full 16-bit builds: both link, 0 errors, 0 backend warnings

    Byte identity was not free: the first pass rewrote the validation
    message and the 16-bit object moved (the string and one extra argument).
    The 16-bit validation line is now verbatim under #else. The reported
    freeze is the second, identical object.

## Negative control

    mutation:          in a disposable copy, the ARGB8888 texture/surface
                       format forced back to RGB565 under COLOUR_DEPTH=32
                       (one site per backend)
    TRUE32 failure:    YES - both backends: the 32-bit validation rejects the
                       format (bpp 16 vs 32), dr_os_open returns pitch 0, no
                       framebuffer, RESULT FAIL; on SDL2 the same with the
                       driver forced to direct3d11
    SDL16 preserved:   YES - the 16-bit object of the MUTATED source is still
                       byte-identical to pristine (the mutation lives under
                       #if COLOUR_DEPTH == 32)

## Renderer freeze

    inventory:      90 / 90 / 0 / 0
    T1-T4:          PASS
    U8E:            19 of 19 image-matrix hashes unchanged
    U9:             backend probe identical to frozen
    API residual:   probe identical to frozen
    U10:            20/20 identical to sequential, 0 out-of-strip, 0
                    contamination, dirty 20/20, contexts distinct, peak 4/4
    legacy:         29 scenarios 4a758dc6c6612f7be4d49a46a2825d6dec93a9799ed12884ea864806748a6b39
                    in both builds; simgraph32.cc untouched

## Regression

    tests:     302/302 PASS ("Tests completed successfully", 0 error markers);
               20/20 vectors PASS
    warnings:  0 in simsys_s2.cc / simsys_s3.cc in all four full builds

## Performance / dependencies

    per-frame conversion:  none - the renderer writes ARGB8888 straight into
                           the surface (SDL2) / allocation (SDL3) that
                           SDL_UpdateTexture uploads; no RGB565<->ARGB8888
                           pass anywhere
    allocations:           none new; the 32-bit surface/allocation is the
                           same object the 16-bit build had, twice as wide
    new dependency:        none

## Debt

    SDL_SYSTEM_COLOUR_DEBT:  CLOSED
    SDL32_BACKEND_DEBT:      CLOSED
    KNOWN_RENDERER_DEBT:     NONE
    IMAGE_DIRTY_DEBT:        CLOSED
    BACKEND_DEBT:            CLOSED
    SET_LIGHT_COLOR_DEBT:    CLOSED
    GRAPHICAL_SURFACE:       COMPLETE
    API_SURFACE:             COMPLETE
    THREADING_DEBT:          CLOSED

## Recommended next action

    STLAB-SIMUTRANS-RGBA32-FULL-RUNTIME-CERTIFICATION-01

All three backends can now run the 32-bit renderer. Do NOT execute it.

## Safety

    fresh isolated lane:     confirmed
    renderer frozen:         confirmed - simgraph32.cc byte-identical
    STORED16 unchanged:      confirmed - no storage-format change anywhere
    no makeobj changes:      confirmed
    no pak format changes:   confirmed
    no network changes:      confirmed
    no savegame changes:     confirmed
    disposable trees:        g/full-sdl{2,3}-{16,32} (build evidence),
                             g/ncsrc and g/pristsrc removed
    no SVN commit:           confirmed
    no push:                 confirmed
    no publication:          confirmed

## Provenance

    harness:   rt_probe.cc - includes the REAL backend translation unit,
               drives dr_os_init / dr_os_open / dr_textur_init / dr_textur /
               dr_flush / dr_textur_resize / dr_os_close through a real
               window, with a minimal simgraph_t stand-in installed into the
               production `gfx` pointer; links the production objects of the
               matching depth minus the GDI backend and simmain. Readback via
               SDL render target (SDL2) / SDL_RenderReadPixels (SDL3), plus a
               PrintWindow compositor snapshot as an oracle outside SDL.
               nc_sdl32.sh for the mutants. Full builds: mingw32-make -j8
               COLOUR_DEPTH=16|32 with BACKEND := sdl2 | sdl3.
    toolchain: MinGW-w64 g++ 16.1.0, SDL2 and SDL3 from msys64/mingw64;
               drivers present: direct3d, direct3d11, direct3d12, opengl,
               opengles2, software
    processes: non-lab processes terminated: 0

### Probe defects, mine, all fixed before the numbers above were taken

  * `dr_os_init` takes a parameter array, not argc/argv.
  * `dbg` and `env_t` were `ret`-stubbed data symbols on the first link -
    writing to them crashed main; the fix was to link the real production
    objects.
  * the stand-in simgraph_t lacked `set_screen_height`, which dr_os_open
    calls - null call.
  * the SDL3 readback copied past a smaller readback surface after resize.
  * `dr_textur_new` does not exist - the "dirty-tile" variant had been
    calling a `ret` stub. The real entry is `dr_textur(x,y,w,h)`.
  * `<SDL_syswm.h>` is an SDL2 header and leaked into the SDL3 build.
  * my 16-bit expected-value formula assumed one RGB565 expansion rule; this
    machine's drivers use two. The 16-bit round trip is now a +/-1 per
    channel check with channel order; the 16-bit FREEZE rests on the
    byte-identical objects, not on the readback.
  * one apparent SDL3-16 dirty-tile failure did not reproduce on a fresh
    build (pristine and candidate identical); recorded as a transient of an
    earlier probe binary.
