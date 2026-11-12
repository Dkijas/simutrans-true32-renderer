# Evidence — XR32 testing-8

Selected text evidence. Paths of the lab machine are replaced by placeholders (`<testing-8-gate-lab>`, `<dev>`, ...).

- `release-gate/` — the final Windows x64 SDL2 + SDL3 release gate of testing-8 (see `reports/xr32-testing-8/`): builds
  and identity, the automated suite, the translucent-colour oracles and their swap controls (`tcc/`), screen updates and
  their drop control, the zoom 2/1 check, memory with its synthetic-leak control, the thread stress with its lockless
  control, performance with its A/A control, the package, credential and Defender checks of the candidate packages.
- `publication/` — the publication of testing-8: the source gate (testing-7's public source + the three patches gives the certified tree; one patch alone does not), the Linux builds with their recipe control (the same recipe on testing-7's
  commit re-makes testing-7's published binary byte for byte), the derivation of the four packages from testing-7's
  published archives (exactly the expected files change, also against the certified candidate packages), the source
  tarball and diffs with their recipe control, the manifest, the **essential** Linux checks only (WSL 2 is not a native
  Linux desktop) and the final Microsoft Defender scan. Raw per-frame outputs and captures are not included.
