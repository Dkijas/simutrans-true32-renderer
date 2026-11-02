> **Publication note (2026-11-02).** This certification was done on the frozen candidate; it was then published,
> with the user's authorisation, as the pre-release `xr32-prissi-night-image-prototype-01-r12377-testing-3` of this
> repository, from the same files: the four game archives carry exactly the certified executables (Windows SDL2
> `6e016e96…`, SDL3 `69444edc…`, Linux SDL2 `d096df85…`, SDL3 `2f594079…`, and testing-2's `simutrans-16bit`), makeobj
> and the test kit are testing-2's assets byte for byte, and `source/xr32-r12377/` is the candidate's tree `e0c48441`.
> "Publication: NOT_AUTHORISED" below is the state at the end of the certification. Paths: `deliver/…` files of the lab
> are in `reports/xr32-dirty-regions-cert-01/` and `deliver/testing-3/…` is the release itself; `evidence/…`, `logs/…`
> and `out/…` refer to the lab and selected files are in `evidence/xr32-dirty-regions-cert-01/`. Local machine paths are
> replaced by placeholders (`<lab>` and similar).

# XR32 dirty-region upload — certification report (SIMUTRANS-XR32-DIRTY-REGIONS-CERTIFICATION-01)

## Verdict

**B — CERTIFIED WITH LIMITATIONS.**

- **Correctness: PASS** on every lane the oracle can see. On Windows SDL2 / SDL3 and on Linux SDL2 / SDL3 (WSLg X11 and
  Wayland; SDL3 also with SDL's dummy driver), the window showed exactly what the renderer drew in every verified
  frame, with three exceptions, none caused by the candidate:
  - T1, two toolbar elements after a live theme switch: official r12377 16-bit leaves the same pixel positions stale;
  - the window-size switch frame under SDL3 + X11: testing-2 mismatches the same frames;
  - the Linux SDL2 + dummy lane, which the oracle cannot see at all (VOID; testing-2 fails identically there).

  Every result testing-2 certified gave the same values. Sanitizer and warning sets are unchanged.
- **Performance: better where the screen changes in part, which is the normal case**; neutral where the whole screen
  changes every frame. There is one small unexplained cost: whole-screen scrolling on Windows SDL3, +0.08 ms at 1080p
  and +0.41 ms at 1440p, and +0.1 ms on SDL2 at 1440p.
- **Limitations** (why B and not A):
  - Linux was tested only under WSL 2, and **WSLg is not a native desktop**.
  - Native X11 / Wayland, HiDPI, multi-monitor, other GPUs, macOS, Android and ARM were not tested.
  - The X11 switch frame is unverifiable.
  - The scroll cost above is unexplained.

Readiness: **READY_FOR_APPROVAL** as XR32 r12377 testing-3. Publication: **NOT_AUTHORISED** (this mission authorised
none). Nothing was pushed, tagged, released or posted.

## What was certified

| | |
| --- | --- |
| Base | official SVN trunk r12377 (lab commit `7cc6d45` = `svn export -r 12377 --native-eol LF`) |
| Previous | testing-2: published commit `ff4279fc`, source tree `3a194065` (lab `fb5e8a5`) |
| Candidate | lab branch `dr/candidate` `df2a1b0`, tree `e0c48441`, 3 files, +123 / −14 against testing-2 |
| Changes | C `2d9905d` invalidate on light / company colour · B `0de51bc` screen texture unblended · D `0671565` `draw_rect` marks dirty (**found by this mission's oracle**) · A `df2a1b0` dirty-tile upload (simgraph16's walk) — `PATCH-REVIEW.md` |
| Scope | only `simgraph32.cc` and the 32-bit branches of `simsys_s2.cc` / `simsys_s3.cc`; no trunk patch B/C/D/E/G/H, no discarded lock experiment, no lab code (`PATCH-REVIEW.md`) |
| Product binaries | Windows SDL2 `6e016e96…`, SDL3 `69444edc…` (MSYS2 GCC 16.1); Linux SDL2 `d096df85…`, SDL3 `2f594079…` (Debian 13 GCC 14.2); fresh, non-incremental builds |
| Reproducibility | testing-2's Linux 32-bit rebuilt here = testing-2's certified binaries byte for byte; the candidate's Linux 16-bit = testing-2's |

## Evidence

| Gate | Result | File |
| --- | --- | --- |
| Builds Win / Linux × SDL2 / SDL3, fresh | 4 / 4 product + 4 / 4 testing-2 base + lab builds, rc 0 | `logs/builds-*.txt` |
| Warning **sets** vs testing-2 | Windows 21 = 21, Linux 16 = 16; 0 added, 0 removed; none in touched files; control detected | `evidence/warning-sets.txt` |
| Framebuffer oracle | candidate 0 px outside T1 / switch frames / the VOID lane; mutant and noC controls fail | `FRAMEBUFFER-INTEGRITY-REPORT.md`, `evidence/oracle-matrix.txt` |
| Platform matrix (15 cases × 8 lanes = 120 cells) | PASS 86, PASS at framebuffer level (water) 4, PASS by exit code (shutdown in WSLg) 4, PASS outside the X11 switch frames 2, KNOWN-T1 7, VOID 13 (Linux SDL2 + dummy), NOT RUN 4 (water in WSLg), FAIL 0 | `PLATFORM-VALIDATION-MATRIX.csv` |
| Regression battery (A1/A2, A3, A5, A6, N1, N5, I, H, night, MRES, gates A/B) | same checks, same values as testing-2, line for line; control vs testing-1 differs | `XR32-REGRESSION-MATRIX.md`, `evidence/regression-vs-testing2.txt` |
| Automated tests (pak64, 309) | 309 / 309 for rgba32 and 16-bit, Windows and Linux, SDL2 and SDL3; injected failure FAILs | `logs/suites-win.txt`, `logs/linux*-pkg.txt` |
| Clean shutdown, peak memory | Windows 24 / 24 PASS, Δ peak WS vs testing-2 +0.0 … +0.2 MB; Linux 6 / 6 PASS | `evidence/shutdown-mem-t3.txt` |
| ASan + UBSan + LSan (Linux, 32 runs) | 0 ASan errors; UBSan / LSan sets identical to testing-2 in every pair | `evidence/asan-compare.txt` |
| Benchmarks (interleaved, per-run SHA-256, 0 rejected) | `BENCHMARK-COMPARISON.csv` | `evidence/bench-comparison.txt` |
| Patches | steps / one / full routes = candidate tree byte for byte; control fails; generator reproduces testing-2's published diff | `evidence/patches-t3.txt` |

## Findings of this mission

1. **D (new, fixed):** `simgraph32_draw_rect` ignored its dirty flag, so nothing it drew was ever uploaded once the
   upload became partial. Found by the oracle's `edges` scenario (5,605 px stale with the starting patch). Audit:
   it was the only one of 19 functions with this problem.
2. **C is necessary:** the candidate without C leaves 64–121 M px stale per run wherever the light level changes or a
   company colour changes.
3. **B is necessary on SDL3:** testing-2's SDL3 builds blend 33–189 framebuffer pixels per frame with the render
   target (1,022–4,038 mismatching px per run, up to 277,080); 0 with B.
4. **T1 (trunk):** two toolbar elements stay stale after a live theme switch, with identical pixel positions in official
   r12377 16-bit. O1, an 82-px variant first taken for a candidate defect, is the same trunk behaviour (picture dumps).
5. **Harness limits, measured:**
   - SDL2's dummy display is fixed at 1024×768 (VOID lane).
   - SDL3 + X11 switch frames are unverifiable.
   - The lab logger drops one row at `STLAB_START=0`.
   - The csv header's `tex=` is the pre-resize size.

   None of them changes a verdict.

## Benchmarks (summary; full table `BENCHMARK-COMPARISON.csv`)

CPU time per frame (frame period minus the game's own pacing sleep), not frame rate: the game caps at 100 fps, and
vsync was off in every run (`vsync=0` in all 308 benchmark headers). Paired, interleaved runs: 3 at 1080p, 2 at 1440p, 2 on dummy.
A difference counts as measurable only when every pair has the same sign and the median exceeds testing-2's own
run-to-run spread; anything else is reported as "no measurable difference".

| Scenario class | Windows 1080p / 1440p | Linux WSLg X11 1080p / 1440p | Linux dummy 1080p (SDL3) |
| --- | --- | --- | --- |
| real: traffic (`vehicles`) | SDL2 −0.63 / −1.36 ms, SDL3 −1.19 / −1.61 ms | SDL2 −0.99 / −2.01 ms, SDL3 −2.35 / −5.47 ms | −0.72 ms |
| real: paused map (`static`) | SDL2 −0.94 / −1.99, SDL3 −0.29 / −1.69 | SDL2 −0.54 / −2.00, SDL3 −2.43 / −5.16 | −0.06 |
| synthetic, partial (`zoom`, `windows`, `daynight`) | −0.28 … −1.75 in all 12 cells | −0.43 … −4.58 in all 12 cells | −0.10 … −0.92 |
| synthetic, whole screen every frame (`scroll`, `fullredraw`) | no measurable difference, except `scroll` SDL3 +0.08 / +0.41 and SDL2 1440p +0.11 | no measurable difference, except SDL3 1440p `scroll` −0.42 and `fullredraw` −0.52, and 1080p `fullredraw` SDL3 −0.25 | no measurable difference |

Bytes handed to SDL per frame with traffic: 7.9 → 0.15–0.23 MB at 1080p, 14.1 → 0.28–0.45 MB at 1440p.

**Noise statement.**

- Measured on one machine (Windows 11, one 3440×1440 display; WSL 2 on the same host).
- Each platform's phases ran one at a time, after every correctness job of the mission had finished. The machine
  was not idle in the strict sense: the desktop session had Steam, Discord, VS Code, Edge WebView and this agent
  open. `evidence/bench-load-*.txt` lists them with their cumulative CPU seconds, which is not the load during the
  runs. Interleaving, with the configuration order rotated per repetition, spreads such load over both sides.
- One exception: a ~5 s, one-core analysis of the Windows results ran during the first Linux phase
  (`evidence/analysis-during-LBX.txt`). It is interleaved across both sides and smaller than any reported difference.
- The scroll cost is not explained: the walk itself measures 0.02 ms (flush minus upload), and the rest is inside
  `SDL_UpdateTexture` of the same full-screen rectangle.
- Whole-screen cases on SDL3 / WSLg gain, probably from B (no blending) — an inference, not measured separately.

## Not covered / limitations

- **Native Linux desktops** (X11 / Wayland), HiDPI above scale 1, more than one display, GPU drivers other than D3D9 /
  D3D11 on this machine and WSLg's: not tested. **WSLg is not a native desktop.**
- macOS, Android, ARM, GDI backend, the plain Makefile build: not tested (XR32 builds only with CMake for SDL2 / SDL3).
- Oracle blind spots:
  - the frames of the loading screen before the world clock moves (the first frame of the world is a full upload);
  - the SDL3 + X11 window-size switch frames;
  - the Linux SDL2 + dummy lane.
- Water / coasts: framebuffer-level gates only (the upload path does not depend on content); A4 stays open.
- A4, A7 / A8, N2 / N3 / N4: unchanged, **not** declared fixed.

## Processes

Non-lab processes terminated: 0. Only this mission's own processes were started (every game run by this lane's
scripts, each with a timeout); none was killed by name.
