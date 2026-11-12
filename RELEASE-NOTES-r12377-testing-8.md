# XR32 testing-8 — RGBA32 Coast Rendering, Terrain Cache & Translucent Colours

> **IMPORTANT: start the game with `SIMUTRANS_XR32=1`, or you will not see any 32-bit art.**
>
> Windows (cmd): `set SIMUTRANS_XR32=1`, then `simutrans-rgba32.exe` · Windows (PowerShell): `$env:SIMUTRANS_XR32="1"`, then
> `.\simutrans-rgba32.exe` · Linux: `SIMUTRANS_XR32=1 ./simutrans-rgba32`

**Experimental test build (pre-release).** This is not an official Simutrans release. It is not merged into Simutrans
Standard, not part of the official trunk, and not a stable version. XR32 remains experimental. Use a separate folder and
copies of your savegames.

| Platform | Status |
| --- | --- |
| **Windows x64, SDL3 and SDL2** | **certified with limitations** (see "Known limitations") |
| **Linux x86_64, SDL3 and SDL2** | **Experimental — not fully tested or certified on native Linux.** Built from exactly the same source; only checked that the files are complete and uncorrupted and that the program starts, loads a pak and quits cleanly under WSL 2 (not a native Linux desktop). No graphics, performance, memory or regression tests were run on Linux for testing-8 |

## What this build is

- **Simutrans SVN trunk r12377**, the same trunk as testing-1 to testing-7. No rebase.
- **testing-7 plus three separate changes**, one commit each, in two source files:

  | # | Change | Commit | File |
  | --- | --- | --- | --- |
  | 1 | A4 — coastal RGBA32 rendering correction | `b03d729` | `display/stlab_xr32_hook.cci` |
  | 2 | A12-P1 — composed terrain cache | `a644622b` | `display/stlab_xr32_hook.cci` |
  | 3 | TCC-R — translucent company-colour accuracy | `3200e8e` | `display/stlab_xr32_zoom.cci` |

  Source commit `3200e8e8bbc84880f74a32a2c5e3fb5d6c2d73de`, tree `70e54fc4a865d0ba850f2083df0c33d06a597aaa`
  (`source/xr32-r12377/` in this repository).
- **Each package** holds the 32-bit renderer `simutrans-rgba32` and, for comparison, the traditional 16-bit renderer
  `simutrans-16bit`. The 16-bit programs are byte-identical to testing-7's (on Linux, rebuilt from this source and
  byte-identical).

## What's new

### 1. Coast rendering: the water stays 32-bit (A4)

Simutrans reserves exact colours as special: company colours, lights, and six plain greys. In testing-7, a slope whose
`LightTexture` row contained one of those exact colours drew its land in 32-bit but **fell back to 16-bit for its coast
water**. Now the coast water of those slopes keeps its 32-bit picture too. Real special colours that an artist declared in
a mask are untouched.

### 2. Composed terrain cache: faster light changes (A12-P1)

The game composes every generated ground tile (light map × climate texture) and used to compose them all again at every
change of light. testing-8 keeps the composed tiles for reuse, in a cache **limited to 16 MiB**.

- **Pictures:** nothing looks different. The tiles are the same, bit for bit.
- **Memory:** up to 16 MiB more, the cache's content (at most 13.4 MiB was measured).

### 3. Translucent company colours when zoomed out (TCC-R)

When a picture that mixes company colours with semi-transparent pixels (for example the glass roofs of some station
platforms) is drawn smaller than its size, the company colour now gets its **correct share**. Before, it came out too
weak, by up to about 40 colour levels at 9/8, 3/4 and 1/2.

- **At 1/1 and 2/1** nothing changes.
- **On PAK128.German 2.5,** 47–68 of its 50,823 images look slightly different when zoomed out; every other image is
  identical.
- **Opaque company colours, and images without special colours,** are unchanged.

## Performance (Windows, against testing-7)

Measured on lab builds with identical probes, n = 5 interleaved runs per configuration, with an A/A control (testing-7
against itself) that showed no measurable difference anywhere. The game is capped at 100 FPS, so **no FPS figure is
claimed**.

| Scene | Result |
| --- | --- |
| **Light changes** (pyramid test world, PAK128.German 2.5) | a frame with a light change: median 6.41 → 5.58 ms, **−12.8 %**; every testing-8 run faster than every testing-7 run |
| Zoom, flat world | slowest frames (P99 of frame work) −1.21 ms; zoom frames (P95) −1.71 ms |
| Zoom, coast | zoom frames (median) −0.71 ms |
| **Coast with light changes** | ordinary frames **+0.09 ms (+2.6 %) in median: a small, real regression in this specific scene**; its slowest frames are not slower |
| TCC-R on its own | no measurable difference |
| Zooming in general | **not faster than testing-7**: a zoom optimisation (A10) was measured and **not included** |

## Compatibility

| | |
| --- | --- |
| Base | Simutrans SVN r12377, not rebased |
| XR32 format | **unchanged**. No reader or writer was touched |
| makeobj | **no functional change**. No makeobj or writer file was touched: keep using testing-7's makeobj |
| Traditional 16-bit renderer | **unchanged**, byte-identical to testing-7's |
| DAY/NIGHT pak syntax and night rendering | kept |
| Savegames | nothing saved changes |
| Paksets without 32-bit data | as before |
| A10 (a zoom optimisation) | **excluded**: it showed no reliable gain |

## Tested (Windows 11 x64, one display, SDL3 and SDL2)

- **Builds:** fresh, non-incremental, reproducible (the SDL3 build made twice gives the same bytes); warning sets equal to
  testing-7's.
- **Automated suite:** 309 of 309 tests pass on SDL3 and SDL2 (an injected failing test is caught).
- **Translucent company colours, in real windows:** 0 failures in 172 checks against a model, at five zoom levels, three
  companies, day and night, SDL3 and SDL2. The control (old and new swapped) fails 18 checks per backend.
- **Every 32-bit image of PAK128.German 2.5 and pak192.comic** drawn alone at each zoom: changes only where TCC-R applies.
- **Screen updates in real windows:** compared with what the game drew while zooming, scrolling, changing light, switching
  company colours, opening windows, resizing and switching to fullscreen, against testing-7, with a control that drops
  screen updates on purpose.
- **Memory:** 44–58 minute sessions side by side with testing-7. The cache stays below 16 MiB, and memory does not grow
  during a session any more than it does in testing-7 (a synthetic leak is caught by the same analysis).
- **Threads:** an 8-thread stress test of the cache completed 12 of 12 times; the cache without its lock crashes 12 of 12.
- **Malware scan:** Microsoft Defender found no threats in any file of this release.

**Not tested:** macOS, Android, ARM, multi-monitor setups, a native Linux desktop.

## Known limitations

- **Coast scenes with light changes:** ordinary frames take about 0.09 ms longer in median (+2.6 %).
- **Zooming is not faster than in testing-7.**
- **SDL2:** rare, brief mismatches in screen updates during window events (resize, dialogs). This behaviour is inherited:
  it was also seen in testing-7, and testing-8 does not change its rate measurably. Its cause is not yet established.
- **Zoom 2/1, SDL2 and SDL3:** 8 pixels where the window differs from the frame the game drew — the same pixels, at the
  same coordinates, as in testing-7. testing-8 adds none.
- **Memory while zooming:** a slow growth during long zoom sessions, in testing-7 too; testing-8 grows no faster.
- **Night colours under glass** were checked for confinement (only the affected pixels change), not against a model of
  every night colour.
- **A semi-transparent pixel in a `LightTexture`** crashes the game at load, in official r12377 too.
- **Linux:** experimental, not fully tested or certified on native Linux.
- **All known issues of testing-7** still apply (`docs/KNOWN-ISSUES-r12377-testing-7.md`).

Details: `docs/KNOWN-ISSUES-r12377-testing-8.md` and the release-gate reports in `reports/xr32-testing-8/`.

## Files

| File | SHA-256 |
| --- | --- |
| `simutrans-xr32-night-prototype-r12377-testing-8-win64-sdl2.zip` | `6e83343045d0f93f32307ae5f623eaf9b56961a8a559fdae058b0bc7dd89d9d4` |
| `simutrans-xr32-night-prototype-r12377-testing-8-win64-sdl3.zip` | `2a41012f4b7c8e7ea1ece5fdee7c2a213b360e128b7f99fde4bbb1d46179ba24` |
| `simutrans-xr32-night-prototype-r12377-testing-8-linux-x86_64-sdl2.tar.gz` | `cc9a6dc16e60858c3432609ed412b1d6594ca8a42101030dab226d2494d8ec80` |
| `simutrans-xr32-night-prototype-r12377-testing-8-linux-x86_64-sdl3.tar.gz` | `3b2f597543158f36b4919014b1842bdd943bda6723336d4327aa5a1aa1595f66` |
| `xr32-night-prototype-r12377-testing-8-source.tar.gz` | `33a64f0988eecc1f4eb6f9129f1344f2286a5e164c24122e1771c5e3c2d19350` |
| `XR32-NIGHT-PROTOTYPE-FULL.r12377-testing-8.diff` | `e7df3935df785c70a1f741ce50738e6e11d67bdbdd8a8abbfc58e4344a2f8550` |
| `01-A4.diff` | `e1e6edff66d7c8817861e35ff1698895ef769442bbcbe2f45e604789db0b0aaa` |
| `02-A12-P1.diff` | `cb38cf5c98f7f2bf47a1fab2e4df79a3c8e704c36bca77db7f5db9d4e8de2e30` |
| `03-TCC-R.diff` | `0817c4e75d2beeac5b15ed7daaafbde9491190e5fda94287057485d63e3a5c44` |
| `SHA256SUMS.txt` | `acfc75de730713a997ff591e77831d3cd4275044fbf77a1e0497c64928591014` |
| `NIGHT-PROTOTYPE-MANIFEST.md` | `e4c1ec3fbc810a609e9cca55df505d367a033db4aee383d9220603a715f51cb5` |

`SHA256SUMS.txt` covers every file above. The full diff is everything XR32 changes against official r12377; `01-A4.diff`,
`02-A12-P1.diff` and `03-TCC-R.diff` are the three changes on top of testing-7, in that order.

makeobj: not changed by testing-8. Use `makeobj-xr32-night-prototype-r12377-testing-7-*` from the testing-7 release.

## Reports

https://github.com/Dkijas/simutrans-true32-renderer/issues — please give the build name, your OS and package name, the
pakset, whether `SIMUTRANS_XR32=1` was set, and attach a PNG screenshot.
