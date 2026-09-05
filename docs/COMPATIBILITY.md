# Compatibility

Three categories: what the certification measured, what is unchanged by
design (and therefore not a claim of this candidate), and what is
explicitly future work.

## CERTIFIED (measured in RECERTIFICATION-02)

- Existing paksets render through the 32-bit renderer: pak64, pak128 and
  pak192.comic, real worlds and generated worlds, on GDI32, SDL2-32 and
  SDL3-32.
- The 16-bit executable built from the candidate tree is output-identical
  to trunk r12254: the 29-scenario renderer hash of `simgraph16.cc`
  (default and `LOW_LEVEL`) equals the pure-trunk value, the 16-bit
  `simsys_w.o` is byte-identical to trunk, warning sets are equal to
  trunk, and the 16-bit runtime smoke (startup, scroll, zoom, night,
  resize, screenshot, save, reload) passes on GDI16, SDL2-16 and SDL3-16.
- Savegames: worlds saved by the 32-bit executable are reloaded by it in
  the same session (`karte_t::save` / `karte_t::load`, 3 backends x 3
  paksets); the save format is not touched by the patch.
- Screenshots: the production `take_screenshot` path writes 8-bit-per-
  channel PNGs at 32 and behaves as before at 16.
- The three backends present the same framebuffer: window contents equal
  the framebuffer pixel for pixel (PrintWindow vs screenshot) and the
  three backends render one paused state 100.00 % identically.

## NOT CHANGED BY DESIGN

- pak file format and makeobj: untouched; `STORED_PIXVAL` stays 16-bit
  and `image_reader.cc` decodes what it decoded before
- savegame format: untouched
- network protocol: untouched (`NETWORK_PIXVAL` is 16-bit)
- legacy renderer: `COLOUR_DEPTH=16` remains the default build and the
  reference behaviour
- historical assets: every existing pak works unchanged; there is no
  asset conversion step

These are not certified *features* of the candidate; they are boundaries
the candidate does not cross, verified by inspection of the patch (no
reader/writer of those formats changes its data layout) and by the
runtime checks above.

## FUTURE WORK (not part of this candidate)

- Native 32-bit pak assets ("STORED32"): a separate project touching
  makeobj, the pak format and the readers. This candidate does **not**
  claim it.
- CMake and Visual Studio builds of the 32-bit renderer: the project
  files still name `simgraph16.cc`; adapting them is a small follow-up
  that was deliberately not made inside the certified candidate.
- Linux runtime certification: blocked in the laboratory environment by
  font loading (see LIMITATIONS.md), not by the renderer.
- A runtime switch between the two renderers inside one executable: not
  attempted; depth is a build-time choice today, as it always was.
