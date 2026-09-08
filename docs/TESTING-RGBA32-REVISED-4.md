# Testing the RGBA32 revised-4 candidate (base r12261)

This branch carries a **later candidate** than the r12254 one on `main`. It is
published for **testers**, with a prebuilt Windows binary, so people can play
with it without building anything.

    CANDIDATE PUBLISHED FOR TESTING
    STANDARD INTEGRATION: NOT AUTHORISED / NOT PERFORMED
    Experimental prototype. Not an official Simutrans release.

Next step: **tester feedback and its evaluation** — not SVN integration.

`main` is untouched. The r12254 review candidate stays exactly where it is.

Download: [`rgba32-revised-4-r12261-testing-1`](https://github.com/Dkijas/simutrans-true32-renderer/releases/tag/rgba32-revised-4-r12261-testing-1)
(Windows x86_64, SDL2, archive sha256
`99a1c3e46c0d875ce8cb6bab30058c302d93d8cf199b1284cf24957522319632`).

## Identity

| item | value |
|---|---|
| candidate | `RGBA32-CANDIDATE-REVISED-4` |
| patch | [`patches/rgba32-candidate-revised-4-r12261.diff`](../patches/rgba32-candidate-revised-4-r12261.diff) |
| sha256 | `b562c2da8dd1281809cb8a08a657423578a0daf67068d0560e0936238d6cc08c` |
| base | official SVN trunk **r12261** = git `ecca332dca25025833f5ce6641491f8886a3ef9f` |
| shape | 26 files, 15 modified and 11 added, +6743 / −832 |
| full tree | [`source/rgba32-revised-4-r12261/`](../source/rgba32-revised-4-r12261/) |

The tree in `source/rgba32-revised-4-r12261/` **is** r12261 plus that patch, and
nothing else: it was produced by `svn export -r 12261 --native-eol LF` followed
by `patch -p1 --binary`, it differs from the pristine export in exactly 26 paths,
and those 26 are byte-identical to the tree that carries the certification
evidence. You can reproduce that check yourself:

```
svn export -r 12261 --native-eol LF svn://servers.simutrans.org/simutrans/trunk base
cp -r base tree && cd tree
patch -p1 --binary -i ../patches/rgba32-candidate-revised-4-r12261.diff
diff -rq ../base .        # expect exactly 26 differing paths
```

The tree carries Simutrans' own `.gitignore`, which is written for a working
copy and would exclude `simutrans/text/`, `simutrans/font/`, `config.template`
and the `.vcxproj.filters` files. They are all present here — the tree was added
deliberately in full, because it is a published artefact, not a checkout.

## What this candidate is

An optional 32-bit ARGB8888 screen renderer, **off by default**, plus the shared
core the 16-bit renderer now uses.

- A default build still compiles `simgraph16.cc` at `COLOUR_DEPTH=16` and draws
  the same pixels as trunk.
- `simgraph16.cc`'s clipping geometry, its three image blitters and its
  resampler now come from `blit_clip_core.h`, `blit_core.h` and `zoom_core.h`,
  which the 32-bit renderer shares.
- `COLOUR_DEPTH` is now defined for the `simusquirrel` object library as well as
  for `simutrans`; it was not, so the two halves of one binary would have
  disagreed about the width of `PIXVAL` in a 32-bit build.
- Pakset image data, savegames and the network protocol are unchanged.

Relative to the r12254 candidate on `main`, this one adds the XR32 loader seam,
three multithreading fixes, the scaled-alpha and stored-pixel fixes, and the
build-system change described above.

## Building

The exact command the published binary was built with:

```
cmake -S source/rgba32-revised-4-r12261 -B build32 \
      -DCMAKE_BUILD_TYPE=Release \
      -DSIMUTRANS_BACKEND=sdl2 \
      -DSIMUTRANS_TRUE32=ON \
      -DSIMUTRANS_MULTI_THREAD=ON \
      -DSIMUTRANS_USE_REVISION=12261
cmake --build build32 --target simutrans -j 8
```

Note `--target simutrans`: CMake creates a *directory* called `simutrans` in the
build tree, so `make simutrans` is silently a no-op.

For the 16-bit comparison build, the same command with
`-DSIMUTRANS_TRUE32=OFF`.

Two things worth knowing:

- **`SIMUTRANS_TRUE32` is honoured only for `SIMUTRANS_BACKEND=sdl2`.** sdl3 and
  gdi stay at `COLOUR_DEPTH=16`, headless at 0.
- **Only CMake can build the 32-bit renderer.** The GNU Makefile builds
  `simgraph$(COLOUR_DEPTH).cc` with a depth of 16 or 0, and the Visual Studio
  projects name `simgraph16.cc` explicitly; neither lists `simgraph32.cc` or
  `stlab_xr32.cc`, and neither is changed by this candidate.

To confirm a build really is 32-bit, look in the binary rather than at the
command line: `nm -C simutrans.exe | grep g_simgraph` must find `g_simgraph32`
and must not find `g_simgraph16`.

## XR32

XR32 is an optional sibling node in a pak file. It is gated three times:

1. the build must be 32-bit;
2. the environment variable `SIMUTRANS_XR32` must be set, and it is read once at
   startup by `getenv("SIMUTRANS_XR32")` with the test `e[0] == '1'` — so the
   **first character must be `1`**. It is not a CMake option, not a command-line
   switch, and not a `simuconf.tab` setting;
3. the pakset must actually contain XR32 nodes.

**No public pakset contains them, and makeobj is not part of this candidate**, so
there is nothing to produce them with. On any downloadable pakset,
`SIMUTRANS_XR32=1` changes nothing visible. Testing the 32-bit renderer against
ordinary IMG paksets is the useful test; testing XR32 content is a separate
thing with no content behind it yet.

Measured on a lab pak that does carry the node, all three runs reaching the game
world: 32-bit without the variable skips 12 XR32 nodes, the 16-bit build skips
the same 12 as an unknown 4CC, and 32-bit with `SIMUTRANS_XR32=1` skips none.

## What is measured and what is not

Measured:

- the 16-bit renderer's output is unchanged by this candidate: 13 440 drawing
  cases — three zooms, four clipping passes, seven draw families, eighty images,
  two players — byte-identical to trunk r12261, with coverage counted inside the
  moved code and two injected defects (one in the clipping, one in the
  resampling) caught by the same comparison;
- the 32-bit renderer's frozen colour contracts, its zoom resampling (exact over
  645 M pixels), and three multithreading defects found and fixed with
  sanitizers and negative controls;
- this package, extracted and started: it loads a saved world and runs, with the
  32-bit ARGB8888 framebuffer confirmed active.

Not measured — and this is what tester feedback is for:

- long real play sessions on real paksets;
- any machine other than the single Windows / MinGW GCC 16.1 box this was built
  and measured on;
- the GDI and SDL3 backends, and Linux and macOS: not built here;
- interactive zoom changes and info windows **driven through this binary** — the
  zoom levels and the info-window draw families are covered by the certified
  matrices on the same sources, but the machine used has no GUI automation, so
  they were not re-driven through the published executable.

## Known inherited defects

Not introduced by this candidate, not fixed by it, and not claimed harmless.

| # | what | note |
|---|---|---|
| L1 | left shift of a negative value in the clipping code | verbatim from `simgraph16.cc`; reproduces in trunk; now also compiled into `blit_clip_core.h` |
| L2 | unlocked read of `recode_flags` in the main viewport | trunk's; this candidate adds a writer under the same mutex and does not widen it |
| L3 | one-element overread in `create_texture_from_tile` when a **ground** image carries a semi-transparent pixel | trunk's; no shipped pakset has such art. AddressSanitizer could not cover the case: it aborts during world creation before the first frame |

## Reporting

Open an issue. Please give the package name and candidate, your Windows version
and graphics hardware, the pakset and its version, the steps from a fresh start,
**the zoom level and which window was open**, a screenshot, the log
(`-debug 3`), and a minimal savegame when one is needed.

Use copies of your savegames and a separate test folder — `-singleuser` keeps
everything inside the extracted folder instead of your `Documents\Simutrans`.

The most useful comparison is against `simutrans-16bit.exe` from the same
package on the same world, not against memory of another version.
