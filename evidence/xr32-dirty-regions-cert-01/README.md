# Evidence of XR32-DIRTY-REGIONS-CERTIFICATION-01 (testing-3)

Selected files of the certification lab, verbatim except that local machine paths are replaced by placeholders
(`<lab>`, `<wsl-lab>` and similar). The reports are in `reports/`.

- `oracle-matrix.txt` / `.csv` - the read-back oracle per phase, scenario and configuration (mismatching pixels over
  the verified frames), with the void-lane rule and the mutant / no-C controls; `switch-frames.txt` - resize and
  fullscreen frame by frame; `theme-ref.txt`, `theme-masks.txt` - the theme-switch references against official r12377
  16-bit (pixel positions from picture dumps).
- `bench-manifest*.csv` - every run (oracle and benchmark): executable, its SHA-256 (`bench-exes.sha256`), environment,
  exit code; `bench-summary-*.txt`, `bench-rejected-*.txt`, `bench-comparison.txt` - the analyses.
- `regression-vs-testing2.txt`, `catworld-variation.txt`, `logs/battery-win.txt`, `logs/linux-analysers.txt`,
  `logs/linux*-pkg.txt`, `logs/suites-win.txt` - the regression battery against testing-2's published evidence.
- `asan-compare.txt`, `warning-sets.txt`, `shutdown-mem-t3.*` - sanitizers, compiler warning sets, clean shutdown and
  peak memory.
- `patches-t3.txt`, `pkg-t3-*.txt`, `linux-pub-install.txt`, `logs/builds-*.txt` - the patch routes,
  the derivation of the packages from testing-2's archives, the builds and their hashes.
- `audit-dirty-params.txt` - the audit that found the dirty flag `draw_rect` dropped.

Raw run outputs (per-frame CSV files, picture dumps, sanitizer logs) are not included.
