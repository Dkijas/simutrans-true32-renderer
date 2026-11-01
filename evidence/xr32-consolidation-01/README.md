# Evidence — XR32-CONSOLIDATION-01 (r12377 testing-2)

Derived summaries only: no binaries, no paksets, no raw logs. Local machine paths are replaced by placeholders
(`<lab>` = the consolidation laboratory, `<release-lab>` = the packaging laboratory of testing-2, `<testing-1-lab>` =
testing-1's, `<wsl-...>` = their Linux work directories). Labels: `base` / `*-base` = the published testing-1 binary
(the control), `cand` / `fin2` / `fin3` / `i6` = the candidate (fin2 = Linux SDL2, fin3 = Linux SDL3), `xr1` / `xr0` =
`SIMUTRANS_XR32` on / off.

## `consolidation/` — the certification of the candidate (`reports/XR32-CONSOLIDATION-01.md`)

| File | What it shows |
| --- | --- |
| `identity-*.txt`, `audit-*.txt` | A3: the shared-image test pak, candidate 0 failures, testing-1 6; the audit of real paks |
| `a5-*.txt`, `a5-daynight-callers.txt` | A5: interface images at night levels 0-13, toolbar brightness, world night unchanged; who asks for `daynight=false` |
| `n1-*.txt` | N1: interface base-size draw at 3/8 |
| `a1-base.txt`, `a1-align-i6.txt`, `a1box-*.txt` | A1 / N5: size and position of every image at 12 zoom steps; the visible (lit-pixel) box |
| `a2-*.txt` | A2: the catenary wire against the 16-bit renderer |
| `a6-*.txt` | A6: the two zoom routes, with the 3/2 vs 5/4 control |
| `fit-parity-cut-*.txt` | toolbar icon sizes against 16-bit (testing-1's code at I3, and final) |
| `final-analysers*.txt` | thin images, DAY/NIGHT, MRES, alpha on the final candidate |
| `warning-sets.txt` | compiler warning sets against testing-1, with an injected-warning control |
| `asan-final.txt` | AddressSanitizer / LeakSanitizer runs of the final source |
| `perf-zoom-final.txt`, `perf-load.txt`, `shutdown-mem.txt` | performance and memory against testing-1, per-row binary hash |
| `void-runs-README.txt` | why two earlier performance runs are void (shared executable, ETXTBSY) and were redone |
| `phase4-shutdown.csv` | Linux SDL3 clean exit per video driver in WSLg (dummy, auto, x11, wayland) |
| `smoke-exes.txt` | the hashes of the certified Windows executables |

## `packages/` — the certification of the published packages themselves

| File | What it shows |
| --- | --- |
| `package-vs-testing1.txt` | every package's members against testing-1's: only the executables, the diff and the texts differ |
| `patches-routes.txt` | the three patch routes (on testing-1, in one, on `svn export -r 12377`) give the source tree byte for byte; control |
| `testkit-build.txt`, `testkit-linux.txt` | the kit's 13 paks rebuilt by both makeobj builds equal testing-1's; control |
| `tram-probe.txt` | the kit tram's duplicate-night warning: testing-1 6, testing-2 0 |
| `smoke-setup.txt`, `linux-from-staging.txt` | the executables extracted from the staged packages are the certified ones |
| `package-analysers.txt` | identity, A5, N1, fit parity, thin, DAY/NIGHT, MRES, alpha on the extracted packages, with testing-1 controls |
| `package-battery.txt` | the suite, gate and log battery on the extracted packages, per platform |
| `package-linux-gates.txt` | identity, A5, N1, A6, A2, A1 on the extracted Linux packages, with testing-1 as control |
| `shutdown-mem.txt` | Windows clean shutdown and peak memory of the packaged executables against testing-1 |
| `ARCHIVE-CONTENTS-AUDIT.md` | no local path, personal data or build leftover in any package; planted control caught |
