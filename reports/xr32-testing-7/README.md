# XR32 testing-7 — reports

testing-7 = testing-6 + the special colour fallback fix: a generated climate or snow tile no longer falls back to its
16-bit (RGB555) picture because a pixel of `LightTexture` or `ClimateTexture` happens to equal a special colour.

- `release-gate/` — the Windows x64 SDL2 + SDL3 release gate of the testing-7 code (mission
  SIMUTRANS-XR32-TESTING7-RELEASE-GATE-01), **verdict B, certified with limitations**. Lab reports:
  - `CERTIFICATION-REPORT.md` — the gate, line by line;
  - `SOURCE-VERIFICATION.md` — identity of the code, the patches, the builds;
  - `SPECIAL-COLOR-REGRESSION.md` — the three lab fixtures, the controls, night and players, the mutants;
  - `WINDOWS-TEST-MATRIX.md`, `VISUAL-COMPARISON.md` — every check on SDL2 and SDL3, real windows;
  - `PERFORMANCE.md`, `MEMORY-STRESS.md` — cost per event and the long sessions;
  - `SDL3-ZOOM-INVESTIGATION.md` — the keyboard question (focus, not a defect of the build);
  - `LINUX-STATUS.md`, `DEFENDER-SCAN.md` — Linux (experimental) and the security scan of the candidate packages;
  - `KNOWN-ISSUES.md` — the gate's own list.
- `special-colour-fallback/` — the investigation and the fix (mission SIMUTRANS-XR32-TESTING7-SPECIAL-COLOR-FALLBACK-01):
  root cause, the semantics of special colours, the implementation, the independent oracle, regressions, the comparison
  with makie's published resources, the coast-water case (out of scope) and the light-map crash of official r12377.
- The **Linux builds of testing-7 are experimental and not certified**; their minimal checks are in
  `evidence/xr32-testing-7/`.

The lab captures are not included: they show PAK128.german art, which is not redistributed here. makie has not
reviewed or tested this build. Paths of the lab machine are replaced by placeholders such as `<testing-7-gate-lab>`.
