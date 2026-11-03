# Simutrans XR32 32-bit renderer — DAY+NIGHT test build on trunk r12377, testing-5

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

> **Linux builds — Experimental / Not certified**
>
> The Linux x86_64 SDL2 and SDL3 builds are provided for community testing. They are built from the XR32 testing-5
> source but have not undergone the full Linux graphics certification process.
>
> Windows x64 SDL2 and SDL3 have been certified with limitations.
>
> Please use a separate installation, back up your savegames and report any issues.

> **Warning, from the official trunk r12377 itself:** founding a city whose first road runs onto a road with a
> private-way sign crashes the game, in both programs of this build. See `docs/KNOWN-ISSUES-r12377.md`, item B1.

## Two switches, two programs

| Switch | Program | What it does |
| --- | --- | --- |
| `STLAB_XR32_WRITE=1` | **makeobj** | writes 32-bit (XR32) image data into the paks it builds, next to the ordinary 16-bit images |
| `SIMUTRANS_XR32=1` | **simutrans-rgba32** | reads and draws that 32-bit data |

To check that XR32 is really on, start once with `-log -debug 2` added to the command line. If the variable is
**not** set, the file `simu.log` in your Simutrans user folder lists
`Warning: pakset_manager_t::read_nodes: Skipping unknown XR32-node` once for every 32-bit image. With
`SIMUTRANS_XR32=1`, that line does not appear.

## What this build is

- **Simutrans trunk r12377**, the same trunk as testing-1 to testing-4.
- **testing-4's XR32 renderer, plus two fixes:**
  - **translucent company colours** keep their company colour (makeobj and renderer): glass shows each company's
    colour by day and at night with full 8-bit alpha, translucent lights glow at night, ordinary RGB paint is never
    recoloured, and zooming weights each sample by its alpha;
  - **no more sand pixels in the water at zoom 2/1** next to sloped coasts (known issue A11 of testing-4).
- **Nothing else changes:** no new graphics features; the XR32 format, the DAY + optional NIGHT prototype and its pak
  syntax are unchanged; existing paks draw exactly as in testing-4; `simutrans-16bit` and the test kit are the same
  files as in testing-4. **makeobj is new** (it carries the company-colour fix).
- **Certification:** Windows x64 SDL2 and SDL3, certified with limitations. Linux x86_64 SDL2 and SDL3: experimental,
  not certified. No measurable frame-time or memory cost against testing-4 on Windows. See the release notes and
  `reports/xr32-testing-5/`.

## Downloads

Releases of this repository contain:

- Windows (SDL2 and SDL3) and Linux (SDL2 and SDL3) builds, each with `simutrans-rgba32` (32-bit, XR32) and
  `simutrans-16bit` (the normal renderer, for comparison);
- the matching makeobj for Windows and Linux;
- a test kit, the full source and the full diff against official r12377;
- `SHA256SUMS.txt` and a manifest.

## Source

- `source/xr32-r12377/` is the full source of this build: official r12377 plus XR32.
- `patches/r12377-testing-5/` has the change against official r12377, against testing-4, and as two steps
  (`APPLY-ORDER.txt`).

## Known issues, certification and reports

- `RELEASE-NOTES-r12377-testing-5.md` — what is new, cost, checks, known issues, where it was tested.
- `docs/KNOWN-ISSUES-r12377-testing-5.md` — open and fixed issues of this build.
- `reports/xr32-testing-5/` — the Windows certification of testing-5 (A11 root cause, TCC integration, test matrix,
  performance); `reports/xr32-tcc/` — the certification of the company-colour fix.
- `evidence/xr32-testing-5/` — selected evidence files.

## Previous test build: testing-4 on trunk r12377

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

> **Warning, from the official trunk r12377 itself:** founding a city whose first road runs onto a road with a
> private-way sign crashes the game, in both programs of this build. See `docs/KNOWN-ISSUES-r12377.md`, item B1.

### Two switches, two programs

| Switch | Program | What it does |
| --- | --- | --- |
| `STLAB_XR32_WRITE=1` | **makeobj** | writes 32-bit (XR32) image data into the paks it builds, next to the ordinary 16-bit images |
| `SIMUTRANS_XR32=1` | **simutrans-rgba32** | reads and draws that 32-bit data |

To check that XR32 is really on, start once with `-log -debug 2` added to the command line. If the variable is
**not** set, the file `simu.log` in your Simutrans user folder lists
`Warning: pakset_manager_t::read_nodes: Skipping unknown XR32-node` once for every 32-bit image. With
`SIMUTRANS_XR32=1`, that line does not appear.

### What this build is

- **Simutrans trunk r12377**, the same trunk as testing-1 to testing-3.
- **testing-3's XR32 renderer, plus 32-bit water on sloped coasts (A4).** The game makes the water of a sloped coast
  itself, from the sea picture, when it starts. Until testing-3 it made it only from the 16-bit sea, so a pak with a
  32-bit sea showed a 16-bit strip on every sloped coast. testing-4 makes it from the 32-bit sea (and its night
  picture), with exactly the steps of the 16-bit version.
- **Nothing else changes:**
  - testing-3's screen update;
  - the DAY + optional NIGHT prototype and its pak syntax;
  - makeobj and the test kit (testing-2's files, byte for byte);
  - `simutrans-16bit` (testing-3's file, byte for byte).

  Paks whose sea has no 32-bit data draw exactly as in testing-3.
- **Cost, on a map with many sloped coasts and 32-bit water:**
  - the typical frame shows no measurable difference;
  - the slowest 1 % of frames is +0.0 to +0.75 ms at 1920×1080, and up to +2.45 ms when zooming at 2560×1440 on
    SDL3;
  - peak memory is +0.6 / +0.9 MB.

  See the release notes.
- **New known issue, accepted for this test build:** at zoom 2/1 a few single sand-coloured pixels can show in the
  water next to sloped coasts. See the release notes and `docs/KNOWN-ISSUES-r12377-testing-4.md`, A11.

### Downloads

Releases of this repository contain:

- Windows (SDL2 and SDL3) and Linux (SDL2 and SDL3) builds, each with `simutrans-rgba32` (32-bit, XR32) and
  `simutrans-16bit` (the normal renderer, for comparison);
- the matching makeobj for Windows and Linux;
- a test kit;
- the full source diff against trunk r12377, and a source archive.

No pakset is included.

### Source

- `source/xr32-r12377/` is the complete source tree of testing-4: official trunk r12377 plus this test build's
  changes.
- `patches/r12377-testing-4/` has the same changes in two forms:
  - as one full diff against official r12377;
  - as A4 on top of testing-3, in one diff or in three reviewable steps (`APPLY-ORDER.txt`).
- The trees of testing-1 to testing-3 stay at their tags, and their patches in `patches/r12377/`,
  `patches/r12377-testing-2/` and `patches/r12377-testing-3/`.
- `patches/r12377/upstream-build-fix/` is **not** part of XR32. It is the one-word fix (`old32` → `ole32`) that
  official r12377's `CMakeLists.txt` needs before any Windows CMake build links.

### Known issues, certification and reports

See:
- `docs/KNOWN-ISSUES-r12377-testing-4.md`;
- the release notes, `RELEASE-NOTES-r12377-testing-4.md`;
- the certification reports in `reports/xr32-a4/`, with evidence in `evidence/xr32-a4/`.

Report problems at <https://github.com/Dkijas/simutrans-true32-renderer/issues>. Please include:

- the release tag;
- your OS and package (Windows or Linux, SDL2 or SDL3);
- the pakset;
- whether `SIMUTRANS_XR32=1` was set;
- a PNG screenshot.

---

*The section below describes the previous test build, testing-3 on trunk r12377. Its release and its tag stay
unchanged; the section is kept here for history.*

## Previous test build: testing-3 on trunk r12377

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

> **Warning, from the official trunk r12377 itself:** founding a city whose first road runs onto a road with a
> private-way sign crashes the game, in both programs of this build. See `docs/KNOWN-ISSUES-r12377.md`, item B1.

### Two switches, two programs

| Switch | Program | What it does |
| --- | --- | --- |
| `STLAB_XR32_WRITE=1` | **makeobj** | writes 32-bit (XR32) image data into the paks it builds, next to the ordinary 16-bit images |
| `SIMUTRANS_XR32=1` | **simutrans-rgba32** | reads and draws that 32-bit data |

To check that XR32 is really on, start once with `-log -debug 2` added to the command line. If the variable is
**not** set, the file `simu.log` in your Simutrans user folder lists
`Warning: pakset_manager_t::read_nodes: Skipping unknown XR32-node` once for every 32-bit image. With
`SIMUTRANS_XR32=1`, that line does not appear.

### What this build is

- **Simutrans trunk r12377**, the same trunk as testing-1 and testing-2.
- **testing-2's XR32 renderer with a new screen update:** what the renderer draws is unchanged; how the finished
  picture reaches the window changes. No new graphics feature; the DAY + optional NIGHT prototype, its pak syntax and
  makeobj are unchanged.
- **Changes since testing-2:**
  - only the changed parts of the screen are sent to the graphics card, with the 16-bit renderer's own method
    (testing-2 sent the whole 32-bit screen every frame: 7.9 MB at 1920×1080; with a running game it is now
    0.15–0.45 MB);
  - the 32-bit screen picture is copied to the window without blending (with SDL3, testing-2 let a few interface
    pixels blend with the window background);
  - a light level or company colour change, and filled rectangles, are reported as changed, as the 16-bit renderer
    does.
- The CPU time of a frame drops wherever only part of the screen changes (−0.6 to −5.5 ms per frame with traffic,
  Windows and Linux, SDL2 and SDL3); whole-screen scrolling is unchanged within a fraction of a millisecond. See the
  release notes for the measurements and the one small cost.
- `simutrans-16bit`, makeobj and the test kit are testing-2's files, byte for byte.

### Downloads

Releases of this repository contain:

- Windows (SDL2 and SDL3) and Linux (SDL2 and SDL3) builds, each with `simutrans-rgba32` (32-bit, XR32) and
  `simutrans-16bit` (the normal renderer, for comparison);
- the matching makeobj for Windows and Linux;
- a test kit;
- the full source diff against trunk r12377, and (from testing-3) a source archive.

No pakset is included.

### Source

`source/xr32-r12377/` is the complete source tree of testing-3: official trunk r12377 plus this test build's changes.
`patches/r12377-testing-3/` has the same changes as one full diff against official r12377, and as four steps on top
of testing-2 (`APPLY-ORDER.txt`; step 04 must not be applied without 01 and 03). testing-1's and testing-2's trees
stay at their tags, and their patches in `patches/r12377/` and `patches/r12377-testing-2/`.
`patches/r12377/upstream-build-fix/` is **not** part of XR32: it is the one-word fix (`old32` → `ole32`) that
official r12377's `CMakeLists.txt` needs before any Windows CMake build links.

### Known issues, certification and reports

See `docs/KNOWN-ISSUES-r12377-testing-3.md`, the release notes (`RELEASE-NOTES-r12377-testing-3.md`) and the
certification report `reports/XR32-DIRTY-REGIONS-CERTIFICATION-01.md` (companion reports in
`reports/xr32-dirty-regions-cert-01/`, evidence in `evidence/xr32-dirty-regions-cert-01/`).

New in testing-3's known issues, from the official trunk itself: after a live theme change the compass and one small
toolbar element can keep their old picture until they are redrawn; the official 16-bit renderer leaves exactly the
same pixels stale.

Report problems at <https://github.com/Dkijas/simutrans-true32-renderer/issues>. Please include:

- the release tag;
- your OS and package (Windows or Linux, SDL2 or SDL3);
- the pakset;
- whether `SIMUTRANS_XR32=1` was set;
- a PNG screenshot.

---

*The section below describes the previous test build, testing-2 on trunk r12377. Its release and its tag stay
unchanged; the section is kept here for history.*

## Previous test build: testing-2 on trunk r12377

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

> **Warning, from the official trunk r12377 itself:** founding a city whose first road runs onto a road with a
> private-way sign crashes the game, in both programs of this build. See `docs/KNOWN-ISSUES-r12377.md`, item B1.

### Two switches, two programs

| Switch | Program | What it does |
| --- | --- | --- |
| `STLAB_XR32_WRITE=1` | **makeobj** | writes 32-bit (XR32) image data into the paks it builds, next to the ordinary 16-bit images |
| `SIMUTRANS_XR32=1` | **simutrans-rgba32** | reads and draws that 32-bit data |

A pak built with `STLAB_XR32_WRITE=1` also loads in the other builds we tested, which ignore the extra data: the
official Simutrans r12377 and the 16-bit build. To *see* it, you need this build's `simutrans-rgba32` **and**
`SIMUTRANS_XR32=1`.

To check that XR32 is really on, start once with `-log -debug 2` added to the command line. If the variable is
**not** set, the file `simu.log` in your Simutrans user folder lists
`Warning: pakset_manager_t::read_nodes: Skipping unknown XR32-node` once for every 32-bit image. With
`SIMUTRANS_XR32=1`, that line does not appear.

### What this build is

- **Simutrans trunk r12377**, the same trunk as testing-1.
- **The XR32 DAY + optional NIGHT prototype of testing-1, consolidated:** five fixes that make the 32-bit renderer
  behave like the 16-bit one, and no new graphics feature. The prototype, its pak syntax and makeobj are unchanged.
- **Changes since testing-1:**
  - 32-bit interface images (the toolbar) keep their day colours at night; the map's night is unchanged;
  - 32-bit images have exactly the 16-bit size and position at every zoom level, and catenary wires keep the 16-bit
    pixels; this costs about 1 ms on the first frame after a zoom change;
  - two objects with identical 16-bit images but different 32-bit pictures each show their own;
  - interface images drawn while the map is zoomed use their 32-bit art;
  - small images that do not scale with the zoom are placed as in 16-bit.
- testing-1's known issue A6 (the 3/2 picture depending on the zoom step before it) could not be reproduced; it is
  listed as not reproduced, not as fixed.

### Downloads

Releases of this repository contain:

- Windows (SDL2 and SDL3) and Linux (SDL2 and SDL3) builds, each with `simutrans-rgba32` (32-bit, XR32) and
  `simutrans-16bit` (the normal renderer, for comparison);
- the matching makeobj for Windows and Linux;
- a test kit;
- the full source diff against trunk r12377.

No pakset is included.

### Source

`source/xr32-r12377/` is the complete source tree of testing-2: official trunk r12377 plus this test build's changes.
`patches/r12377-testing-2/` has the same changes as one full diff against official r12377, and as five steps on top of
testing-1 (`APPLY-ORDER.txt`). testing-1's tree stays at its tag, and its patches in `patches/r12377/`.
`patches/r12377/upstream-build-fix/` is **not** part of XR32: it is the one-word fix (`old32` → `ole32`) that
official r12377's `CMakeLists.txt` needs before any Windows CMake build links.

### Known issues, certification and reports

See `docs/KNOWN-ISSUES-r12377-testing-2.md`, the release notes (`RELEASE-NOTES-r12377-testing-2.md`) and the
certification report `reports/XR32-CONSOLIDATION-01.md` (evidence in `evidence/xr32-consolidation-01/`).

Report problems at <https://github.com/Dkijas/simutrans-true32-renderer/issues>. Please include:

- the release tag;
- your OS and package (Windows or Linux, SDL2 or SDL3);
- the pakset;
- whether `SIMUTRANS_XR32=1` was set;
- a PNG screenshot.

---

*The section below describes the previous test build, testing-1 on trunk r12377. Its release and its tag stay
unchanged; the section is kept here for history.*

## Previous test build: testing-1 on trunk r12377

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

> **Warning, from the official trunk r12377 itself:** founding a city whose first road runs onto a road with a
> private-way sign crashes the game, in both programs of this build. See `docs/KNOWN-ISSUES-r12377.md`, item B1.

### Two switches, two programs

| Switch | Program | What it does |
| --- | --- | --- |
| `STLAB_XR32_WRITE=1` | **makeobj** | writes 32-bit (XR32) image data into the paks it builds, next to the ordinary 16-bit images |
| `SIMUTRANS_XR32=1` | **simutrans-rgba32** | reads and draws that 32-bit data |

A pak built with `STLAB_XR32_WRITE=1` also loads in the other builds we tested, which ignore the extra data: the
official Simutrans r12377 and the 16-bit build. To *see* it, you need this build's `simutrans-rgba32` **and**
`SIMUTRANS_XR32=1`.

To check that XR32 is really on, start once with `-log -debug 2` added to the command line. If the variable is
**not** set, the file `simu.log` in your Simutrans user folder lists
`Warning: pakset_manager_t::read_nodes: Skipping unknown XR32-node` once for every 32-bit image. With
`SIMUTRANS_XR32=1`, that line does not appear.

### What this build is

- **Simutrans trunk r12377**: the current development version. Every official change from r12338 to r12377 is
  included, among them r12368 (toolbar icons are fitted to the theme once, when the toolbars are built) and r12361
  (music on Windows uses Media Foundation, so Windows 8 or newer is required).
- **The XR32 DAY + optional NIGHT prototype** of the r12337 test build, unchanged: 32-bit images can carry an optional
  night picture and explicit company colours. Old paks keep the normal rendering path.
- **Changes since the previous test build (r12337):**
  - moved to r12377; the 32-bit renderer implements the renderer function r12371 added, and draws r12342's line break
    in single-line text as a space;
  - authored double-size (MRES) toolbar icons work on Linux file systems too (they were lost when another object had
    the menu's name, as the cursor does in Frank's pak64.german);
  - authored double-size icons keep their double-size art after the map is zoomed in and out.

### Downloads

Releases of this repository contain:

- Windows (SDL2 and SDL3) and Linux (SDL2 and SDL3) builds, each with `simutrans-rgba32` (32-bit, XR32) and
  `simutrans-16bit` (the normal renderer, for comparison);
- the matching makeobj for Windows and Linux;
- a test kit;
- the full source diff against trunk r12377.

No pakset is included.

### Source

`source/xr32-r12377/` is the complete source tree: official trunk r12377 plus this test build's changes.
`patches/r12377/` has the same changes as one full diff and as five separate steps (`APPLY-ORDER.txt`).
`patches/r12377/upstream-build-fix/` is **not** part of XR32: it is the one-word fix (`old32` → `ole32`) that
official r12377's `CMakeLists.txt` needs before any Windows CMake build links.

### Known issues and reports

See `docs/KNOWN-ISSUES-r12377.md` and the release notes (`RELEASE-NOTES-r12377-testing-1.md`).

Report problems at <https://github.com/Dkijas/simutrans-true32-renderer/issues>. Please include:

- the release tag;
- your OS and package (Windows or Linux, SDL2 or SDL3);
- the pakset;
- whether `SIMUTRANS_XR32=1` was set;
- a PNG screenshot.

---

*The section below describes the previous test build, on trunk r12337. Its release and its tag stay
unchanged; the section is kept here for history.*

## Previous test build: DAY+NIGHT on trunk r12337

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

### Two switches, two programs

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

### What this build is

- **Simutrans trunk r12337**: the current development version, including r12336's extra zoom-in levels (the zoom is
  now shown in the display settings) and r12337's network fix (a socket is no longer reused after a failed
  connection).
- **The XR32 DAY + optional NIGHT prototype:** 32-bit images can carry an optional night picture and explicit company
  colours. Old paks keep the normal rendering path.
- **Fixes since the previous test build (r12322):**
  - 40 px toolbar icons (for example Frank's pak64.german) get 40 px buttons;
  - 32-bit toolbar icons follow `icon_scaling` (theme-large 150 % → 60 px);
  - very thin 32-bit images (catenary poles) are no longer drawn too big and in the wrong place when zoomed far out.

### Downloads

Releases of this repository contain:

- Windows (SDL2 and SDL3) and Linux (SDL2) builds, each with `simutrans-rgba32` (32-bit, XR32) and `simutrans-16bit`
  (the normal renderer, for comparison);
- the matching makeobj for Windows and Linux;
- a test kit;
- the full source diff against trunk r12337.

No pakset is included.

### Source

`source/xr32-r12337/` was the complete source tree of that build: official trunk r12337 plus its changes. It
is kept in the tag `xr32-prissi-night-image-prototype-01-r12337-testing-1`.
`patches/r12337/` has the same changes as one full diff and as four separate steps (`APPLY-ORDER.txt`).

### Known issues and reports

See `docs/KNOWN-ISSUES-r12337.md` and the release notes (`RELEASE-NOTES-r12337-testing-1.md`).

Report problems at <https://github.com/Dkijas/simutrans-true32-renderer/issues>. Please include:

- the release tag;
- your OS and package (Windows SDL2 / SDL3, Linux);
- the pakset;
- whether `SIMUTRANS_XR32=1` was set;
- a PNG screenshot.


---

*The text below is the original description of this repository's first candidate (against trunk r12254). It is kept
for history; for the current test build, the first section of this file applies.*

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
