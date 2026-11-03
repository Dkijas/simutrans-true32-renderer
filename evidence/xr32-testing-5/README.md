# Evidence — XR32 testing-5

Selected text evidence. Paths of the lab machine are replaced by placeholders (`<testing-5-lab>`, `<dev>`, ...).

- `certification/` — Windows x64 SDL2 + SDL3 certification of testing-5 (see `reports/xr32-testing-5/`). Every check
  that matters has a control that shows it can fail (a knocked-out water pixel, the next tile's model, a stale-cache
  mutant, a droprect mutant, a synthetic warning, an injected heap overflow...).
- `publication/` — the packages: inputs (testing-4's published assets, hash-checked), the patch set and its apply checks,
  the package derivations (exactly the expected paths change), the source and diff recipes (they re-make testing-4's
  published files byte for byte), the Linux builds (the recipe re-makes testing-4's published Linux executables byte for
  byte), the **minimal** Linux smoke tests (WSL 2 / WSLg, not a native Linux desktop, not a certification), the makeobj
  cross-check (Windows and Linux makeobj write byte-identical paks on the whole test kit) and the Microsoft Defender scans.
