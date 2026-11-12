# Regression matrix — testing-8 candidate `3200e8e` against testing-7

**Rule:**
- **A4** may change the coasts it fixes.
- **TCC-R** may change the translucent company-colour pixels it corrects when zoomed out.
- **A12-P1** may change nothing.
- **The 16-bit renderer** may change nothing.

Each row has its own oracle and, where it matters, its own negative control.

**How the rows are labelled:**
- "**this gate**" = run here, on the final executables.
- "**carried**" = run in the previous lab (`<testing8-graphics-zoom-1-lab>`) or the A12-P1 lane, on executables
  **byte-identical** to these. F3-s3 equals the previous lab's t8c-s3, and B0p/B1p/B2p equal the A12-P1 lane's prof twins
  (`SOURCE-VERIFICATION.md`). The same bytes on the same machine give the same result, so the result applies.

| # | Area | Check | Result | Control | Source |
| --- | --- | --- | --- | --- | --- |
| R1 | Automated suite | 309 tests, F3-s3 and F3-s2 (tests tree `c911ea7d` = testing-7's) | **309/309 both** | one injected failing test → **FAIL [1/310]** | this gate, `logs/suites.txt` |
| R2 | TCC-R, fixture | 5 zooms × 3 players (0, 3, 5), day + night, round trips; **real windows** SDL3 + SDL2 | **0 failures** (172 checks) | `--swap` → **18 failures** (day, both backends) | this gate, `TCC-R-REAL-WINDOW-VALIDATION.md` |
| R3 | TCC-R, real paks | every XR32 image alone, PAK128.German (50,823) and pak192.comic, 4–5 zooms | changes only at 9/8, 3/4, 1/2, **0 outside the candidate sizes**, 0 at 1/1 | empty candidate set → fails (previous lab) | this gate, `logs/tcc/drawall-*.txt` |
| R4 | Real window vs headless | the TCC raw captures and drawall tables, window vs dummy | **0 differing** of 41 files | — | this gate |
| R5 | Integration oracle | P1 → t8c changes exactly the rows T0 → T1 changes (climate, snow, keys, coast water, all XR32 images, day and night) | **41/41 PASS** | expectation "nothing changes" fails in the 16 captures that do change | carried (t8c-s3 = F3-s3), `TESTING8-VISUAL-REGRESSION.md` |
| R6 | A4, coast water | identity oracle on the coast water of N, NK, LA, TW, SK; A4's independent coast model; all 32 stages × 65 slopes × 5 zooms and the night levels | 0 differing; model PASS | B1 vs B0: 18,240 tile-steps differ; P0 under A4's rule FAILS (1,216 tiles) | carried (A12-P1 lane `REGRESSION-REPORT.md`; B1p = prof-b1, B2p = prof-p1) |
| R7 | A12-P1, identity | 520 tiles × 15 steps × 6 paks, SDL3 + SDL2: cache on = cache off; plus A11/H1 2/1 steps, every other XR32 image (~50,800 per pak, day and level 4), DAY/NIGHT | **158 PASS / 0 FAIL**, 0 differing | mutants `anyhit`, `climate`, `nightday`, `evict`: caught | carried (A12-P1 lane, `evidence/ident-cmp.txt`) |
| R8 | Special colours (testing-7's fix) | generated tiles with exact special greys (NK, TK, LT paks) stay 32-bit | included in R7's identity oracle on NK/TK/LT: 0 differing | — | carried |
| R9 | Day / night | night captures differ from day (N1); confinement (N2); light changes in real windows | PASS | `daynight` repaints the whole screen: no droprect control there (known) | this gate |
| R10 | Zoom 1/1, 2/1, 9/8, 3/4, 1/2 and back | fixture and drawall at each level, round trips identical | PASS | — | this gate |
| R11 | Dirty regions | screen oracle, real windows, B0 against B3, SDL3 + SDL2 | SDL3 0 of 44 runs both; SDL2 B0 0 of 54, B3 1 of 54 (3 px, not recurring in 20 more runs) | droprect mutant caught wherever a rectangle can be dropped; 3 full-repaint scenarios uncatchable | this gate, `DIRTY-REGIONS.md` |
| R12 | 16-bit renderer | candidate's 16-bit builds against testing-7's published 16-bit | **byte-identical**, SDL3 and SDL2 | — | this gate, `logs/builds.txt` |
| R13 | XR32 format, makeobj | files touched; makeobj's compiled sources | **0** readers, writers, `descriptor/`, `dataobj/`, `makeobj/`; makeobj compiles no `display/` file. In the A12-P1 lane, a makeobj rebuilt with A4 + A12-P1 was byte-identical to the A4 lane's (`8a150381…`) | the A10 marker check finds A10 in its own record | this gate, `logs/identity.txt`; carried |
| R14 | Memory | 44–58 min side-by-side sessions, cache bound | **0 NEW-GROWTH**; cache max 13.4 MiB < 16 MiB; the one short first-run flag did not recur in 2 × 57 min | synthetic +20 MB/h leak → flagged on **every** B3 series | this gate, `MEMORY-STRESS.md` |
| R15 | Threads | 8-thread stress of the cache | 12/12 complete, identical checksums | the lockless cache crashes 12/12 | this gate, `THREAD-SAFETY.md` |
| R16 | Warnings | warning sets of F3 against testing-7 | **equal**, SDL3 and SDL2 | — | this gate, `logs/build-*.warnings` |
