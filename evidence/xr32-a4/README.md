# Evidence of the A4 certifications and of the testing-4 package

Selected files of the labs, verbatim except that local machine paths are replaced by placeholders. The reports are
in `reports/xr32-a4/`.

- `a4/` (the first candidate 84b3225):
  - `rootcause-probe-t3.txt`, `probe-cand.txt`: generated coast tiles with an XR32 node, testing-3 and candidate.
  - `final-gates.txt`: the composition oracle, makie precision, night (relative), zoom geometry, H1 and plain-pak
    identity, on the frozen binary.
  - Presentation oracle: `oracle-matrix-all.txt`, `switch-frames.txt`.
  - `warning-sets.txt`, `asan-*.txt`, `shutdown-mem-a4.txt`.
  - Regression battery: `battery-vs-testing3.txt`, `a6-extra-ids.txt`.
  - `reinit-probe.txt`.
  - Composition counters: `derive-counters-*`, `perframe-compose.txt`.
  - World captures: `visual-*.txt`.
  - `freeze-candidate.txt`, `candidate-binaries.sha256`.
- `a4c/` (the testing-4 code):
  - `gates-cache.txt`: the acceptance gate, byte-identical draws to 84b3225, with the stale-key mutant control.
  - The same presentation, warning, sanitizer, memory and battery evidence.
  - `a6-vs-a4.txt`: the pak64 pictures.
  - `rebuild-cost.txt`, `a4c-bench-summary.txt`: rebuild cost and frame time, three-way.
  - `h1-holes*.txt`: the zoom 2/1 sand-gap finding.
  - `reinit-probe-cache.txt`, `freeze-cache.txt`.
  - `logs/suites-win.txt`: the automated tests, 309 / 309.
- `testing-4/` (the package):
  - `patches-t4.txt`: the patch routes, and the recipe checked on testing-3's published diff.
  - `inputs-t4.txt`: the certified binaries and testing-3's published assets, hash-checked.
  - `pkg-t4-*.txt`: the packages derived from testing-3's published archives, with the recipe checked on them.
  - `release-t4.txt`: the source archive recipe, checked on testing-3's.
  - `smoke-t4-*.txt`: each package unpacked and run.
  - `release-images.txt`.

Raw run outputs (per-frame CSV files, picture dumps, sanitizer logs) are not included.
