# A4C — certification of the cache candidate

| | |
| --- | --- |
| Candidate | `a4/cache` `0afdfbd31ee38c8a581a7f6251ade9bc80f60439` (tree `96f3d410`) = A4 `84b3225` + cleanup `9b03e8b` + cache |
| References | testing-3 published (`e71c024c` / lab `df2a1b0`; Windows `6e016e96` SDL2, `69444edc` SDL3); A4 frozen `84b3225` (Windows `b8b2c4c9` SDL2, `52a0e2cb` SDL3) |
| Product binaries | Windows SDL3 `43420d63…`, SDL2 `42e9089c…`; Linux SDL2 `c0cec57e…`, SDL3 `7a556636…` (`evidence/candidate-binaries*.sha256`) |
| Builds | fresh and non-incremental, `SOURCE_DATE_EPOCH=1793534400`. Two independent Windows SDL3 builds gave the same hash `43420d63` (a trial build and the certification build) |

Every PASS below has a control that was shown to fail.

## 1. Acceptance: the graphic result is exactly A4's

`evidence/gates-cache.txt`: every coast water tile drawn alone by the game's own functions, read back from the
framebuffer, on both black and white, compared with the same draws of the A4 frozen binary. `tools/gate_extras.py
identity` compares the rectangle and both pixel buffers, byte for byte.

| Pak / run | Zooms × light levels × stages | Draws | Byte-identical to A4 |
| --- | --- | --- | --- |
| mutant, XR32 on | 1/1 × 0 × {0, 13, 31} | 195 | **195** |
| mutant, XR32 off | 1/1 × 0 × 3 | 195 | **195** |
| makie (real art, 32 stages) | 5 zooms × {0, 2, 4} × 3 | 3,735 | **3,735** |
| DAY+NIGHT (night planes, 4 stages) | {1/1, 3/8} × 0–4 × 4 | 2,660 | **2,660** |
| mutant, zoom walk | 5 zooms × {0, 4} × 3 | 1,950 | **1,950** |
| plain pak128.german (no XR32 water) | 4 zooms × {0, 4} × 3 | 1,560 | **1,560** |
| mutant, sea images + tiles, XR32 on / off | {1/1, 2/1} × {0, 4} | 476 + 476 | **476 + 476** |
| **Total** | | **11,247** | **11,247** |

**Controls.**
- Neighbouring draws compared with each other differ in every run, so the comparison can fail.
- `mut/cache-stale`, whose cache key leaves out the stage, differs from A4 in **2,862 of 3,735** draws on makie and in
  **1,270 of 2,660** on DAY+NIGHT. The independent model catches it in 192 of 195 and 128 of 260 tiles.

The same exe also passes, unchanged from A4, against the **independent composition model**:
- composition: 0 mismatches for mutant, makie and DAY+NIGHT; model16 0 with XR32 off;
- **night relative** to the sea image: 0 px at every level;
- **zoom geometry** against testing-3: identical, with H1 at 2/1;
- **plain pak**: 1,560 draws byte-identical to testing-3;
- **makie precision**: identical to A4 (71.9 % off the RGB555 grid, 1,063 colours).

## 2. H1 at 2/1: geometric continuity (`evidence/h1-holes*.txt`, `captures/h1-hole-*.png`)

The brief accepts H1 and asks to verify that it introduces no geometric discontinuity. Method:
- world captures of the fixed coast map, mutant pak, forced day, 1920×1080, at 1/1 and 2/1;
- count the "holes": groups of 1–8 non-water pixels enclosed by water;
- classify each hole the candidate has and testing-3 has not, at the same pixels.

The control knocks out 5,316 water pixels: 5,146 new holes are found.

| Zoom | testing-3 holes | A4 holes | cache holes | New vs testing-3 (A4 = cache) |
| --- | --- | --- | --- | --- |
| 1/1 | 41 | 43 | 43 | 2 (5 px): beach-mask blends or the same pixel as testing-3, **no gap** |
| 2/1 | 25 | 55 | 55 | 30 (30 px): 23 are the same grey object testing-3 draws (now surrounded by XR32 water instead of 16-bit water); **7 are real 1-px gaps** |

The 7 real gaps are sand pixels of the land tile under the water, showing through where testing-3 had 16-bit water.
They are isolated single pixels near the shore of sloped coasts (`captures/h1-hole-z2-0.png`). testing-3 has **no**
sand gap at 2/1, flat coasts and open sea included.

**Finding: H1 does introduce small discontinuities at 2/1.** About 7 one-pixel gaps were seen on a 1080p view with 93
sloped coasts. They come from XR32's 2/1 coverage of the coast tile meeting the 16-bit-scaled beach mask.
- They are **A4's**, inherited unchanged: the cache candidate's draws are byte-identical to A4's.
- They were not caught in the A4 certification, which compared coverage but did not look for enclosed gaps.
- 1/1 and every other zoom are free of them.

## 3. Regression and platform gates

| Gate | Result | Evidence |
| --- | --- | --- |
| Warning sets (Win SDL2/SDL3 21/21, Linux 16/16) | equal to testing-3, 0 new; the control is detected | `warning-sets.txt` |
| 16-bit unchanged | Windows `8feaaf49` / `20cbf7da` = A4's = base 16-bit; Linux `3e7cd80e` / `9822aee1` = testing-3 byte for byte | `candidate-binaries*.sha256`, `linux-setup.txt` |
| ASan / UBSan / LSan, Linux SDL2/SDL3, 40 runs (product world on cat and coast, 6 coast scenarios + DAY+NIGHT and makie night with the oracle on) | ASan errors 0; new UBSan locations 0; new LSan origins 0. A warm-up run first creates `settings.xml`, so testing-3 and the candidate start from the same state, and the A4 order artefact is gone (17 = 17 on `worldcat`) | `asan-compare-cache.txt` |
| Frozen candidate: COMPLETE, CACHE-ONLY and CLEANUP diffs | 5 application routes reproduce their target trees exactly, including the published commit's `source/xr32-r12377`; altered-context control rc 1 | `freeze-cache.txt` |
| Presentation oracle (window = renderer, every frame), 6 coast scenarios + first frames + DAY+NIGHT, Windows SDL2/SDL3 (direct3d / d3d11) and Linux SDL3 dummy and SDL2/SDL3 x11 + wayland, the cache lab exe `w-ca` / `l-ca` | **0 px** on all candidate runs. The exceptions are SDL3 + X11 `coastfull` / `coastresize`, which differ only on the window-size switch frames (3 / 8) that testing-3 mismatches identically in the same run. The `droprect` mutant fails in every scenario except `coastscroll` (whole frame dirty). 52 Windows + 106 Linux runs accepted, 0 rejected | `oracle-matrix-all.txt`, `switch-frames.txt` |
| Battery Linux vs testing-3's run of the same battery (identity, A5, N1, A6, A2, A1 box; SDL2/SDL3) | the same lines as A4's run: only A6 on `pak64.german-mres1` differs, 10,895 → 10,970 images, the 75 derived pak64 coast tiles. **All 10,970 A6 pictures are byte-identical to A4's**, on 2 routes × 2 zooms × 2 backends, and the 10,895 common ones are identical to testing-3's (control: other zoom, 10,488 differ) | `battery-vs-testing3.txt`, `a6-vs-a4.txt` |
| Battery Windows vs testing-3 (gdb gates, identity, A5, N1, A6, fit, gates A/B) | the same lines as A4's run: `seadraw`/`mutantdraw` slots 54 → 2,134 (the derived tiles), and `catworld` `rezoomed` counts within their known run-to-run drift. gates-sdl2 / gates-sdl3: 0 differing lines. The automated-test suites did not run the first time, a lane set-up error of mine (`work/final-src/tests` missing). **Re-run after the performance phase:** the control fails at the injected test (1 of 310), as designed, and candidate SDL2/SDL3 32-bit and 16-bit plus official r12377 16-bit each **PASS 309 / 309**. These are the same lines as testing-3's run | `battery-vs-testing3.txt`, `logs/suites-win.txt` |
| K (clean shutdown) / M (peak memory), testing-3 / A4 / cache, 2 workloads × SDL2/SDL3 × XR32 on/off, n = 3 | 72 of 72 PASS. Cache − A4 with XR32 water: −0.5 / −0.1 MB, so no regression; otherwise noise | `shutdown-mem-cache.txt`, `A4C-MEMORY.md` |
| Re-init without `release_above`: gdb calls `init_ground_textures()` a second time on the running world, lab exe `w-ca-s3`, oracle on | the same ids; 2,080 nodes again (re-registration succeeds because `attach` replaces the node); 0 frames with oracle mismatch. Removing the function changes neither correctness nor ownership (exit frees everything through `release_all`, and LSan is clean) | `reinit-probe-cache.txt` |
