# Performance and memory

All numbers from RECERTIFICATION-02 on one machine (16-core Windows 11,
MinGW-w64 g++ 16.1.0, `-O3`, `MULTI_THREAD`). Raw per-window data in
[`../evidence/performance/benchmark.txt`](../evidence/performance/benchmark.txt).

## Method

Frames are **completed presents**: a counter in every backend's `dr_flush()`
(laboratory build only), incremented once per present. Each present
follows one full redraw of the visible world plus the GUI. The benchmark
marks the world dirty on every loop iteration of a paused game with a fixed
camera (pak128, the same real world), so every frame is a full redraw of
the same scene; wall time from `dr_time()`, CPU time from
`GetProcessTimes`; 3 s warm-up, three 20 s windows per run, several runs.
The three executables come from the same tree and flags: pure trunk
r12254 at 16 bits, the candidate at 16 bits, the candidate at 32 bits.

## Frame limiter

At the product's maximum frame limiter (`fps 100`), all three hold
100 fps (2000 frames per 20 s window in every window).

## Full redraw, uncapped, 1920x1080, one display lane, GDI

Quiet machine, interleaved runs (3 passes x 3 windows each):

    trunk 16-bit  (GDI16):  median 4.88 ms/frame   (4.57-5.14)   ~205 fps
    candidate 32  (GDI32):  median 2.55 ms/frame   (2.14-3.16)   ~392 fps

The candidate built at 16 bits measured 5.19 ms/frame (5-run series),
within noise of trunk, as expected from the identical renderer output.

## Scaling checks (2 runs x 3 windows)

    GDI   1024x640,  1 lane:   16-bit 1.33 ms    32-bit 1.07 ms
    SDL2  1920x1080, 1 lane:   16-bit 3.25 ms    32-bit 2.86 ms   (about 12 % apart)
    GDI   1920x1080, 4 lanes:  16-bit 5.67 ms    32-bit 5.06 ms

## Reading the numbers carefully

- The large GDI difference is mostly **presentation**, not blitting: at
  32 bpp the DIB is `BI_RGB` in the desktop's own format, whereas the
  16-bit DIB is converted by GDI on every present. On SDL2, where both
  depths upload a texture, the two are about 12 % apart.
- Do **not** read this as "TRUE32 is 2x faster". The correct conclusion
  is: **no blocking performance regression was observed**, in any of the
  configurations measured, and the 32-bit renderer was never slower than
  the 16-bit one on this workload.
- Run-to-run dispersion is about +-10 % on a quiet machine and up to
  +-30 % with other processes running; the 16/32 gaps above are outside
  that noise except the 4-lane case, which is within it.
- Not measured here: load time. The first certification observed the
  initial load to be about 3x longer at 32 bits because every image is
  recoded at load; this was not re-measured in the recertification and
  is worth a maintainer's attention.
- The paused loop does not sleep, so CPU time equals wall time in these
  runs; CPU figures are not a separate render cost.

## Memory

Working set (pak128, one lane, right after load / 20 s later / after a
zoom step):

    GDI  16-bit:  487.5 / 487.6 / 491.3 MB
    GDI  32-bit:  919.6 / 919.6 / 924.7 MB
    SDL3 16-bit:  522.7 / 522.9 / 526.5 MB
    SDL3 32-bit:  962.2 / 962.2 / 967.3 MB

About 1.9x: the per-pixel image caches double in width. The zoom-cache
step is the same size at both depths. Stability run (SDL3-32, 38 min):
964 MB at start, 983-990 MB from cycle 5 on, 990 MB at the end, peak
998 MB - no unbounded growth, no leak observed in the measured run.
