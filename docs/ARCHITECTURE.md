# Architecture

## The separation

Simutrans historically used one 16-bit word for everything a pixel can
be: what makeobj stores in the pak, what the image cache holds, what the
screen shows. The candidate separates *storage* from *screen*.

Legacy (unchanged, the default build):

    pak image (STORED16, RGB565 + special colours)
        -> image cache (16-bit)
        -> RGB565 screen

TRUE32 (`COLOUR_DEPTH=32`):

    pak image (STORED16, unchanged)
        -> recode once per image and player colour scheme
        -> ARGB8888 image cache and framebuffer
        -> 32-bpp presentation (GDI DIB, SDL2/SDL3 texture)

Everything that mixes colours at draw time happens in 8 bits per channel:
day/night shading, player colour recoding, transparency and blending,
outlines, zoom filtering. The source images do not gain detail - they are
the same 16-bit assets - but nothing is re-quantised to 5/6/5 between the
asset and the screen.

## Pixel-type contracts (`src/simutrans/simcolor.h`)

    PIXVAL            the screen pixel: uint16 (RGB565) at COLOUR_DEPTH 16,
                      uint32 (ARGB8888) at COLOUR_DEPTH 32
    FLAGGED_PIXVAL    a screen pixel plus flags in the upper bits: uint32 / uint64
    STORED_PIXVAL     uint16 - what paks store and image_reader decodes (unchanged)
    SAVED_PIXVAL      uint32 - what save/screenshot paths serialise
    NETWORK_PIXVAL    uint16 - what crosses the network (unchanged)

Flags and state (special colours, transparency markers, player-colour
indices) are carried independently of the 32-bit colour word, so a
`FLAGGED_PIXVAL` at 32 is a 64-bit value and the flag semantics are the
same at both depths.

## Renderer selection

`src/simutrans/display/simgraph.h` describes the renderer as a function
table (`struct simgraph_t`) reached through the global `gfx` pointer; the
executable contains one renderer, chosen at build time by the Makefile
(`SOURCES += src/simutrans/display/simgraph$(COLOUR_DEPTH).cc`). The
16-bit executable is built from the same tree and is output-identical to
trunk; the 32-bit executable links `simgraph32.cc` instead.

## Shared cores

The two renderers share the renderer-independent parts through
header-only cores that take a *pixel policy* (the type of a screen pixel
and the copy/blend operations on it):

                 blit_core.h        image run-stream traversal
                 blit_clip_core.h   clipped traversal (clip rectangles, lanes)
                 zoom_core.h        zoom-level image generation
                 simgraph_palette.h palette / special-colour tables
                          |
                 ---------+---------
                 |                 |
            PixelOps16        PixelOps32
              RGB565           ARGB8888
           (simgraph16.cc)   (simgraph32.cc)

`simgraph16.cc` was refactored to use the shared cores; its output is
frozen by a 29-scenario hash that is equal to the pure trunk renderer and
is re-checked in every certification (see CERTIFICATION.md). `simgraph32.cc`
implements the same function table over ARGB8888 with its own pixel
policy; it is a new file (about 3600 lines) and the bulk of the patch.

## Backends

- `simsys_w.cc` (GDI): the DIB header is depth-aware - `BI_RGB` at 32 bpp
  (the framebuffer word is exactly what the desktop expects), the RGB555
  and RGB565 bit-field branches unchanged for 16-bit builds.
- `simsys_s2.cc` (SDL2) and `simsys_s3.cc` (SDL3): an ARGB8888 streaming
  texture at 32; the 16-bit texture path unchanged.
- `simsys.h`: the texture/framebuffer interface typed with `PIXVAL`.

## Threading

The multi-threaded display lanes (`MULTI_THREAD`, per-lane clip
rectangles) are preserved; the 32-bit renderer has its own per-lane clip
state and the same dirty-tile mechanism (16x16 tiles, atomic OR). Image
recoding is serialised exactly as in the 16-bit renderer (a mutex plus a
re-check of the per-player flag under it).

## Other touched files

- `image_reader.cc`, `ground_desc.cc`: the stored pixel type is named
  explicitly and the readers stay 16-bit.
- `gameinfo.cc`, `simmesg.cc`: use the `SAVED_PIXVAL` / screen-colour
  helpers instead of assuming a 16-bit screen word.
