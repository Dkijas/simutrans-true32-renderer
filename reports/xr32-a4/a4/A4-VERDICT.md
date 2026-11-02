# A4 — verdict (SIMUTRANS-XR32-COAST-RENDERING-01)

## **B — CERTIFIED WITH LIMITATIONS**

Sloped-coast water now uses the pak's RGBA32 sea when `SIMUTRANS_XR32=1`.

- The picture is exactly what the game's own generation would produce from the 32-bit sea. It matches an independent
  model with 0 mismatches in 650 tiles over 3 water paks, and at night it is identical to the sea image at every
  light level.
- Nothing else changes:
  - the 16-bit renderer, which is byte-identical or the same program;
  - traditional paks, which are byte-identical;
  - the batteries, which match testing-3 line for line except the explained, expected lines;
  - dirty regions, with 0 px on every coast scenario on Windows and Linux;
  - clean shutdown (48 of 48), sanitizers (0 new) and warning sets (equal).

## Why B and not A

1. **A measured CPU cost in the tail.** Frame-time P99 is +0.4 to +3 ms on frames that rebuild coast caches: a light
   change, a new animation stage, scrolling, or zoom. It shows on every lane, with the same sign in every repetition.
   P50 is unchanged. The cost is explained by composition bursts, whose per-frame P99 is 1.83 ms. Outside zoom, the candidate's P99 frames stay
   under a 16.7 ms frame: at 1080p `coastnight` goes 7.6 → 9.6 ms (SDL2); at 1440p the highest is 16.3 ms (SDL2
   `coastnight`, testing-3 15.0), a narrow margin; `coastzoom` is already at
   about 40 ms in testing-3 and rises 2–3 ms. A per-stage texture cache would likely remove most of it, but was not
   applied: it would change the frozen candidate (`A4-MAINTAINER-REVIEW.md`, point 4).
2. **H1 reaches the coast.** At 2/1, sloped-coast tiles take XR32's edge coverage: 1.6 % of their pixels differ from
   the 16-bit scaler. The open sea and the flat coasts already behave this way in testing-3.
3. **Platform limits.** Linux was tested only under WSLg, which is not a native desktop. There was no MSVC, no macOS,
   no multi-monitor or HiDPI testing. The tile oracles ran on the pak128.german family; pak64 coast tiles were seen
   only by the A6 gate (`A4-PLATFORM-VALIDATION.md`).
4. **Memory.** Peak working set is +0.8 MB on SDL2 and +1.1 MB on SDL3 with an XR32 water pak; there is no
   difference without one. This is not a reason for B, only reported.

Out of scope and unchanged: the other generated ground tiles (climates, snow, transitions) are still 16-bit, and the
beach masks are 5-bit.

## What exactly

| | |
| --- | --- |
| Base | testing-3, published `e71c024c` (lab `df2a1b0`, tree `e0c48441`), trunk r12377 |
| Candidate | lab `coast/candidate` `84b3225506804ff6e14edb5e70bd0924ab78faf5`, tree `64548ac8`; 5 files +289/−4 |
| Frozen | `candidate/XR32-A4-COAST-r12377-testing-3.diff` (sha256 `05792adc…`), `candidate/coast-candidate.bundle`; it applies with `git apply`, with GNU `patch`, and onto the published commit, giving the candidate tree exactly |
| Product binaries | Windows SDL3 `52a0e2cb…`, SDL2 `b8b2c4c9…`; Linux SDL2 `f2cbc892…`, SDL3 `4d8efce6…`; 16-bit unchanged (`evidence/candidate-binaries.sha256`) |

## Status

| | |
| --- | --- |
| Readiness | **READY_FOR_APPROVAL**, as an A4 addition to XR32 r12377 testing, with the limitations above |
| Publication | **NOT_AUTHORISED**. No push, tag, release, forum post, SVN or contact; testing-3 not altered |
| Decisions for the maintainer | (1) keep the defensive `release_above`; (2) drop the measurement counters before going upstream; (3) accept H1 on the coast; (4) do the per-stage texture cache now, or in a later iteration (`A4-MAINTAINER-REVIEW.md`) |
| Non-lab processes terminated | 0 |
