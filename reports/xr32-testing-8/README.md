# XR32 testing-8 — reports

testing-8 = testing-7 + three changes, one commit each: A4 (coast water keeps its 32-bit picture), A12-P1 (a 16 MiB cache of
composed terrain tiles) and TCC-R (translucent company colours keep their share when zoomed out).

- `release-gate/` — the final Windows x64 SDL2 + SDL3 release gate of the integrated code (mission
  SIMUTRANS-XR32-TESTING8-FINAL-RELEASE-GATE-01), **verdict B, certified with limitations**. The gate certified the
  candidate as technically ready for a publication decision; publication was authorised separately afterwards. The files
  there still say `candidate` and "not authorised": that is the state at the time of the gate. Lab reports:
  - `TESTING8-FINAL-VERDICT.md` — the verdict, every gate with its control, the limitations;
  - `SOURCE-VERIFICATION.md`, `INTEGRATED-COMMIT-IDENTITY.md` — identity of the code, the patches and the builds;
  - `TCC-R-REAL-WINDOW-VALIDATION.md` — translucent company colours in real windows, SDL3 and SDL2, day and night;
  - `DIRTY-REGIONS.md`, `SDL2-WINDOW-EVENTS.md`, `ZOOM-2-1-PIXELS.md` — screen updates, the inherited SDL2 behaviour and
    the 8 pixels at zoom 2/1, each against testing-7;
  - `MEMORY-STRESS.md`, `THREAD-SAFETY.md`, `BENCHMARK-INTEGRATED.md` — long sessions, the thread stress, performance;
  - `REGRESSION-MATRIX.md`, `WINDOWS-CERTIFICATION.md`, `PLATFORM-SCOPE.md`, `DEFENDER-SCAN.md`, `ASSET-MANIFEST.md`.
- The **Linux builds of testing-8 are experimental — not fully tested or certified on native Linux**. They were built
  for the publication from the same commit; their basic checks are in `evidence/xr32-testing-8/publication/`.

No captures are included: they show PAK128.german art, which is not redistributed here. Paths of the lab machine are
replaced by placeholders such as `<testing-8-gate-lab>`.
