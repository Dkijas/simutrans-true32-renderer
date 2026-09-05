# Testing

## Quick review (15 minutes)

Build a 32-bit executable for the backend you use (BUILDING.md) and point
it at a pakset you already have - pak64, pak128 and pak192.comic were the
certified ones; none is redistributed here (get them from the usual
Simutrans download locations or build them from their sources).

1. **Compile** `make COLOUR_DEPTH=32` - expect 0 errors, no warning you
   do not see at 16.
2. **Launch** - the welcome screen and menu must look normal (a pink or
   colour-shifted window was the signature of the GDI DIB defect closed in
   REMEDIATION-01; it must not appear).
3. **Load or generate a world** - load one of your saves, or start a new
   map.
4. **Scroll** across the map, including over water and coast.
5. **Zoom** in and out through all levels.
6. **Night** - let the clock run into the night, or change the day/night
   level in the display settings; shading should be smooth, no banding
   that the 16-bit build does not show.
7. **Transparency** - hide buildings / trees with transparency (display
   settings): translucent objects must blend cleanly.
8. **Resize** the window: shrink, enlarge, maximise, restore, repeatedly,
   with a text window open near the edge. No crash, no stale clipping,
   no garbage text (the second REMEDIATION-01 defect lived here).
9. **Screenshot** (the in-game key): the PNG must match what you see.
10. **Save, then load** the save: identical world.

Then do the same run at `COLOUR_DEPTH=16` from the same tree: it must
behave exactly as trunk r12254.

## What to compare against

`evidence/screenshots/` holds labelled frames of the same paused pak128
state through GDI16, GDI32, SDL2-32 and SDL3-32, plus night,
transparency, zoom, resize, a pak64 generated world with a freshly built
road and a pak192.comic world; `evidence/comparison-grid.png` puts the
four backends side by side, and `evidence/true32-precision.png` shows
which pixels of a frame carry values RGB565 cannot hold.

## Advanced: the certification harness

The certification runs used a laboratory hook (compiled only with
`-DSTLAB_CERT_HARNESS`, never part of the candidate) that scripts the
production entry points: viewport moves, zoom, day/night, resize through
`gfx->on_window_resized`, `take_screenshot`, `karte_t::save` /
`karte_t::load`, the two-click way tool, a clip-rectangle postcondition
check, a real-frame benchmark, memory readings and quit provenance. Its
logs are in `evidence/runtime/` (one line per command with its result;
`clip ... MATCH` is the resize postcondition, `MEM` lines the working set,
`BENCH` lines the benchmark). The hook itself is laboratory tooling and is
not included; the reports describe what each gate measures and how, and
the exact hashes of every frozen gate are in the reports.

Identity of an applied tree: `evidence/hashes/` (see PROVENANCE.md).

## Reporting

Use the issue templates (bug/regression, build or backend problem,
maintainer review feedback). A useful bug report names the backend, the
bit depth, the pakset, the world (generated or a save), and attaches a
screenshot; for crashes, the command line and, if possible, a backtrace.
