# Known issues — XR32 testing-8 (r12377)

testing-8 = testing-7 + three changes: A4 (coast water keeps its 32-bit picture), A12-P1 (a 16 MiB cache of composed
terrain tiles) and TCC-R (translucent company colours keep their share when zoomed out). The XR32 format and makeobj do
not change. XR32 remains **experimental** and is not integrated into Simutrans Standard.

**Measurement scope.** Unless an item says otherwise, results were measured on the Windows x64 executables of this
release (SDL2 and SDL3, or their lab twins built from the same source with measuring hooks) on one machine with one
display. The Linux builds are experimental (K9).

## A. Of testing-8

### K1. Coast scenes with light changes are slightly slower

On the coast test world with light changes, ordinary frames take **+0.09 ms (+2.6 %) more work in median**; every
testing-8 run was slower than every testing-7 run. The slowest frames of that scene are not slower. On the pyramid test
world, a frame with a light change takes 12.8 % less time. See `reports/xr32-testing-8/release-gate/BENCHMARK-INTEGRATED.md`.

### K2. Zooming is not faster

The zoom optimisation measured for testing-8 (A10) showed no reliable gain and is not included.

### K3. The cache costs memory

The terrain cache holds up to 16 MiB; at most 13.4 MiB was measured. In 44–58 minute sessions, memory did not grow
any more than in testing-7 (`MEMORY-STRESS.md`).

### K4. Night colours under glass: confinement only

The translucent company-colour correction was checked against a model by day at five zooms, three companies, SDL3 and
SDL2. At night it was checked for confinement (only the affected pixels change), not against a model of every night
colour.

### K5. Some controls could not be shown

- Three screen-update scenarios repaint the whole screen in every frame, so a deliberately dropped update cannot show:
  their zero mismatches are not backed by a failing control (`DIRTY-REGIONS.md`).
- The Defender scan's ability to detect was not demonstrated with a test file.

## B. Inherited, unchanged by testing-8

### K6. SDL2: rare screen-update mismatches during window events

During resize and dialog events, SDL2 occasionally presents a few pixels that differ from the frame the game drew
(1 of 36 window-event runs of testing-8, and also seen in testing-7). The rate cannot be told apart between the two
versions, and testing-8 does not touch window, presentation or SDL code. The cause is not established
(`SDL2-WINDOW-EVENTS.md`).

### K7. Zoom 2/1: 8 pixels

At zoom 2/1, on SDL2 and SDL3, the window differs from the game's frame in 8 pixels by 1–6 colour levels — the same
pixels, at the same coordinates, as testing-7. Part of it is the timing of the capture; the cause of the rest is not
established (`ZOOM-2-1-PIXELS.md`).

### K8. Slow memory growth while zooming

During long zoom sessions memory grows slowly (about 5–11 MB per hour) in testing-7 and testing-8 alike; testing-8
grows no faster.

### K9. Linux: experimental

**Experimental — not fully tested or certified on native Linux.** The Linux x86_64 builds come from exactly the same
source (commit `3200e8e`). Only these were checked: the files are complete and uncorrupted, the binaries inside are the
builds of that source, the libraries resolve, and each program starts, loads a pak and quits cleanly under WSL 2, which
is not a native Linux desktop. No graphics, performance, memory or regression tests were run on Linux for testing-8.

### K10. A semi-transparent pixel in a LightTexture crashes at load

This happens in official r12377 too.

### K11. Not tested

macOS, Android, ARM, multi-monitor setups, a native Linux desktop. Only one machine, one display and one GPU.

### K12. Everything of testing-7

Every item of `docs/KNOWN-ISSUES-r12377-testing-7.md` that is not fixed above still applies, including the crash of the
official trunk r12377 when a city's first road runs onto a road with a private-way sign
(`docs/KNOWN-ISSUES-r12377-testing-6.md`, D).
