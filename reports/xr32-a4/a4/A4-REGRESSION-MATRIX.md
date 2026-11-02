# A4 — regression matrix

| | |
| --- | --- |
| Base | published testing-3 (`e71c024c`; lab `df2a1b0`); Windows exes `6e016e96` (SDL2) and `69444edc` (SDL3), extracted from the release zips |
| Candidate | `coast/candidate` `84b3225`; product exes from fresh, non-incremental builds with a fixed `SOURCE_DATE_EPOCH` |
| Lab builds | `lab/t3coast` and `lab/coast`: the same lab code (`-DSTLAB_PERF`, coast scenarios) on both sides |

## Gates

Every PASS below has a control that was shown to fail.

| Gate | What | Lanes | Candidate | Control | Evidence |
| --- | --- | --- | --- | --- | --- |
| C1 composition | coast tiles == independent model32, 1/1 | Win SDL3, 3 paks, 650 tiles | 0 mismatches | no-copies mutant: 180/195; testing-3: 192/195 | `final-gates.txt` |
| C2 XR32 off | coast tiles == model16, 0 key px | Win SDL3 | 0 mismatches | (C1 controls) | `final-gates.txt` |
| C3 night | sea-sourced px == the sea image, levels 0–4 | Win SDL3, 3 paks | 0 px | another stage, another level | `final-gates.txt` |
| C4 zoom geometry | rect + coverage == testing-3 at 11/8, 1/1, 3/8, 1/8 | Win SDL3 | identical; at 2/1 H1 (52,026 px, inherited behaviour) | shifted ids | `final-gates.txt` |
| C5 traditional pak | `pak128.german` draws byte-identical to testing-3 | Win SDL3 | 1,560 / 1,560 | neighbour draws differ | `final-gates.txt` |
| O1 dirty regions | window == renderer, every frame, 6 coast scenarios + first frames + DAY+NIGHT | Win SDL2/SDL3 (direct3d / d3d11); Linux SDL3 dummy, SDL2/SDL3 x11 + wayland | 0 px on 54 of 56 candidate runs (20 Windows, 36 Linux), every frame verified (240 per run; 40 in the first-frames phase; 12 in the two short Windows phases). The other 2 runs are SDL3 + X11 `coastfull` / `coastresize`, which differ only on the window-size switch frames, see O2. testing-3 shows the same pattern on its 55 runs. | `droprect` mutant fails in every scenario except `coastscroll`, where the whole frame is dirty and the mutant has nothing to drop | `oracle-matrix-all.txt` |
| O2 switch frames | SDL3 + X11 `coastfull` / `coastresize` | Linux | 0 px outside 3 and 8 switch frames; the same frames mismatch in testing-3 | mutant mismatches outside them | `switch-frames.txt` |
| B1 battery | identity, A5, N1, A6, A1/A2, gates A/B, automated tests | Win SDL2/SDL3 | same lines as testing-3's run except: `seadraw`/`mutantdraw` slots 54 → 2,134 (the 2,080 derived tiles, expected); `catworld` `rezoomed` counts, e.g. 1/4: 210 → 203, 1/8: 558 → 543. These counts depend on what the view has drawn when the gate samples. testing-3's own certification saw the same kind of drift against testing-2 (209 / 210, 545 / 558), and `pak128.german-cat` has no XR32 water, so A4 registers nothing there. The `legacy-hidden` and `drawn where legacy hides` columns are equal (0, 1, 6 / 0) | altered-line copy | `battery-vs-testing3.txt` |
| B2 gates | gdb gate battery | Win SDL2, SDL3 | 0 differing lines vs testing-3 | (B1) | `battery-vs-testing3.txt` |
| B3 battery Linux | identity, A5, N1, A6, A2, A1 box | Linux SDL2/SDL3 | Same lines as testing-3's run except A6 on `pak64.german-mres1`: 10,895 → 10,970 XR32 images. The 75 extra are the generated coast tiles of that fixture (ids 11058–11132, contiguous at the end of the image table), which has an XR32 sea. The 10,895 common pictures are byte-identical to testing-3's, and A6 holds for all 10,970: route A vs route B gives 0 differing pictures at 3/2 and 5/4 | altered-line copy | `battery-vs-testing3.txt`, `a6-extra-ids.txt` |
| R1 re-init | a second `init_ground_textures` on a running world | Win SDL3 | same ids, 2,080 nodes again, oracle clean | — (no failing path exists, see review) | `reinit-probe.txt` |
| K clean shutdown | the game runs about 2 months under `-until` and exits by itself | Win SDL2/SDL3, XR32 on/off; `cat` (oepnv1950) and `coast` (mutant pak + coastworld); n=3 interleaved; candidate vs PUBLISHED testing-3 | 48 of 48 PASS: exit 0, no FATAL / `Error:` line, the same skipped-node counts as testing-3 (154 / 192 with XR32 off) | injected FATAL line counted; foreign hash flagged | `shutdown-mem-a4.txt` |
| M peak memory | peak working set, paired per repetition | same | `cat`, or XR32 off: −0.1 to +0.0 MB median (noise). `coast` with XR32 on: **+0.8 MB (SDL2), +1.1 MB (SDL3)** median, candidate higher in 3 of 3 (about 0.2 % of 497–522 MB). The recipes and shells are estimated at about 0.25 MB; the rest is the derived tiles' caches and the composition buffers | (provenance control) | `shutdown-mem-a4.txt` |
| S sanitizers | ASan / UBSan / LSan, new report locations | Linux SDL2/SDL3, product (`worldcat`, `worldcoast`) + lab (6 coast scenarios + DAY+NIGHT night, oracle on); 36 runs | ASan errors 0; new UBSan locations 0. One "new" LSan origin, `rdwr_str ← env_t::rdwr` (3 bytes), came from the run order: the testing-3 run it was compared with was the first, with no `settings.xml`, and a rerun of testing-3 shows the same leak. New LSan origins: 0 | synthetic UBSan line is detected as new | `asan-compare-a4.txt` (+ addendum), `asan-rerun-t3-worldcat.txt` |
| W warnings | warning sets | Win SDL2/SDL3, Linux SDL2/SDL3 | 21/21, 16/16, 0 new | injected warning | `warning-sets.txt` |

## Binary identity of what must not change

| Item | Result | Evidence |
| --- | --- | --- |
| Linux 16-bit candidate vs testing-3 16-bit | byte-identical (`3e7cd80e`, `9822aee1`) | `lin_hashes` / `candidate-binaries.sha256` |
| Linux testing-3 rebuilt here vs testing-3 published | byte-identical (`d096df85`, `2f594079`) | same |
| Windows 16-bit candidate vs base 16-bit | byte-identical | `repro-windows.txt` |
| Windows 16-bit candidate vs testing-2/3 published 16-bit | the same program (only the PE TimeDateStamp / CheckSum bytes differ) | `repro-windows.txt` |
| Windows testing-3 rebuilt here vs the testing-3 release zips | the same program | `repro-windows-published.txt` |

The `pe_cmp` control, one flipped byte, is detected.

## Regressions found

**No correctness regression.**

**One measured performance cost** (`A4-BENCHMARKS.csv`, `evidence/perframe-compose.txt`): on frames that rebuild
coast caches, the **frame-time P99 rises by about 0.4–3 ms**. Those are frames where the light changes, the water
animation reaches a (slope, stage) not yet seen, new coast scrolls into view, or the zoom changes.
- The sign is the same in every repetition, on Windows SDL2/SDL3 at 1080p and 1440p and on Linux X11.
- P50 shows no measurable difference.
- Composition arrives in bursts of about 11 tiles at about 1.4 ms, in about 15 % of the frames of `coastnight`. The
  P99 of the per-frame composing time is 1.83 ms, against the measured P99 cost of +1.9 / +2.1 ms there.
- The control without XR32 water shows no difference, except one n=3 row of noise.

Behaviour that changes, and is intended:
1. Sloped-coast water uses the XR32 sea. This is the fix.
2. At 2/1, sloped-coast tiles carry XR32's edge coverage (H1), as the open sea already does.
3. Tiles get XR32 slots: 54 → 2,134 with an XR32 water pak. This is visible in the slot counts the battery prints.
