# Evidence — XR32 testing-6

Selected text evidence. Paths of the lab machine are replaced by placeholders (`<testing-6-lab>`, `<dev>`, ...).

- `certification/` — Windows x64 SDL2 + SDL3 certification of testing-6 (see `reports/xr32-testing-6/`). Every check
  that matters has a control that shows it can fail (a model of the wrong tile, a single patch instead of two, a
  mutant pak, a deliberately wrong oracle...).
- `publication/` — the publication of testing-6: the source gate (testing-5's public
  tree + the two A12 commits gives the certified tree by two independent routes; one patch alone does not), the
  independent Linux rebuild (identical bytes to the certified Linux binaries), the package derivations (exactly the
  expected paths change against testing-5 and against the certified packages), the **minimal** smoke tests (WSL 2 /
  WSLg, not a native Linux desktop, not a certification) and the Microsoft Defender scans.
