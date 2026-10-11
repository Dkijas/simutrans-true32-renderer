# Simutrans XR32 32-bit renderer — DAY+NIGHT test build on trunk r12337

> **IMPORTANT: start the game with `SIMUTRANS_XR32=1`, or you will not see any 32-bit art.**
>
> Windows (cmd):
>
>     set SIMUTRANS_XR32=1
>     simutrans-rgba32.exe
>
> Windows (PowerShell):
>
>     $env:SIMUTRANS_XR32="1"
>     .\simutrans-rgba32.exe
>
> Linux:
>
>     SIMUTRANS_XR32=1 ./simutrans-rgba32
>
> Without it, paks that contain 32-bit data still load and play normally, but the game deliberately draws their old
> 16-bit images instead.

**This is an experimental test build.** It is not an official Simutrans release, not merged into Simutrans and not a
stable version. Use a separate folder and copies of your savegames.

## Two switches, two programs

| Switch | Program | What it does |
| --- | --- | --- |
| `STLAB_XR32_WRITE=1` | **makeobj** | writes 32-bit (XR32) image data into the paks it builds, next to the ordinary 16-bit images |
| `SIMUTRANS_XR32=1` | **simutrans-rgba32** | reads and draws that 32-bit data |

A pak built with `STLAB_XR32_WRITE=1` also loads in the other builds we tested, which ignore the extra data: the
official Simutrans r12322 and r12337 and the 16-bit build. To *see* it, you need this build's `simutrans-rgba32`
**and** `SIMUTRANS_XR32=1`.

To check that XR32 is really on, start once with `-log -debug 2` added to the command line. If the variable is
**not** set, the file `simu.log` in your Simutrans user folder lists
`Warning: pakset_manager_t::read_nodes: Skipping unknown XR32-node` once for every 32-bit image. With
`SIMUTRANS_XR32=1`, that line does not appear.

## What this build is

- **Simutrans trunk r12337**: the current development version, including r12336's extra zoom-in levels (the zoom is
  now shown in the display settings) and r12337's network fix (a socket is no longer reused after a failed
  connection).
- **The XR32 DAY + optional NIGHT prototype:** 32-bit images can carry an optional night picture and explicit company
  colours. Old paks keep the normal rendering path.
- **Fixes since the previous test build (r12322):**
  - 40 px toolbar icons (for example Frank's pak64.german) get 40 px buttons;
  - 32-bit toolbar icons follow `icon_scaling` (theme-large 150 % → 60 px);
  - very thin 32-bit images (catenary poles) are no longer drawn too big and in the wrong place when zoomed far out.

## Downloads

Releases of this repository contain:

- Windows (SDL2 and SDL3) and Linux (SDL2) builds, each with `simutrans-rgba32` (32-bit, XR32) and `simutrans-16bit`
  (the normal renderer, for comparison);
- the matching makeobj for Windows and Linux;
- a test kit;
- the full source diff against trunk r12337.

No pakset is included.

## Source

`source/xr32-r12337/` is the complete source tree: official trunk r12337 plus this test build's changes.
`patches/r12337/` has the same changes as one full diff and as four separate steps (`APPLY-ORDER.txt`).

## Known issues and reports

See `docs/KNOWN-ISSUES-r12337.md` and the release notes (`RELEASE-NOTES-r12337-testing-1.md`).

Report problems at <https://github.com/Dkijas/simutrans-true32-renderer/issues>. Please include:

- the release tag;
- your OS and package (Windows SDL2 / SDL3, Linux);
- the pakset;
- whether `SIMUTRANS_XR32=1` was set;
- a PNG screenshot.

---

*The text below is the original description of this repository's first candidate (against trunk r12254). It is kept
for history; for the current test build, the section above applies.*

# Simutrans TRUE32 Renderer

Maintainer-review candidate for a true 32-bit (ARGB8888) screen renderer in
**Simutrans Standard**, presented against official SVN **r12254**.

## Status

    PRODUCT CANDIDATE
    Maintainer review only
    Not integrated into Simutrans SVN
    Not an official Simutrans repository, fork or release

This repository exists so that the Simutrans maintainers and the community
can read, build, run and judge the candidate. Nothing here has been
committed to the official trunk, and no integration is requested yet.

## What this is

An optional 32-bit ARGB8888 screen renderer for Simutrans Standard, built
by selecting `COLOUR_DEPTH=32` at build time. The historical 16-bit RGB565
renderer stays the default and is untouched in behaviour.

Image data in the paksets stays exactly what it is today (16-bit stored
pixels). The renderer recodes it once into an ARGB8888 screen and image
cache, so every operation that mixes colours at draw time - shading,
day/night, transparency, blending, outlines, zoom filtering - works with
8 bits per channel instead of 5/6/5.

## What changes

- the screen framebuffer and the per-player image caches are ARGB8888 when
  built with `COLOUR_DEPTH=32` (`src/simutrans/display/simgraph32.cc`, new)
- the image traversal, clipping and zoom logic shared by both renderers is
  factored into header-only cores used by `simgraph16.cc` and
  `simgraph32.cc` alike (`blit_core.h`, `blit_clip_core.h`, `zoom_core.h`,
  `simgraph_palette.h`, new)
- the three Windows backends can present a 32-bit framebuffer: GDI (32-bpp
  `BI_RGB` DIB), SDL2 and SDL3 (ARGB8888 streaming texture)
- the pixel-type contracts are made explicit in `simcolor.h`: the stored,
  saved and network pixel formats are separate typedefs from the screen
  pixel (`PIXVAL`)

18 production files: 12 modified, 6 new (the renderer candidate, frozen
since its certification), plus 5 build files (CMake and Visual Studio
renderer selection by `COLOUR_DEPTH`). Patches:

- [`patches/true32-r12254-v2.diff`](patches/true32-r12254-v2.diff) - the
  current review patch: renderer candidate + build integration (23 files,
  65 hunks, +4484/-791)
- [`patches/true32-r12254.diff`](patches/true32-r12254.diff) - v1, the
  renderer candidate alone as certified (kept for audit)
- [`patches/build-integration-r12254.diff`](patches/build-integration-r12254.diff) -
  the build-file part on its own (5 files, 18 hunks, +60/-20)

## What does not change

- pak image storage: unchanged, existing 16-bit assets are what is drawn
- makeobj and the pak file format: unchanged
- savegame format: unchanged
- network protocol: unchanged (`NETWORK_PIXVAL` stays 16-bit)
- the legacy 16-bit renderer: still the default build, output-identical to
  trunk (a 29-scenario hash freeze of `simgraph16.cc` is part of every
  certification run)
- no native 32-bit pak assets: that would be a separate, future project

See [docs/COMPATIBILITY.md](docs/COMPATIBILITY.md).

## Current validation status

The candidate went through a laboratory programme of unit and identity
gates and then two full product certifications on the real executable.
The final one,
[RGBA32-FULL-RUNTIME-RECERTIFICATION-02](reports/RGBA32-FULL-RUNTIME-RECERTIFICATION-02.md),
classified it **PRODUCT CANDIDATE** with one documented environment
limitation:

- internal identity gates: pass, frozen
- real runtime on GDI32, SDL2-32 and SDL3-32 with pak64, pak128 and
  pak192.comic: scroll, zoom, GUI, night, transparency, road construction
  with the production tool, rotation, resize (68/68 clip checks), screenshot,
  save and reload: pass
- window contents identical to the framebuffer on all three backends;
  99.5-99.9 % of runtime pixels carry values RGB565 cannot represent
  (0.0 % in the 16-bit control)
- the three backends render the same paused state 100.00 % identically
- legacy 16-bit: identical to trunk (hash freeze + runtime smoke)
- stability: SDL3-32, pak128, 52 cycles / 38 minutes, 0 crashes, 0 clip
  mismatches, 10 saves, flat memory
- performance: no regression on a real-frame benchmark (see
  [docs/PERFORMANCE.md](docs/PERFORMANCE.md)); memory about 1.9x the 16-bit
  working set, stable

Summary: [docs/CERTIFICATION.md](docs/CERTIFICATION.md).

## Supported, tested backends

    GDI32     (Windows, 32-bpp DIB)
    SDL2-32   (ARGB8888 streaming texture)
    SDL3-32   (ARGB8888 streaming texture)

Build systems: the GNU Makefile (`make COLOUR_DEPTH=32`), CMake
(`-DCOLOUR_DEPTH=32`, MinGW and the Visual Studio generator) and the
hand-maintained Visual Studio projects (`msbuild /p:COLOUR_DEPTH=32`). The
CMake and Visual Studio selection was added and certified after the renderer
certification, as build-file changes only (5 files, no renderer change) -
see [docs/BUILDING.md](docs/BUILDING.md) and the build-integration section of
[docs/CERTIFICATION.md](docs/CERTIFICATION.md).

## Tested paksets

    pak64
    pak128
    pak192.comic

(Not redistributed here; see [docs/TESTING.md](docs/TESTING.md).)

## Status by platform

    Windows GDI32 / SDL2-32 / SDL3-32   PRODUCT CERTIFIED
    Build systems Makefile / CMake / MSBuild   certified (16 and 32)
    Linux                               compile PASS, runtime ENVIRONMENT_BLOCKED
    Android (Gradle/CMake, x86_64)      compile, package, emulator run and internal
                                        framebuffer precision CERTIFIED at 32 (emulator);
                                        physical device / ARM NOT RUN
    Windows GDI resize-event delivery   CERTIFIED FIX AVAILABLE as a separate auxiliary
                                        patch (patches/gdi-resize-event-delivery-r12254.diff);
                                        NOT in SVN, maintainer approval required
    Windows GDI DPI WindowSize units    CERTIFIED FIX AVAILABLE as a separate auxiliary patch
                                        (patches/gdi-dpi-windowsize-r12254.diff), certified on
                                        top of the resize-event fix; forum topic 23805 root
                                        cause PROVEN, the topic itself stays OPEN; NOT in SVN

## Known limitation

In the WSL Linux environment used for certification, FreeType cannot load
the BDF fonts shipped with Simutrans, and the program stops at the modal
font-selection dialogue before entering the game loop. This blocks the
16-bit runtime in exactly the same way, so it is not demonstrated to be
TRUE32-specific. Linux compilation of SDL2-16, SDL2-32 and SDL3-32
succeeds. Linux runtime is therefore **not certified** here, neither way.

## How to review

1. Get the official base: `svn checkout -r 12254 svn://servers.simutrans.org/simutrans/trunk`
2. Apply the patch: `svn patch --strip 1 true32-r12254.diff` (or
   `patch -p1` on an LF checkout)
3. Check the identity: hashes in [`evidence/hashes/`](evidence/hashes/)
4. Build: `make COLOUR_DEPTH=32` (with `BACKEND := gdi|sdl2|sdl3` in
   `config.default`), or `cmake -DCOLOUR_DEPTH=32`, or
   `msbuild Simutrans-GDI.vcxproj /p:COLOUR_DEPTH=32` -
   [docs/BUILDING.md](docs/BUILDING.md)
5. Test: the quick path in [docs/TESTING.md](docs/TESTING.md)
6. Review checklist: [docs/MAINTAINER-REVIEW.md](docs/MAINTAINER-REVIEW.md)

The exact certified renderer files are also stored byte-for-byte under
[`source/candidate/`](source/candidate/), and the five certified build files
under [`source/build/`](source/build/).

## Documents

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - what the renderer split is
- [docs/COMPATIBILITY.md](docs/COMPATIBILITY.md) - what is certified, what is unchanged by design, what is future work
- [docs/CERTIFICATION.md](docs/CERTIFICATION.md) - the final certification, summarised
- [docs/PERFORMANCE.md](docs/PERFORMANCE.md) - measured frame times and memory
- [docs/LIMITATIONS.md](docs/LIMITATIONS.md) - Linux environment, build systems, line endings
- [docs/PROVENANCE.md](docs/PROVENANCE.md) - base revision, lineage, hashes
- [docs/BUILDING.md](docs/BUILDING.md) / [docs/TESTING.md](docs/TESTING.md)
- [reports/](reports/) - the canonical laboratory reports, with [reports/README.md](reports/README.md) saying which one is authoritative
- [evidence/](evidence/) - screenshots, hashes, runtime logs, benchmark data
- [CHANGELOG-TRUE32.md](CHANGELOG-TRUE32.md) - what each production file changes

## License

Simutrans is licensed under the Artistic License 1.0; this candidate is a
modification of the Simutrans source and is offered under the same license
([LICENSE.txt](LICENSE.txt), copied verbatim from r12254). All upstream
copyright and license headers are preserved.

## Upstream status

    NOT MERGED
    awaiting maintainer / community validation
