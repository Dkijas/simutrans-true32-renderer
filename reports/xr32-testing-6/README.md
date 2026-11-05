# XR32 testing-6 — reports

testing-6 = testing-5 + A12: the generated climate and snow tiles are composed at 8 bits from the XR32 data of
`ClimateTexture` / `LightTexture`.

- `windows-certification/` — the Windows x64 SDL2 + SDL3 certification of the testing-6 code (mission
  SIMUTRANS-XR32-TESTING6-READINESS-01), **verdict B, certified with limitations**. Lab reports:
  - `CERTIFICATION-REPORT.md` — verdict, answers, limitations, incidents;
  - `A12-INTEGRATION.md` — identity and provenance of the code (testing-5's public tree + the two A12 commits, by two
    independent routes), reproducibility on Windows and Linux;
  - `VISUAL-COMPARISON.md` — 16-bit / testing-5 / testing-6 on the same scenes, day and night, and the brightness
    analysis;
  - `WINDOWS-TEST-MATRIX.md` — every check on SDL2 and SDL3, each with the control that shows it can fail;
  - `PERFORMANCE.md` — frame time, the cost of light changes and zoom steps, cache rebuild counters, memory, and the
    A12-P1 cache proposal (not implemented);
  - `A12-D-TRANSITION-MASKS.md` — the 32-level transition masks (accepted limitation) and the options for later;
  - `KNOWN-ISSUES.md` — the lab's own known-issue list.
- `../xr32-a12/` — the A12 work itself: root cause, architecture options, implementation.
- The **Linux builds of testing-6 are experimental and not certified**; their minimal checks are in
  `evidence/xr32-testing-6/`.

The lab captures are not included: they show PAK128.german art, which is not redistributed here. The artistic review of
the brighter terrain by the pak's author is pending. Paths of the lab machine are replaced by placeholders such as
`<testing-6-lab>`.
