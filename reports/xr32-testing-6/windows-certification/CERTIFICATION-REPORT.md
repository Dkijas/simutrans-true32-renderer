> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# SIMUTRANS-XR32-TESTING6-READINESS-01 — certification report

| | |
| --- | --- |
| **Technical verdict** | **B — CERTIFIED WITH LIMITATIONS** (Windows x64 SDL2 and SDL3) |
| **Readiness** | **READY_FOR_ARTIST_REVIEW**: technically ready for a release review, but the artistic review of the brighter terrain (makie) is pending and is not implied |
| **Publication** | **NOT_AUTHORISED**: no push, tag, release, SVN commit, forum or Discord post was made |
| Candidate | `f04638adc06130dc73121df50f9f2f0c89fd4e1d` (tree `b0efabef`) = testing-5 `038b538` + A12 `4c71953` + `f04638a`, on SVN r12377; frozen and **unchanged** |
| Lane | `<testing-6-lab>` (date 2026-11-03/04) |

## Answers to the brief

**1. Is the candidate still exactly `f04638a`?**
Yes.
- The public testing-5 tree, fetched anonymously (`cfcc03c6` → `c8297dca`), plus the two A12 commits gives tree `b0efabef` by two independent routes (cherry-pick, and the delivered patches applied to the index), with a control.
- Only 3 files change; no XR32 reader, writer or loader file is touched; makeobj is byte-identical.
- All 11 Windows builds reproduce the A12-certified hashes byte for byte (`A12-INTEGRATION.md`).

**2. Is A12's graphical improvement kept?**
Yes, on every scene and light level measured (`VISUAL-COMPARISON.md`).
- testing-5's terrain: 89–96 % on the RGB555 grid, 78–92 % of its colour steps multiples of 8 (banding).
- The candidate: 0.2–1 % and 0 %, with 2–4× the colours, by day, at dusk, at night and at 2/1.
- All 520 generated tiles match an independent model exactly, with makie's pak and with synthetic sources, on SDL2 and SDL3; the crosses fail.

**3. Does the higher brightness correspond to the original colours?**
Yes.
- XR32 data = PNG (100 % on the synthetic PNGs).
- On the flat tile of every climate the candidate equals PNG × the pak's own light map (124, 132, 132)/128 exactly.
- testing-5 was 9–14 % lower in red and 2–7 % lower in green and blue: darker and hue-shifted by rounding.

Clipping occurs only on snow (56 %, predicted by the formula and identical in testing-5). The night is applied once: the A12/testing-5 ratio is the same by day and at night. SDL2 = SDL3 (`evidence/C-luminosity.txt`).

**4. Are there visual artefacts?**
None found.
- Geometry is unchanged at 1/1, 2/1, 9/8, 3/4 and 1/2.
- The other 50,299 XR32 images are identical by day and at night.
- Screen updates: 0 of 2,160 checked frames mismatched.
- The transparent-key product is handled.

Known and accepted: the transition weights stay at 32 levels (`A12-D-TRANSITION-MASKS.md`).

**5. Is compatibility with testing-5 kept?**
Yes, within what was tested.
- Paks without XR32 terrain (pak128.german 2.4, the mutant pak) and makie's pak with XR32 off draw like testing-5. Only the status-bar clock and a factory animation differ between runs, and those also differ between two runs of the same executable.
- XR32 slot counts are unchanged except the +520 derived tiles.
- TCC, lights, night planes and A4 water: every one of the 50,299 XR32 images is identical. A11's 2/1 path is unchanged and the mutant pak's 2/1 frames are identical.
- The 16-bit renderer and makeobj are byte-identical; the XR32 format is unchanged.

This is not a claim of universal compatibility: one real pak with XR32 terrain was tested.

**6. Candidate performance?**
- Steady state (still, scroll, animated-water scroll): no measurable difference.
- Light changes and zoom steps cost **+0.4 to +2.2 ms per event** (median), up to +2.7 ms at P95/P99. That reproduces the first certification's +1.1 to +3.5 ms. The cost scales with the number of distinct derived tiles in view, about a third on a flat view.
- No cumulative regression over 3,000 frames.
- zoomx's lower P99 is **not** an improvement: A12 has more slow frames, but shorter peaks (`PERFORMANCE.md`).

**7. What does a cache rebuild cost?**
Per light change or zoom step, the visible derived tiles are recomposed and nothing is kept:
- pyramid view: ≈ 40 tiles, 0.33 Mpx (≈ 1.3 MB transient), 2–5 ms of composition, max 6.7 ms (lab counters, under gdb);
- flat view: ≈ 25 tiles, 0.8–1.4 ms.

A separate proposal, not applied, would remove most of the light-change part (A12-P1, `PERFORMANCE.md` §6).

**8. Is memory under control?**
Yes.
- Peak working set within ±1 MB of testing-5 in every scenario except very frequent zooming (+5 / +9 MB, ≈ 0.5 %).
- No growth over 3,000 frames of light changes. Long zoom runs were not measured.

**9. Are Windows SDL2 / SDL3 certified?**
Yes, **with limitations**:
- 32-level transition weights (accepted);
- the higher cost of light-change and zoom-step frames;
- one real pak; one display;
- no AddressSanitizer for A12.

Microsoft Defender 1.463.110.0: no detection on any candidate file or packaged executable.

**10. Are the experimental Linux binaries ready?**
Yes, as **Experimental / Not certified**.
- Built from the same tree with the release recipe; the recipe reproduces the published testing-5 Linux binaries.
- SDL2 `a3eef27e`, SDL3 `7a094d8b`; the 16-bit binaries and makeobj are identical to testing-5's.
- Minimal checks passed on WSL 2 / WSLg, which is not a native Linux desktop: libraries, `-help`, start and clean exit including makie's pak (the A12 path), one X11 window, makeobj output = Windows.

**11. Which artistic opinion is pending?**
makie's, on four points (`ARTIST-REVIEW-PENDING.md`); not requested yet, draft in `MAKIE-FORUM-DRAFT.md`:
- the brighter ground;
- the cyan-white clipped snow on lit slopes;
- any contouring from the 32-level transitions;
- whether the pak should be re-authored instead.

**No approval is implied by the absence of an answer.**

**12. Is testing-6 ready for a publication review?**
**Technically yes; artistically not yet.** The candidate is frozen, reproducible, packaged, hashed and scanned. Publication needs:
- (a) a decision on the artistic review, to wait for makie or to publish with the brightness clearly announced;
- (b) explicit authorisation of the release.

## Limitations (the "with limitations" of the verdict)

| | |
| --- | --- |
| L1 | Transition masks: 32 weight levels (accepted for testing-6). |
| L2 | Brighter terrain on paks with XR32 terrain: faithful to the source, artist review pending. |
| L3 | Light-change and zoom-step frames: +0.4 to +2.7 ms; frequent zooming: +5–9 MB peak. |
| L4 | Certification scope: Windows x64 only, one display, one real pak with XR32 terrain, synthetic sources, no ASan for A12. |
| L5 | Linux experimental (WSL / WSLg only). macOS, Android and ARM not tested. |

## Changes made in this mission (none to the candidate)

- **Packages:**
  - derived from the published testing-5 assets with their own recipes (the recipe controls reproduce the published bytes);
  - the Windows 16-bit file is the candidate's own build, because testing-5's packaged 16-bit file is not byte-identical to any build of the candidate tree.
- **Lab only, never in the candidate:**
  - gate steps: night drawing in `drawall`, a configurable gdb timeout;
  - a counters branch `lab/a12cnt`.

## Incidents

All recorded:
1. The first provenance cherry-pick ran on an empty base (`commit-tree` rejected the `@0` date). It is void and was redone; the patch route was always valid.
2. testing-5's packaged Windows 16-bit file is not byte-identical to any candidate build. It was replaced by the candidate's build, as the brief requires.
3. The first Linux makie smoke used the wrong target date (1950.8 for a 5/1930 save) and timed out, with no FATAL. It was repeated: PASS.
4. zoomx P99 looked like an improvement. It was analysed: it is a change in the distribution's shape, not an improvement.
5. The zoomx peak memory difference (+5 / +9 MB) was found while writing the notes; the notes were corrected.
