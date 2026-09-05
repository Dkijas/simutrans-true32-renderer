# Certification summary

Authoritative report:
[RGBA32-FULL-RUNTIME-RECERTIFICATION-02](../reports/RGBA32-FULL-RUNTIME-RECERTIFICATION-02.md)
(verdict B - TRUE32 PRODUCT CERTIFIED WITH DOCUMENTED ENVIRONMENT
LIMITATION; `RGBA32_RENDERER_STATUS = PRODUCT_CANDIDATE`). This page
summarises it; the measured facts are in the report and in
[`../evidence/`](../evidence/).

Environment: Windows 11, 16-core machine, one 3440x1440 display, MinGW-w64
g++ 16.1.0 (`-O3`, `MULTI_THREAD`), SDL2 / SDL3 3.4.2 from MSYS2; WSL
Ubuntu g++ 15.2 for the Linux builds. Base trunk r12254.

## Internal identity

    PASS

Every laboratory gate of the development programme was re-run against
freshly built objects of the frozen candidate and came out identical to
its frozen reference: the 29-scenario legacy hash of `simgraph16.cc`
(default and `LOW_LEVEL`, equal to pure trunk), the core blitter probes
(T1-T4), coloured/day-night, zoom preservation, blend/alpha/outline,
remaining image paths, drawing primitives, GUI state, nine-patch, image
dirty contract, backend closure, API residual (90 primitives real, 0
stubs), and threading lane isolation.

## Real runtime

    PASS

Backends: GDI32, SDL2-32, SDL3-32 (all three certified).
Paksets: pak64 (generated 128x128 world), pak128 (a real 192x512 game with
22 cities), pak192.comic (generated world, SDL3).

Runtime features exercised on every backend, live game, 4 display lanes:
startup, world load or generation, scroll, zoom in/out, GUI windows and
text, player colours, day/night levels, transparency (hidden buildings and
trees drawn translucent), real road construction with the two-click way
tool, four map rotations, window resize, screenshot, save and reload
inside the session.

Resize/clip: 68 / 68 clip checks MATCH (45 in dedicated resize sequences
including a 3424x1360 maximise-equivalent geometry, 9 in the scenarios,
14 in the stability run); no crash, no stale clip, no malformed text.

Window/framebuffer: the window contents captured through the compositor
(PrintWindow) are 100 % identical to the framebuffer screenshot on GDI,
SDL2 and SDL3 - the presentation path is 32-bit end to end.

TRUE32 evidence: 99.5-99.9 % of the pixels of real frames carry channel
values that RGB565 cannot represent; the 16-bit control frames have 0.0 %.
(See [`../evidence/true32-precision.png`](../evidence/true32-precision.png).)

Backend consistency: one paused world state rendered through GDI32,
SDL2-32 and SDL3-32 is 100.00 % identical across the three (day, night,
zoomed out, transparent).

## Legacy

    PASS

The 16-bit product built from the candidate tree: renderer hash identical
to trunk, warning sets identical to trunk, runtime smoke (startup, scroll,
zoom, night, resize, screenshot, save, reload) on GDI16, SDL2-16 and
SDL3-16.

## Stability

    PASS

SDL3-32, pak128, `-freeplay`, 1 display lane: 52 cycles of scroll, zoom,
day/night, resize (every third cycle) and save (every fifth), 38.2 minutes
of scripted activity, 0 crashes, 18/18 clip checks, 10 saves, 54
screenshots, memory 964 MB -> 990 MB (peak 998 MB, flat from cycle 5),
planned shutdown reached. A spontaneous clean termination seen once in the
previous cycle did not recur under quit-provenance instrumentation.

## Performance

    classification NORMAL

No blocking regression. On a real-frame benchmark (completed presents of
a full redraw, 1920x1080, one lane) the 32-bit build was not slower than
the trunk 16-bit build in any configuration; details and caveats in
[PERFORMANCE.md](PERFORMANCE.md).

## Memory

About 1.9x the 16-bit working set in the measured workload (pak128:
487 MB -> 920 MB on GDI, 523 MB -> 962 MB on SDL3), explained by the
doubled per-pixel image caches; stable over time, no leak demonstrated.

## Limitation

Linux runtime not certified: the laboratory's WSL FreeType cannot load the
BDF fonts, which blocks the 16-bit runtime in the same way. See
[LIMITATIONS.md](LIMITATIONS.md).

## Earlier certification and remediation

The first full certification
([RGBA32-FULL-RUNTIME-CERTIFICATION-01](../reports/RGBA32-FULL-RUNTIME-CERTIFICATION-01.md),
verdict C) found three lifecycle defects that the unit gates could not
see: a missing recode lock (startup crash at 12 display lanes), a missing
resize epilogue / clip clamp (crash after shrinking the SDL window), and
the GDI DIB described with 16-bit masks at 32 bpp (pink window). They were
closed in
[RGBA32-RUNTIME-REMEDIATION-01](../reports/RGBA32-RUNTIME-REMEDIATION-01.md)
(verdict B) with before/after evidence and negative controls, and the
recertification above was run on that frozen result without further
change.
