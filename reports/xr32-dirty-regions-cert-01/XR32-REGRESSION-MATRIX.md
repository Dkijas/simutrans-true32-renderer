# XR32 regression matrix — testing-3 candidate vs testing-2

**Question.** Does the candidate keep every result that testing-2 certified (A1/A2, A3, A5, A6, N1, N5, and the rest of
the battery)?

**Answer.** Yes. The imported battery (testing-2's own gates, `tools/battery/`, rewritten to this lane, 0 references
left to the testing-2 lane) was run on the candidate's **product** executables. Every analyser that writes a per-label
evidence file gives **the same checks with the same values as testing-2's published evidence**, line for line
(`evidence/regression-vs-testing2.txt`; control: the candidate's file against testing-1's differs by 18 lines).

## Executables

| | Windows SDL2 | Windows SDL3 | Linux SDL2 | Linux SDL3 |
| --- | --- | --- | --- | --- |
| candidate `simutrans-rgba32` | `6e016e96…` (build/p-sdl2) | `69444edc…` (build/p-sdl3) | `d096df85…` (p-sdl2-32) | `2f594079…` (p-sdl3-32) |
| `simutrans-16bit` (= testing-2's) | `6a3a9742…` | `a3b66376…` | `3e7cd80e…` | `9822aee1…` |
| base / control (published testing-1) | `f5db98c5…` | `6682cec8…` | `5ca3357c…` | (the SDL2 one) |

The candidate's Linux 16-bit build is byte-identical to testing-2's; testing-2's Linux 32-bit rebuilt here with the same
recipe is byte-identical to testing-2's certified binaries (`fc04d444…`, `7d4b36d2…`), so the recipe is the one that
made testing-2.

## Matrix

| # | What | Gate (data) | Windows SDL2 / SDL3 | Linux SDL2 / SDL3 | vs testing-2 | Control |
| --- | --- | --- | --- | --- | --- | --- |
| A1 | 32-bit image = 16-bit size and place at every zoom | Windows: `barzoom`, fit parity; Linux: `a1box` (every 10th image of Frank's pak64.german) | toolbar band 0 px at world zoom 2/1, 3/8 and back at 1/1 (XR32 on); fit parity 278 / 278 | lit box identical 1164 of 1173 at 3/2 (as testing-2), … every step | identical values | testing-1 base runs; `barzoom` XR32 off: 6,400 px |
| A2 | catenary wire as 16-bit | `a2` (122 catenary images of pak128.german) | — | lit box = legacy 122 / 122 at every step; 105,607 vs 105,601 lit px at 9/8 | identical values | testing-1 base |
| A3 | shared images keep their own 32-bit picture | `identity` (10 objects × 8 directions, merged pak) | 13 / 13 checks ok, 64 distinct ids | 13 / 13 ok, 64 ids | identical values | testing-1: 6 checks FAIL |
| A5 | interface images keep day colours at night | `a5` (uiday, uinight) | 0 of 144 (8 images × 2 players × 9 levels) differ from day; toolbar night / day 0.973 | 0 of 144; 0.973 | identical values | testing-1: 112 of 144 differ, 0.341 |
| A6 | 3/2 independent of the step before | `a6` (122 + 8 + 10,895 images) | — | route A vs B: 0 differ at 3/2 and 5/4 | identical values | 3/2 vs 5/4 differs (117, 8, 10,412) |
| N1 | base-size UI draw uses 32-bit art while zoomed | `n1` | 8 / 8 own 32-bit picture | 8 / 8 | identical values | testing-1: 8 / 8 16-bit picture |
| N5 | non-zoomable images placed as 16-bit | fit parity with the I1 cut | 278 / 278 equal | 281 / 281 equal | identical values | — |
| I | thin images hidden where 16-bit hides them | `thin` (catenary, thin poles) | drawn where legacy hides 0; back to 1/1 0 differ | same | identical verdicts | coverage 21 / 3 legacy-hidden |
| H | sea / water images | `seadraw`, `mutantdraw`, `seaworld` | makie's water 0 px of the key colour; mutant 221,184 px | mutant 225,280 | identical verdicts | the mutant pak |
| — | DAY / NIGHT cards | `night_pk` (16 image × player series × 9 levels, 2 card sets) | geometry 0, return-to-day 0, dusk never brighter; night used 16 / 16 | same | identical verdicts | XR32 off: night-used finds nothing (FAILS as it must) |
| — | MRES 2× toolbar art at 200 % | `mres` | 6,400 / 6,400 px = authored 2× art | same | identical | 16-bit path, tolerance 8 |
| — | gates A / B (XR32 switch, `Skipping unknown XR32-node`) | `logcheck` | 154 lines without `SIMUTRANS_XR32`, 0 with; 16-bit ignores the 32-bit data | same | identical | — |
| — | automated tests (pak64, 309) | `suites` | 309 / 309 for rgba32 and 16-bit, both backends | 309 / 309 | as testing-2 | one injected failing test: FAIL |
| K | clean shutdown (1950 save → 1951.3, own exit) | `shutdown_mem_t3.ps1` / `linux_all` | 24 / 24 PASS | 6 / 6 PASS | — | — |
| M | peak memory | same, n = 3 | Δ vs testing-2: median +0.0 … +0.2 MB (no measurable difference) | rgba32 518–524 MB peak RSS (n = 1 per row, no testing-2 pair: reported, not compared) | — | — |

`analyze_gates.py` reports "failures 26" (Windows) and "failures 152" (Linux): exactly the figures of the consolidation
lane's certified run on testing-2's binaries. They come from its naive fit-parity count, which does not know that the
I1 twins shift image ids; `parity_cut_check.py`, the authoritative check, finds 278 / 278 and 281 / 281 equal.

One difference from the consolidation lane's Windows run: in `catworld` the coverage counts (images rezoomed at 1/4,
1/8 and back: 209 / 210, 545 / 558 / 560, 46 / 49) differ, while the checked value (drawn where legacy hides) is 0 in
both. Measured as run-to-run variation of a running world: `catworld` repeated twice for testing-2's published
executable and twice for the candidate (Windows SDL2) gives, for **testing-2 itself**, 49 / 559 / 46 and 48 / 545 / 46,
and for the candidate 48 / 545 / 49 twice; drawn where legacy hides: 0 in all four (`evidence/catworld-variation.txt`).

## Sanitizers (Linux, ASan + UBSan + LSan)

Product builds (testing-2 vs candidate, SDL2 and SDL3): the 1950 save to 1950.5. Lab builds (identical lab code): the
oracle scenarios `resize zoomx edges theme fullscreen windows scroll`. 32 runs: **0 AddressSanitizer errors**; the
UBSan report set (keyed by the source text of the reported line, since the candidate moves lines in `simgraph32.cc`) and
the LeakSanitizer origins are **the same for testing-2 and the candidate in every pair**. The UBSan reports are the
trunk's misaligned `freelist_tpl.h` accesses (known, r12367) and left shifts of negative values in `blit_clip_core.h`
and in simgraph32's line drawing, present in testing-2 unchanged. One extra leak in the first `world` pair came from
`settings.xml` (the first run of the sequence started without one); rerun with the same settings for both (`world2`):
identical. Control: a synthetic report in a copy of a candidate log is flagged as new (`evidence/asan-compare.txt`).

## Compiler warnings

Warning **sets** (file, message, flag; without line numbers), fresh product builds of testing-2 and the candidate with
the same recipe: Windows 21 / 21 unique, Linux 16 / 16, **0 added, 0 removed**, and no warning in the three files the
candidate touches. Control: a synthetic warning is detected (`evidence/warning-sets.txt`).
