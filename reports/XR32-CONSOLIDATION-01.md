# XR32-CONSOLIDATION-REPORT — SIMUTRANS-XR32-CONSOLIDATION-01

> **Publication note, 2026-11-01 (added when this report was published with testing-2).** The candidate below was
> published as `xr32-prissi-night-image-prototype-01-r12377-testing-2`, on Victor's authorisation, after a second
> certification of the **packages themselves** (`evidence/xr32-consolidation-01/packages/`):
> - the executables extracted from the staged packages are byte-identical to the certified builds, and every package
>   has the same members as testing-1's, differing only in the executables, the diff and the texts; the Linux 16-bit
>   executables and the Linux makeobj are byte-identical to testing-1's;
> - the suite and gate battery ran again on the extracted packages, with testing-1 as the control, and gave the
>   certified results on Windows SDL2/SDL3 and Linux SDL2/SDL3 (shared images 0 failures against 6, interface at
>   night 0.973 against 0.341, zoom geometry, catenary 122/122, A6 routes 0, 309/309, clean shutdown 27/27 on
>   Windows and 3/3 per Linux build);
> - the full diff was applied to a real `svn export -r 12377 --native-eol LF` and gives the source tree byte for
>   byte. The text below, written before the packaging, says this route was checked; at that time it had been
>   checked against the git mirror of r12377 only. The SVN route is now measured (`packages/patches-routes.txt`);
> - the test kit's 13 paks rebuilt by both makeobj builds are byte-identical to testing-1's.
>
> The text below is otherwise the laboratory report as written, with local paths replaced by placeholders. Its
> "NOT_AUTHORISED" refers to the time it was written, before the publication was authorised.

**Decision: READY as the next experimental XR32 build. Publication: NOT_AUTHORISED.** Nothing was pushed, tagged,
released, proposed or committed to SVN; the r12377 testing-1 release is untouched.

| | |
| --- | --- |
| Base | published testing-1: commit `aa15669b` (tag `base-testing1` in the lane), tree `cdd101f7` = `source/xr32-r12377/` of `xr32-prissi-night-image-prototype-01-r12377-testing-1`; Simutrans SVN trunk **r12377** (live HEAD re-checked 2026-11-01: still r12377) |
| Candidate | branch `xr32/consolidation-01` = **`42c5d6237d030377c3e273ee6177d0bfc713bfe1`**, tree `3a194065…`, six commits on the base (I1, I2, I3, I3b, I5, I6), local only |
| Lane | `<lab>` (git clone `src/`; WSL work dir `<wsl-lab>`) |
| Change | 6 files, +638 / −281: `dataobj/pakset_manager.cc` (+4), `display/simgraph32.cc`, `display/stlab_xr32.h`, `display/stlab_xr32_hook.cci`, `display/stlab_xr32_load.cci`, `display/stlab_xr32_zoom.cci`. No 16-bit file, no makeobj file, no trunk file outside the XR32 seam |
| Trunk patches | B, C, D, E, G **not** in any diff; H (`old32` → `ole32`, r12362) applied to the Windows build copy only, as in testing-1 |

## Results at a glance

| Id | Issue | Verdict | testing-1 → final (each with a failing control) |
| --- | --- | --- | --- |
| A3 | identical 16-bit, different XR32: the last pairing won | **FIXED (I1), CERTIFIED** | identity fixture 6 FAIL → 0 on Win SDL2/SDL3, Linux SDL2/SDL3 |
| A5 | 32-bit interface images dark at night | **FIXED (I2), CERTIFIED** | toolbar night/day 0.341 → 0.973 (= 16-bit); 112 → 0 interface images changed by the night; world night byte-identical |
| N1 | interface showed 16-bit art while the world is zoomed | **FIXED (I3 + I3b), CERTIFIED** | 8/8 16-bit → 8/8 XR32 |
| A1 | XR32 image smaller than 16-bit at some zooms | **FIXED (I5 + I6), CERTIFIED** | size = 16-bit: 16–40 % → 100.00 % of 11 735 images × 12 zooms; fit parity → 278/278 (Win), 281/281 (Linux) |
| A2 | catenary wire drawn thinner / other rows | **FIXED (I5), CERTIFIED** | lit wire px at 9/8 68 557 → 105 607 (16-bit 105 601); 122/122 boxes |
| N5 | non-zoomable images displaced | **FIXED (I6), CERTIFIED** | x/y differ 25 → 0 |
| A6 | picture depends on the zoom route | **NOT_REPRODUCIBLE** | 0 differ on 11 025 images, control differs |
| A4 | coast tiles stay 16-bit | OPEN, DESIGN_REQUIRED | new graphics feature, out of the brief |
| A7 / A8 | night memory / DAY-NIGHT pak syntax | OPEN by design | the consolidation adds no measurable memory (phase 5) |
| N2 / N3 / N4 | merge only within one pak file / MRES split / latent frees | OPEN, latent | not reachable defects; documented |

Full table: `deliver/INCIDENT-MATRIX.md`. Patches: `deliver/patches/` (below).

## Phase 0 — baseline and premises

- Vault read: `03_STLAB` XR32 notes up to `STLAB-XR32-R12337-PRERELEASE-PREPUBLISH-01`; the r12377 testing-1 manifest
  (`<testing-1-lab>/staging/NIGHT-PROTOTYPE-MANIFEST.md`).
- Premises checked: trunk HEAD r12377 (unchanged at the end of the mission); the published source = `aa15669b`; every
  function the brief names exists; none of A1–A8 already fixed upstream (the XR32 code is not in trunk).
- The testing-1 binaries used as BASE are the published ones, byte for byte: Linux SDL2 32-bit `5ca3357c…` = the
  release tarball; Windows SDL2 `f5db98c5…` (32) and `1aeca14a…` (16) = the release zip.

## Phase 1 (P0) — A3, shared image identity — I1

- **Reproduction.** A pak fixture (`tools/gen_identity_fixture.py`, objects IdA–IdL × 8 directions) covers every case
  of the brief: identical 16-bit with different XR32 day (B vs A), identical XR32 (C = A), different night (E vs D),
  night vs none (D vs A), different player-colour runs on the same 16-bit image (K vs G), different coverage (H), and a
  16-bit-only object (L). testing-1 fails 6 checks: B draws A's picture, E draws D's night, K draws G's player colour,
  and L draws IdA's XR32 picture although it has none. A lab-only audit build (`-DSTLAB_IDENTITY_AUDIT`, never in the
  product) found the same conflict in real paks: kit 2/3 slots, Frank + MRES 2.
- **Root cause.** The image reader merges images whose 16-bit data are identical (adler32 + memcmp,
  `image_reader.cc`); XR32 data is paired by image id; `stlab_xr32_attach` deleted the node already attached, so the
  last slot read (directory order) won.
- **Fix (I1).** Per image slot: the same XR32 data shares the image; the first slot attaches; any other slot gets a
  twin — `image_t::copy_image` + `register_image` — and its descriptor's child points to the twin. A 16-bit-only slot
  never inherits another slot's XR32 (`stlab_xr32_has_authored` / `stlab_xr32_shows`). XR32 off: no twin, nothing
  changes.
- **Positive.** 0 failures on all four platform builds; real paks: 0 conflicts left (kit +5 images, Frank + MRES +1,
  pak64 0 twins). The 16-bit pictures with XR32 off are unchanged (sanity check of the fixture).

## Phase 2 (P1) — A5, interface at night — I2

- **Reproduction.** `uiday` draws XR32 images through `display_color_img(…, daynight=false)` at night levels 0–13:
  testing-1 changes 112 of 144 (image × player × level) pictures; toolbar brightness night/day 0.341 (16-bit 0.973).
- **Root cause.** `simgraph32_draw_color_img` served XR32 images from the world's night-resolved cache and ignored
  `daynight=false`.
- **Fix (I2) — semantic, no names.** The caller's request itself (`daynight=false`) selects the day context: a
  thread-local flag read by the four seam lookups (night multipliers, night level, player colour, light colour), and a
  separate day-resolved recode of the image. Who asks for `daynight=false` was measured over the whole source
  (`evidence/a5-daynight-callers.txt`): only `gui/`, the loading screen and the title screen — never the world.
- **Positive.** ui-day 0 differ; toolbar 0.973 (= 16-bit); world `night.csv` byte-identical to testing-1, world still
  darker at level 13 (16/16): the map's night lighting is untouched.

## Phase 3 (P1) — A1, A2, A6 (+ N1, N5)

- **N1 (found in this mission), I3 + I3b.** At a world zoom ≠ 1/1 the interface's base-size draw
  (`simgraph32_draw_base_img(_blend)`) decoded `base_data`, the 16-bit picture. I3 keeps 1:1 XR32 runs per image in
  their own cache (apart from the world's zoom cache); I3b draws them under the recode lock, because a palette change
  frees them (a use-after-free risk found in the architecture review). 8/8 images 16-bit → 8/8 XR32 at 3/8.
- **A1 / A2, I5.** Deterministic census over every image of Frank's pak64.german at 12 zoom steps, XR32 against the
  16-bit resampler of the same build (`tools/a1_gates.sh`, `a1_align_check.py`; I1 twins shift ids, so images are
  aligned by sequence). Root cause: `stlab_xr32_zoom_build` sized `floor(w·num/den)` from a truncated origin without
  `zoom_core_rezoom`'s offset remainder (`x_rem`) and padded box, and its box-coverage threshold dropped thin wire
  pixels. I5 uses exactly the legacy geometry and sampling grid — box `ceil(w/den)·num`, sample position
  `(i·den + k − x_rem)/num` (C division), den² samples, a pixel drawn if any sample is opaque — and keeps XR32's own
  8-bit colour accumulation. **The 16-bit algorithm is not touched.**
- **N5 (found), I6.** 25 small non-zoomable images were placed at other x/y than in 16-bit: `stlab_xr32_zoom_prepare`
  scaled them, the recode reset w/h but left x/y scaled. I6 keeps their base geometry.
- **Positive.** Sizes = 16-bit 100.00 % at all 12 steps, x/y differ 0; lit-pixel box identical 99.2–99.5 % (the rest:
  1–2 px on the left/bottom edge of 6–9 of 1 173 sampled images; cause not isolated — the 8-bit colour
  accumulation is the likely one, not proven); fit
  parity 278/278 and 281/281 (testing-1's one known-issue icon, 60×58 against 60×60, gone); catenary 122/122 boxes at
  every step, lit px within 0.01 %; thin images: 0 drawn where 16-bit hides them.
- **A6: NOT_REPRODUCIBLE.** The game's own zoom, one step at a time, reaches 3/2 and 5/4 by two routes (from the
  zoomed-out side and through 2/1): 0 pictures differ on catenary, the DAY/NIGHT kit and 10 895 Frank + MRES images —
  testing-1 and final alike — while the control (3/2 against 5/4) differs for 117 / 8 / 10 412 images. At the map
  level the remaining differences are world noise, identical with XR32 off.

## Phase 4 — Linux SDL3

| Point | Result | Where | Level |
| --- | --- | --- | --- |
| backend selection | SDL3 picks **x11** on its own; `SDL_VIDEODRIVER=wayland` works; dummy works | WSLg (Weston + XWayland, Windows 11, 3440×1440, scale 1.0) | real session |
| window | 704×560, renderer opengl, display scale 1.0, pixel density 1.0 | WSLg | real session |
| fullscreen | `dr_toggle_borderless` on and back, sizes follow | WSLg x11 + wayland | real session |
| resize | `SDL_SetWindowSize` 1024×700, the game follows | WSLg x11 + wayland | real session |
| mouse wheel | one zoom step per notch with real XTEST input (`xdotool`); the first notch after activating the window is lost on **SDL2 too** (not XR32, not SDL3) | WSLg x11 | real session |
| clean shutdown | rc 0 on every driver: dummy 0.32 s / 49 MB, x11 0.86 s / 155 MB, wayland 1.09 s / 179 MB | WSLg | real session |
| resources | static SDL3 3.4.12; X11/Wayland/ALSA/PulseAudio/PipeWire loaded at run time (`tools/sdl3_symbols.sh`) | — | measured |
| suite + gates | 309/309, control FAIL, 26 gate runs clean, logcheck 4/4, shutdown PASS ×3 | Linux SDL3 | dummy driver |

**Not tested (declared, not inferred):** HiDPI with a scale > 1 (WSLg runs at 1.0), the wheel under Wayland (no
synthetic input there), a native Linux desktop (only WSLg), multi-monitor (one display), any GPU driver other than
WSLg's.

## Phase 5 — performance and memory against testing-1

**Separating XR32 from trunk.** testing-1 and the final candidate sit on the same trunk, r12377, built with the same
recipe, so every difference between them is XR32's own (I1–I6); `legacy` (testing-1 with XR32 off) shows what XR32
itself costs in testing-1. Runs interleaved inside every repetition; machine otherwise idle; Linux = WSL 2, SDL dummy
video; Windows = SDL dummy video. Each row of every CSV carries the sha256 prefix of the binary that ran, and the
summaries refuse a row whose hash does not match its label.

**Incident, declared.** The first idle run and the earlier fps500 run copied every build onto one shared `perf-exe`;
`cp` hit ETXTBSY ("Text file busy") 3 and 1 times and those runs used the other build under the wrong label. Both are
void (`evidence/void-etxtbsy/`), the scripts now copy one file per build once and fail closed
(`tools/fix_perf_exe.py`); the figures below are the rerun, provenance-checked. The negative control of the checks:
the void CSV is rejected (`PROVENANCE FAIL`).

| Measure | testing-1 XR32 off | testing-1 XR32 on | final XR32 on | final − testing-1 (paired) | Evidence |
| --- | ---: | ---: | ---: | --- | --- |
| zoom walk, sum of the 9 first frames after a zoom change, Linux SDL2, Frank's pak64.german, `-fps 500`, n = 7 | 91.0 ms (89.3–98.3) | 96.0 ms (92.4–103.4) | 105.2 ms (98.9–107.6) | **+8.7 ms median (+9.0 %), +2.8 … +15.0, higher in 7 of 7** | `perf-zoom-final.txt` |
| full redraw without rebuild, all steps pooled | 7.4 ms | 7.5 ms | 7.5 ms | no measurable difference | same |
| load + quit wall, pak128.german-cat, n = 5 | — | 0.697 s (0.694–1.539) | 0.747 s (0.694–0.800) | −0.002 s median, higher in 2 of 5: **no measurable difference** | `perf-load.txt` |
| peak RSS, pak128.german-cat | — | 463 MB (461–468) | 463 MB (461–467) | +0 MB median: **no measurable difference** | same |
| peak working set, Windows SDL2, 1950 save to 1951.3, n = 3 | 559.4 MB | 563.3 MB | 563.4 MB | +0.0 MB median (−1.1 … +0.4): **no measurable difference** | `shutdown-mem.txt` |
| clean shutdown, Windows SDL2 + SDL3, 9 rows × 3 | — | — | — | **27 / 27 PASS** (rc 0, no FATAL / Error) | `shutdown-mem.csv` |

- **The one real cost: about +1 ms on the first frame after each zoom change** (+9 % of the walk), from I5 sampling
  den² points per output pixel exactly as the 16-bit resampler does, instead of testing-1's cheaper box. It is paid
  once per visible image and zoom step (the zoomed image is cached), so frames without a zoom change are unchanged
  (redraw 7.5 ms in both). Per step the ranges overlap; the walk totals do not, and the sign is the same in 7 of 7
  repetitions, so the cost is real but small.
- **Load and memory: no measurable difference.** The Linux wall times are quantised (≈ 53 ms steps, e.g. 0.267 /
  0.321 s), so differences below one step are not resolvable; pak64.german and the small `pak` show bimodal peak RSS
  (±15 MB) that follows neither build. The I1 twins (a few images per pakset) are below the noise.
- **A7 (night planes):** the consolidation adds no measurable memory; the cost of night planes themselves was not
  re-measured (pak128.german-cat carries catenary XR32 only). XR32 on vs off on that pakset: +3.8 MB (final) / +3.9 MB
  (testing-1) working set; against the 16-bit build +8.3 MB in both.
- **Noise statement:** n = 7 (zoom) / 5 (load) / 3 (Windows memory); WSL 2 and Windows desktop timing; no CPU pinning;
  medians and full ranges reported; sub-quantum deltas are reported as no measurable difference.

## Phase 6 — compatibility matrix (final binaries)

All builds fresh and non-incremental from `42c5d62`, Release, MT. Windows: MSYS2 MinGW-w64 GCC 16.1.0, CMake 4.4.0
(`logs/pub-win-final.txt`); Linux: Debian 13 GCC 14.2.0, CMake 3.31.6 (`logs/linux-matrix-build-final.txt`).

| Build | SHA-256 | Build | Suite (control) | Gates | Other |
| --- | --- | --- | --- | --- | --- |
| Win SDL2 rgba32 | `45edf0ad…` | rc 0 | 309/309 (control FAIL) | 43/43 clean | logcheck 7/7; identity, A5, N1 |
| Win SDL2 16-bit | `6a3a9742…` | rc 0 | 309/309; official r12377 16-bit 309/309 | — | logcheck |
| Win SDL3 rgba32 | `30a86015…` | rc 0 | 309/309 (control FAIL) | 43/43 clean | logcheck 7/7; identity, A5, N1 |
| Win SDL3 16-bit | `a3b66376…` | rc 0 | 309/309; official 309/309 | — | logcheck |
| Win makeobj | `e8d3bcf7…` | rc 0 | — | — | warnings set = testing-1 |
| Linux SDL2 rgba32 | `fc04d444…` | rc 0 | 309/309 (control FAIL) | 26 runs clean | logcheck 4/4; shutdown PASS ×3; identity, A5, N1, A6, A2, A1 box |
| Linux SDL2 16-bit | `3e7cd80e…` | rc 0 | 309/309 | — | **byte-identical to testing-1** |
| Linux SDL3 rgba32 | `7d4b36d2…` | rc 0 | 309/309 (control FAIL) | 26 runs clean | logcheck 4/4; shutdown PASS ×3; identity, A5, N1, A6, A2, A1 box |
| Linux SDL3 16-bit | `9822aee1…` | rc 0 | 309/309 | — | **byte-identical to testing-1** |
| Linux makeobj | `7646fe25…` | rc 0 | — | — | **byte-identical to testing-1** |

testing-1 guarantees re-checked on the final binaries:

| Guarantee | Result |
| --- | --- |
| 16-bit renderer and makeobj unchanged | Linux 16-bit and makeobj byte-identical to testing-1; no 16-bit or makeobj source touched |
| Compiler warnings | sets (file + message + flag) identical to testing-1 on every Windows build (21, makeobj 2); Linux 32 = 16 (16); control with an injected warning detected (`evidence/warning-sets.txt`) |
| XR32 off = 16-bit pictures | skip lines and pictures as 16-bit (logcheck, `seadraw`/`mutantdraw` off: 0 XR32 colour) |
| DAY/NIGHT | 192 image × player series on Win SDL2/SDL3 + Linux SDL2, 64 on Linux SDL3: 0 failures; control (XR32 off) finds no night and fails as it must |
| MRES 2x | 6 400 / 6 400 pixels = the authored 2x art at 200 % on all four |
| alpha fit sequence | 192 steps on the four final builds (scale 100 and 125, XR32 on and off), 0 failures |
| thin catenary (r12337 fix) | 0 drawn where 16-bit hides them, all four |
| makie sea | drawn from its 32-bit planes (mutant colour on/off control) |
| toolbar band at world zoom | 0 px differ at 1/1, 3/8 and back (XR32 off, without MRES, reports 6 400 — the reported control) |
| clean shutdown | Linux 3/3 per backend; Windows SDL2 + SDL3 27/27 (`shutdown-mem.txt`) |
| ASan/LSan | final source (build `96a43fad…`, Linux SDL2): **0 AddressSanitizer errors in 6 runs** — suite 309/309, kit zoom + interface at night + base draw, identity pak with twins to 1.1, `drawall` of every image at 2/1, 7/4, 4/3, 1/2 or 3/8 and 1/4 on Frank + MRES (10 895) and the catenary pak (122), the 1950 world to 1951.3; LSan: the same 87 / 45 leak records as I3 (±1 byte), none in an `stlab_*` function (two at start-up in `simgraph32_init` / `simgraph32_load_font`, kept to exit) (`logs/asan-fin.txt`) |

## Phase 7 — architecture review

Done by a review subagent and verified here; claims I could not verify are marked.

- **Verified and fixed:** I3's base-size runs could be freed by a palette change while drawn → I3b.
- **Verified, latent, not fixed (N4):** `stlab_xr32_release_all` has no caller; `multires_objects` is not freed on
  an early return of `resolve_multires`; `simgraph32`'s `free_all_images_above` does not free `zoom_data` (its only
  caller runs once per process with nothing to free). None grows; LSan shows no leak through the XR32 seam.
- **Duplication 16/32, measured here:** of 1 367 non-trivial lines of `simgraph32.cc`, 421 also exist verbatim in
  `simgraph16.cc`, and only 281 lie in shared blocks of ≥ 8 lines. The subagent's "1.7–1.8 k duplicated lines" is not a
  measured figure; it can only be structural similarity (the same functions over another pixel type), not copies.
- **Seams:** XR32 is free of SDL (the backends are untouched by I1–I6); its entry points are the image reader pairing
  (`stlab_xr32_load.cci`), the pakset finish (`pakset_manager.cc`), the night/colour lookups (`stlab_xr32_hook.cci`)
  and the zoom builder (`stlab_xr32_zoom.cci`).
- **For a later GPU path (not started, per the brief):** the natural cut is after the XR32 picture is resolved
  (night + player colour) and before it is encoded into runs; everything up to there is renderer-independent.
- **Trunk integration risks:** the image-id twins (I1) change image numbering when XR32 is on (ids are assigned at
  load; the identity pak, which has twins, loads a save and plays to 1.1 cleanly, also under ASan), so any tool
  comparing ids across XR32 on/off must align by sequence; `daynight=false` is now a semantic contract the GUI
  relies on.

## Deliverables

| File | SHA-256 | Content |
| --- | --- | --- |
| `deliver/patches/01-I1-A3-XR32-IMAGE-IDENTITY.diff` | `3e9cd2de…` | I1 (4 files) |
| `deliver/patches/02-I2-A5-UI-DAYTIME.diff` | `e5d51065…` | I2 (2 files) |
| `deliver/patches/03-I3-N1-UI-BASE-SIZE.diff` | `83f7aac7…` | I3 + I3b (2 files) |
| `deliver/patches/04-I5-A1-A2-ZOOM-GEOMETRY.diff` | `04075aae…` | I5 (1 file) |
| `deliver/patches/05-I6-N5-NONZOOMABLE.diff` | `5d5fe6f6…` | I6 (1 file) |
| `deliver/patches/CONSOLIDATION-ON-TESTING-1.diff` | `841ca071…` | 01–05 in one, on testing-1 (6 files) |
| `deliver/patches/XR32-FULL.r12377-consolidation-01.diff` | `eaf4ee99…` | official r12377 → final (51 files; its 20 CR bytes come from upstream files) |

All three routes — 01..05 in order, the consolidation diff on testing-1, the full diff on `svn export -r 12377` —
give tree `3a194065…` byte for byte (`tools/mk_patches.sh`). Evidence: `evidence/`; logs: `logs/`; harness: `tools/`.

## Unresolved

- A4 (coast tiles), A7, A8: by design / out of the brief.
- N2 (16-bit merge only within one pak file): trunk behaviour, cause not established.
- N3 (MRES split loses XR32): latent, needs an MRES fixture with two slots sharing a 1x and wanting different 2x.
- N4: latent frees (above).
- Not testable here: HiDPI > 1, Wayland wheel, native Linux desktop, multi-monitor, macOS, ARM.

## Recommendation for the next experimental version

1. **Use `42c5d62` as the next experimental XR32 build** ("testing-2" on r12377, or on the then-current trunk HEAD
   after a rebase and a rerun of this matrix): testing-1 + I1–I6, nothing else. It fixes the visible defects declared
   in testing-1 (A5 dark toolbar at night, A1/A2 smaller pictures and thinner wire when zoomed) and two found here (N1, N5), and
   makes shared images safe (A3). Same package matrix as testing-1: Windows SDL2/SDL3, Linux SDL2/SDL3, makeobj.
2. **Before any publication:** a new explicit authorisation (none exists); package and gate the archives themselves
   (the gates here ran on the build outputs); re-check trunk HEAD.
3. **Release notes should say:** the zoom change costs ~1 ms more on its first frame (exact 16-bit geometry); image ids
   differ with XR32 on (twins); not tested: HiDPI > 1, Wayland wheel, native Linux desktop, multi-monitor, macOS, ARM.
4. **Next engineering, in order:** an MRES fixture for N3; then, only if the +9 % matters, a cheaper exact resampler for
   I5 (separable sampling over the same grid is the obvious candidate; equivalence to be proven by the A1/A2 census); A4 coast tiles need a design decision (new graphics feature); N2 is a
   trunk question for the 16-bit reader.

## Processes

Non-lab processes terminated: 0.
