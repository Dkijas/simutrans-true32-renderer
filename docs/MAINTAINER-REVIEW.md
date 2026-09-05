# Maintainer review checklist

A short list of the decisions a maintainer has to take on this candidate.
Tick, annotate, or open a "Maintainer review feedback" issue.

## Architecture

- [ ] the storage / screen separation (`STORED_PIXVAL` 16-bit, `PIXVAL`
      per depth, `SAVED_PIXVAL`, `NETWORK_PIXVAL`) makes sense
- [ ] the shared blitter core (`blit_core.h`, `blit_clip_core.h`,
      `zoom_core.h`, `simgraph_palette.h`) driven by a pixel policy is an
      acceptable way to keep the two renderers from diverging
- [ ] the `SCREEN32` representation (ARGB8888 word, flags carried
      separately in `FLAGGED_PIXVAL`) is acceptable

## Compatibility

- [ ] pak compatibility: the reader path (`image_reader.cc`,
      `ground_desc.cc`) keeps the 16-bit stored format unchanged
- [ ] savegame boundary: no format change (verify `SAVED_PIXVAL` uses)
- [ ] network boundary: `NETWORK_PIXVAL` stays 16-bit, no protocol change
- [ ] the 16-bit build stays the default and is output-identical (the
      29-scenario hash freeze is acceptable evidence)

## Backends

- [ ] GDI: depth-aware DIB header (`BI_RGB` at 32, unchanged 555/565
      branches) - `simsys_w.cc`
- [ ] SDL2: ARGB8888 streaming texture at 32 - `simsys_s2.cc`
- [ ] SDL3: ARGB8888 streaming texture at 32 - `simsys_s3.cc`
- [ ] the `simsys.h` interface change (`PIXVAL`-typed texture/framebuffer
      entry points) is acceptable for other ports (posix/headless
      unchanged; Android, macOS not exercised)

## Maintainability

- [ ] duplication between `simgraph16.cc` and `simgraph32.cc` (about 3600
      lines in the new file) is acceptable, given the shared cores
- [ ] code organisation and naming acceptable
- [ ] the future native 32-bit asset path is not prematurely coupled
      (nothing in the candidate assumes 32-bit stored pixels)
- [ ] the line-ending normalisation on integration (`svn:eol-style
      native`) is the intended way to handle the frozen files' mixed
      endings

## Build systems

- [ ] Makefile-only 32-bit build is acceptable for a first integration
- [ ] who adds `simgraph32.cc` to CMake / the Visual Studio projects

## Open points worth a maintainer's opinion

- [ ] memory: ~1.9x working set at 32 (pak128 ~920-960 MB) - acceptable
      as an opt-in build?
- [ ] load time at 32 (initial recode, ~3x observed once, not
      re-measured) - acceptable, or should the recode be lazier?
- [ ] Linux runtime untested in the laboratory (font environment) -
      someone with a working Linux desktop runs TESTING.md's quick path

## Integration decision

- [ ] candidate suitable for SVN integration (as an opt-in
      `COLOUR_DEPTH=32` build)
- [ ] changes requested (list them in an issue)
- [ ] not suitable
