# testing-8 final release gate: verdict

**Mission:** SIMUTRANS-XR32-TESTING8-FINAL-RELEASE-GATE-01 · **Date:** 2026-11-12

## Verdict: **B — READY_WITH_LIMITATIONS → READY_FOR_PUBLICATION_APPROVAL**

**Publication status: NOT_AUTHORISED.** This verdict says the candidate is technically fit to be published as a
**Windows-only experimental testing release**. It is not permission to publish.
- **Not done:** nothing has been pushed, tagged, released, committed to SVN or announced.
- **Required first:** an explicit, separate authorisation for each of those steps.

## Exact identity

| | |
| --- | --- |
| base | testing-7 `3f1b511be97e9fb122780c3da42c7fd07f29e2fc`, tree `562448a2` = the published testing-7 source (`ae28a613`, `source/xr32-r12377/`), on SVN r12377 `7cc6d452`, not rebased |
| candidate | `3200e8e8bbc84880f74a32a2c5e3fb5d6c2d73de`, tree **`70e54fc4a865d0ba850f2083df0c33d06a597aaa`** = testing-7 + A4 `b03d729` + A12-P1 `a644622b` + TCC-R `3200e8e`; exactly 3 commits, 2 files |
| excluded | A10, LEGACY16-SC (0 markers; the A10 marker check finds A10 in its own record) |
| XR32 SDL3 exe | `6f32326414e6a06c4130503a4e8dc838dd314015c374837e59f5b101b5efe7ef`, built twice, identical |
| XR32 SDL2 exe | `6f3dbd0451094db4ea5acbf12a7a1cc2…` |
| 16-bit exes | byte-identical to testing-7's (`8feaaf49…` SDL3, `20cbf7da…` SDL2) |
| SDL3 zip | `961deb01492f75bb29778b622773a3659dd68e430cf90b18017e6b56faedcbf1` |
| SDL2 zip | `bc11f538cfd2a8e91f6b481206de39581a492d202298a82046e6eef9093938bf` |
| source tar | `5c5743891f42f8a50306025c5bec2e1b65c81589f5c66d8f64fac7eac5e5f063` |

All names still carry `candidate`. The renaming step changes the hashes (`ASSET-MANIFEST.md`).

## Gates

Every gate whose PASS matters was first shown able to fail.

| Gate | Candidate | Control | Document |
| --- | --- | --- | --- |
| Identity, reproducibility | tree `70e54fc4` from patches and from cherry-pick; SDL3 exe identical twice | parent tree differs | `INTEGRATED-COMMIT-IDENTITY.md`, `SOURCE-VERIFICATION.md` |
| Build, warnings | 15 fresh builds, all rc 0 (5 release, 8 twins, 2 stress); warning sets of the release builds = testing-7's | — | `WINDOWS-CERTIFICATION.md`, `evidence/logs/builds.txt` |
| Automated suite | **309/309**, SDL3 and SDL2 | FAIL [1/310] | `REGRESSION-MATRIX.md` R1 |
| TCC-R, real windows | **0 failures** in 172 checks: SDL3 day 68, SDL3 night 36, SDL2 day 68 | `--swap` → 18 failures per backend (day) | `TCC-R-REAL-WINDOW-VALIDATION.md` |
| TCC-R on real paks | changes only at 9/8, 3/4, 1/2; 0 outside the candidate images | empty set fails | the same |
| Real window = headless | 41/41 files identical | — | the same |
| Dirty regions | SDL3 B0 0/44, B3 0/44; SDL2 B0 0/54, B3 1/54 (3 px, not recurring in 20 runs) | droprect caught in 8 of 11 scenarios; the 3 others repaint the whole screen | `DIRTY-REGIONS.md` |
| SDL2 window events | not distinguishable from testing-7 (0/36 against 1/36) | — | `SDL2-WINDOW-EVENTS.md` |
| 2/1 pixels | the same 8 px as testing-7, both backends, **0 new** | NK keyed slopes: testing-7 ≠ testing-8 at every step | `ZOOM-2-1-PIXELS.md` |
| Memory | 0 NEW-GROWTH over 44–58 min; cache max 14,018,560 bytes < 16 MiB | +20 MB/h synthetic leak flagged on every B3 series | `MEMORY-STRESS.md` |
| Threads | 12/12 stress runs complete, identical checksums | lockless cache crashes 12/12 | `THREAD-SAFETY.md` |
| Performance | light change event P50 **−12.8 %**, separated 5/5; zoom tails better; TCC-R step: nmd | A/A: nmd everywhere | `BENCHMARK-INTEGRATED.md` |
| Packages | CRC OK, 952 entries each, PE x86-64, exes = fresh builds | — | `ASSET-MANIFEST.md` |
| Private data, credentials | 0 private paths in zips and patches; 0 credentials | synthetic text hits 7/7 | the same |
| Defender | 0 detections, real-time protection on, nothing excluded | **not demonstrated** (EICAR left to the user) | `DEFENDER-SCAN.md` |
| Format, makeobj | 0 reader, writer or makeobj files; **makeobj unchanged** | — | `REGRESSION-MATRIX.md` R13 |

## Why B and not A

These limitations are real and must stay visible in the release notes:

1. **Platform:** Windows x64 only. Linux was not built, and testing-7's Linux packages are not reused (`PLATFORM-SCOPE.md`).
2. **One unfavourable performance result:** on the coast scene with light changes, ordinary frames take **+0.090 ms (+2.6 %) more
   work in median, separated 5/5.** Their tails are not slower. It is reported in the release notes.
3. **Pre-existing behaviours, unchanged:**
   - the 8 px at 2/1, on both backends;
   - the rare SDL2 window-event mismatch;
   - slow memory growth on zoom scenarios in both versions;
   - the crash on a semi-transparent `LightTexture` pixel, in official r12377 too.
4. **Controls that could not be shown:**
   - three full-repaint scenarios, so their dirty-region zeros are not backed by a failing control;
   - Defender, with no EICAR test;
   - night colours under glass, checked for confinement only.
5. **Not measured:**
   - SDL2 memory and SDL2 timing, separately;
   - multi-monitor;
   - any sanitizer or race detector.
6. **Text to fix at the renaming step:** each zip's `README-TESTERS.txt` says "15-19 %"; this gate measured 12.8 %. The zips must
   be rebuilt, rehashed and rechecked after that edit.

**No new defect was found**, so no incident was opened. Every deviation seen is either inside what the patches are meant to change,
or present in testing-7 as well.

## Before publishing (each step needs its own authorisation)

1. **Rename** the files without `-candidate`.
2. **Edit** `README-TESTERS.txt` in both zips (item 6).
3. **Rebuild** the zips, recompute every SHA-256 and re-run the package check and Defender on the renamed files.
4. **Put** the final hashes into `RELEASE-NOTES-DRAFT.md` and remove its DRAFT banner.
5. **Then, only with explicit authorisation:**
   - push, tag and create the GitHub release (Windows-only);
   - the forum post;
   - the Discord post.

## Safety record

- Non-lab processes terminated: **0**.
- The only processes stopped were this mission's own `mem2` waiter, PIDs 959, 960 and 1183, by PID (`evidence/logs/procs.txt`).
- Windows Search, the page file and the WSL distributions were not touched. No protection was disabled.
- The certified patches, testing-7 and makeobj were not modified.
