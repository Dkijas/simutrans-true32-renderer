> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# testing-6 candidate — Windows test matrix (plus the Linux minimum)

**Candidate:** `f04638a`.
- Windows: SDL3 `ed58e66d`, SDL2 `b002d5a9`, 16-bit `8feaaf49` / `20cbf7da`, makeobj `8a150381`.
- Linux: SDL2 `a3eef27e`, SDL3 `7a094d8b`.

**BASE:** testing-5 (`038b538`), SDL3 `f7f7ce82` (= the published executable), SDL2 `e82a2ce8`.

**Machine:** Windows 11 Pro x64, one 3440×1440 display.

**Legend:** PASS = shown, with its control failing.

| # | Check | SDL3 | SDL2 | Evidence |
| --- | --- | --- | --- | --- |
| A1 | candidate = public testing-5 + `4c71953` + `f04638a` (two independent routes; control) | PASS | | `A-provenance.txt` |
| A2 | XR32 format: no reader / writer / loader file touched; makeobj byte-identical | PASS | | `A-provenance.txt`, `G-builds-vs-certified.txt` |
| G1 | fresh builds == A12-certified hashes (11 of 11), BASE == published testing-5 | PASS | PASS | `G-builds-vs-certified.txt` |
| G2 | warning sets BASE = candidate (also the lab twins) | PASS | PASS | `G-warning-sets.txt` |
| G3 | 16-bit renderer and makeobj unchanged (byte-identical to BASE builds) | PASS | PASS | same |
| G4 | Windows packages derived from the published testing-5 zips: zipper reproduces both published zips; only the expected paths change; the 16-bit file is the candidate's own build (testing-5's packaged file was not byte-identical: replaced, not reused) | PASS | PASS | `G-pkg-win.txt` |
| G5 | Microsoft Defender 1.463.110.0 (updated this mission): every candidate file and every packaged executable, 0 detections, 0 active threats | PASS | PASS | `G-defender.txt` |
| B1 | three configurations, same scenes (flat, slopes / bands / snow, cone), day, darkness 2, 4, and 2/1: banding gone, colours ×2–4, RGB555-grid signature 0 % (control: quantised A12 = 100 %) | PASS | (tile draws identical, D3) | `B-visual.txt`, `VISUAL-COMPARISON.md` |
| C1 | brightness = PNG × authored light exactly; testing-5 darker and hue-shifted; snow clipping authored and pre-existing; night applied once | PASS | | `C-luminosity.txt` |
| D1 | independent oracle, makie's 520 tiles: BASE == legacy, candidate == 8-bit model | PASS | PASS | `D-oracle.txt` |
| D2 | the same on the synthetic sources (ramps, near colours, dark, bright, transparent-key product), regenerated here identical to A12's | PASS | PASS | `D-oracle.txt`, `D-synthetic-regenerated.txt` |
| D3 | cross checks fail as required (BASE vs 8-bit, candidate vs legacy and vs C1) | PASS | PASS | `D-oracle.txt` |
| D4 | SDL2 vs SDL3: tile draws byte-identical (BASE and candidate, makie and synthetic) | PASS | | `D-oracle.txt` |
| D5 | every other XR32 image (50,299) drawn alone, by day **and at night (light level 4, day/night on: lights, night planes)**: 0 differ; only the 520 derived tiles added | PASS | — | `D-drawall.txt` |
| D6 | traditional pak 2.4, mutant pak (A4 water, A11 2/1), makie with XR32 off: 7 dumps each (5 zooms, 2 darkness levels), two runs per side: only the status-bar clock and a factory animation differ | PASS | PASS | `D-compat-frames.txt` |
| D7 | XR32 slot counts: traditional 0 = 0, mutant 2,134 = 2,134, makie +520 | PASS | | `D-compat-frames.txt` |
| D8 | screen updates (dirty regions), every 10th frame: 7 terrain scenarios + windows / playercolor: **0 of 2,160 frames mismatched**; control `droprect` caught in 109 of 240 | PASS | PASS | `D-verify.txt` |
| E1 | performance | see `PERFORMANCE.md` | | |
| H1 | Linux candidate built with the release recipe (recipe reproduces published testing-5 Linux); 16-bit and makeobj = testing-5's | built | built | `builds-linux.txt` |
| H2 | Linux minimum (WSL 2 / WSLg, NOT native Linux; not certified): ELF x86-64, libraries resolved, GLIBC ≤ 2.38, `-help`, dummy start and clean exit (mutant pak; makie's pak with the A12 path), WSLg X11 window, makeobj XR32 output == Windows | PASS | PASS | `H-smoke-linux.txt`, `H-smoke-linux-makie.txt`, `H-pkg-linux.txt` |

**Incidents** (all resolved; none changes a result):
- **H2:** the first makie run used the wrong target date (1950.8 for a save dated 5/1930) and timed out, with no FATAL. It was repeated to 1930.7: PASS on both backends.
- **A1:** the first cherry-pick attempt was void (empty base after a `commit-tree` date error). It was redone correctly.

**Not tested:**
- macOS, Android, ARM, a native Linux desktop, several displays;
- AddressSanitizer for A12;
- long runs of zooming (drift was measured for light changes only).
