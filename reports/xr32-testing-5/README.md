# XR32 testing-5 — reports

testing-5 = testing-4 + two fixes: translucent company colours (TCC) and the zoom 2/1 coast pixels (A11).

- `windows-certification/` — the Windows x64 SDL2 + SDL3 certification of the testing-5 code (mission
  SIMUTRANS-XR32-TESTING5-WINDOWS-01), **verdict B, certified with limitations**. Lab reports, in Spanish:
  - `CERTIFICATION-REPORT.md` — executive summary, verdict, limitations, recommendation;
  - `A11-ROOT-CAUSE.md` — the per-pixel cause of A11 (the classic 2/1 outline rule of `zoom_core_rezoom()` versus the
    pixel doubling of the XR32 zoom), proven with an independent model, the fix and the rejected alternatives;
  - `TCC-INTEGRATION.md` — how the certified TCC patch and the A11 fix were combined, and the proof that both are
    unchanged;
  - `WINDOWS-TEST-MATRIX.md` — every check on SDL2 and SDL3, each with the control that shows it can fail;
  - `PERFORMANCE.md` — frame time (P50/P95/P99), zoom cost including 2/1, memory, the A4 cache counters;
  - `KNOWN-ISSUES.md`, `README-LAB.md` — the lab's own known-issue list and package index.
- The **Linux builds of testing-5 are experimental and not certified**; their minimal checks are in
  `evidence/xr32-testing-5/publication/` (`smoke-t5-linux.txt`, `builds-linux.txt`, `pkg-t5-linux.txt`).
- `../xr32-tcc/` — the certification of the company-colour fix itself.

The lab captures are not included: the company-colour scenes show Pak192.Comic art, which is not redistributed here.
Paths of the lab machine are replaced by placeholders such as `<testing-5-lab>`.
