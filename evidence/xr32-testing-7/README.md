# Evidence — XR32 testing-7

Selected text evidence. Paths of the lab machine are replaced by placeholders (`<testing-7-gate-lab>`, `<dev>`, ...).

- `release-gate/` — the Windows x64 SDL2 + SDL3 release gate of testing-7 (see `reports/xr32-testing-7/`): build
  identity, the independent oracle per fixture, night planes and players, the engine mutants that must fail, screen
  updates with their control, the keyboard runs, memory, the Linux build and smoke tests, the Defender scan of the
  candidate packages. Every check that matters has a control that shows it can fail.
- `publication/` — the publication of testing-7: the source gate (testing-6's public tree + the two commits gives the
  certified tree, by history and by patches; one patch alone does not), the package derivations (exactly the expected
  paths change against testing-6 and against the certified candidate packages), the release assets with their recipe
  control, the repeat of the screen-update check on an idle machine, the **minimal** smoke tests of the final packages
  (WSL 2 / WSLg is not a native Linux desktop) and the final Microsoft Defender scan.
