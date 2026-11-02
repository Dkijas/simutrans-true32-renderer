> **IMPORTANT: start the game with `SIMUTRANS_XR32=1`, or you will not see any 32-bit art.**
>
> Windows (cmd): `set SIMUTRANS_XR32=1`, then `simutrans-rgba32.exe` ·
> Windows (PowerShell): `$env:SIMUTRANS_XR32="1"`, then `.\simutrans-rgba32.exe` ·
> Linux: `SIMUTRANS_XR32=1 ./simutrans-rgba32`
>
> Without it, paks with 32-bit data still load and play, but the game draws their old 16-bit images.
> makeobj has its own switch: `STLAB_XR32_WRITE=1` writes the 32-bit data into the paks it builds.

**Experimental test build.** It is not an official Simutrans release, it is not merged into Simutrans, and it is not a
stable version. Use a separate folder and copies of your savegames. Nothing is promised about loading savegames or paks
made with this build in later test builds.

> **Warning, from the official trunk r12377 itself:** founding a city whose first road runs onto a road with a
> **private-way sign** crashes the game. This happens in both programs of this release, as in testing-1 and testing-2.
> Avoid private-way signs next to places where a city may be founded. See *Known issues*.

## What this build is

- **The same Simutrans SVN trunk as testing-1 and testing-2, r12377** (the game shows "125.0.2 Nightly").
- **A screen-update change of the XR32 32-bit renderer**, on top of testing-2: what the renderer draws is unchanged;
  how the finished picture reaches the window changes. No new graphics feature; the DAY + optional NIGHT image
  prototype, the pak syntax and makeobj are unchanged.
- **The 16-bit renderer is unchanged.** `simutrans-16bit`, makeobj and the test kit are testing-2's files, byte for byte.

Each package has two programs: `simutrans-rgba32` (32-bit, XR32) and `simutrans-16bit` (the normal renderer, for
comparison).

## What's new since testing-2

- **Only the changed parts of the screen are sent to the graphics card**, with the 16-bit renderer's own method
  (16×16 tiles of this frame and the previous one, merged into rectangles). testing-2 sent the whole 32-bit screen
  every frame: 7.9 MB per frame at 1920×1080, 14.1 MB at 2560×1440. With a running game and nothing scrolling, it is
  now 0.15–0.45 MB.
- **Fixed (SDL3): a few interface pixels came out darker.** The 32-bit screen picture was blended with the window
  background; it is now copied without blending, on SDL2 and SDL3.
- **Two things the 16-bit renderer redraws but the 32-bit one did not report** are now reported: the whole screen when
  the light level or a company colour changes, and filled rectangles. testing-2 did not show the omission because it
  sent everything; the new screen update needs it.
- **Checked:** a test reads back, frame by frame, what reached the window and compares it with what the game drew, on
  Windows (SDL2 and SDL3) and Linux (SDL2 and SDL3, WSLg X11 and Wayland, and SDL's dummy driver): scrolling, zooming
  to the extremes, windows opened and closed, traffic, dusk and night, company colours, resize, fullscreen, double-size
  (MRES) and semi-transparent icons. A build made to fail on purpose (half of the changed rectangles not sent) was
  caught in every scenario where it can be. Every result that testing-2 certified (A1/A2, A3, A5, A6, N1, N5 and the
  automated tests) was measured again with the same tests and gave the same values.

## Cost and benefit

CPU time of a frame (the game is capped at 100 frames per second; this is the time the frame takes before the cap
waits, not a frame rate), testing-2 → testing-3, median frame (P50) over 2–3 interleaved runs of 600 frames each:

| Where | A running game (traffic) | Paused map | Scrolling (whole screen changes every frame) |
| --- | --- | --- | --- |
| Windows 11, 1920×1080, SDL2 / SDL3 | 2.90 → 2.27 / 3.03 → 1.85 ms | 2.94 → 1.90 / 3.17 → 2.94 ms | 3.01 → 2.94 / 2.84 → 2.95 ms |
| Windows 11, 2560×1440, SDL2 / SDL3 | 4.34 → 2.98 / 4.27 → 2.66 ms | 4.73 → 2.76 / 4.82 → 3.13 ms | 4.68 → 4.76 / 4.34 → 4.75 ms |
| Linux WSLg X11, 1920×1080, SDL2 / SDL3 | 7.60 → 6.62 / 9.06 → 6.67 ms | 8.13 → 7.56 / 9.95 → 7.51 ms | 7.56 → 7.55 / 9.06 → 9.13 ms |
| Linux WSLg X11, 2560×1440, SDL2 / SDL3 | 12.44 → 10.47 / 15.89 → 10.42 ms | 12.59 → 10.59 / 15.80 → 10.64 ms | 12.54 → 12.59 / 15.65 → 15.24 ms |

When only part of the screen changes, testing-3 is faster in every measured case. When the whole screen changes every
frame (scrolling), there is nothing to save: no measurable difference on Windows SDL2 at 1920×1080 and on Linux,
except Linux SDL3 at 2560×1440, which is faster (−0.42 ms); and a small cost on Windows SDL3 (+0.08 ms at 1920×1080,
+0.41 ms at 2560×1440) and SDL2 at 2560×1440 (+0.1 ms), whose cause was not established. Peak memory (Windows, paired
with testing-2): no measurable difference.

## How to test

1. Unpack into a new, empty folder and add a pakset. No pakset is included.
2. Start `simutrans-rgba32` with `SIMUTRANS_XR32=1`, as shown above.
3. Look for anything that stays on screen when it should have gone, or appears late: after scrolling, zooming, opening
   and closing windows, at dusk and dawn, after changing a company colour, after resizing the window or switching to
   fullscreen and back. Compare with `simutrans-16bit` with the same pakset and savegame.
4. Everything from testing-2 still applies: the toolbar at night, zoom over catenary, DAY/NIGHT content from the test
   kit (`xr32-night-prototype-r12377-testing-2-testkit.zip`, unchanged).

## makeobj

Unchanged since testing-1: use the makeobj of this release (`makeobj-xr32-night-prototype-r12377-testing-2-*`,
the same files as in testing-2) to build test content.

## Known issues

New, from the official trunk r12377 itself:

- **After a live theme change, the compass and one small toolbar element can keep their old picture** until something
  redraws them. The official 16-bit renderer leaves exactly the same pixels stale; testing-2 hid it by sending the
  whole screen every frame.

Unchanged from testing-2: coast water tiles stay 16-bit (open); night pictures cost memory (by design); the DAY/NIGHT
pak syntax is a prototype (by design); a zoom change costs about 1 ms on its first frame; three lesser open points
(N2, N3, N4). From the official trunk r12377 itself, unchanged: the private-way sign crash, empty scenario texts, the
chat's public tab, the music volume after off/on, zoomable toolbar icons after a map-zoom round trip (not with
`SIMUTRANS_XR32=1`), sanitizer misalignment reports.

**Where it was tested:** Windows 11 (SDL2 with Direct3D 9, SDL3 with Direct3D 11, one 3440×1440 display), and Debian 13
under WSL 2 on Windows 11 (SDL2 and SDL3) in WSLg's graphical session (X11 through XWayland, and Wayland; OpenGL) and
with SDL's dummy driver. **WSLg is not a native Linux desktop:** a native X11 or Wayland desktop, HiDPI above 1, several
displays and other GPU drivers were not tested. With SDL3 on X11, the one frame on which the window changes size could
not be checked (the window system resizes asynchronously; testing-2 behaves the same); every other frame was. Not
tested at all: macOS, Android, ARM.

Platforms: x86_64 only. Windows 8 or newer, and Linux with glibc 2.38 or newer; both with SDL2 or SDL3.

Details: `docs/KNOWN-ISSUES-r12377-testing-3.md` and the certification report
`reports/XR32-DIRTY-REGIONS-CERTIFICATION-01.md` in the repository.

## Feedback

Please report problems at <https://github.com/Dkijas/simutrans-true32-renderer/issues> and include:

- this release's tag;
- your OS and package (Windows or Linux, SDL2 or SDL3);
- the pakset, and whether `SIMUTRANS_XR32=1` was set;
- a PNG screenshot.
