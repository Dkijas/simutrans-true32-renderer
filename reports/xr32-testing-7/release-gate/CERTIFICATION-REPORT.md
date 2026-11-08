> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# Certification report: the gate of section 15

Each line: result, and the document that holds the evidence.

| Gate | Result | Where |
| --- | --- | --- |
| Code identical to `3f1b511` | **PASS**: commit and tree; one file; patches regenerate byte-identical; fresh builds hash-identical | `SOURCE-VERIFICATION.md` |
| NK fallback fixed | **PASS**: 304 → 0, SDL3 and SDL2 | `SPECIAL-COLOR-REGRESSION.md` |
| TK fallback fixed | **PASS**: 390 → 0 | ibid. |
| LT fallback fixed | **PASS**: 466 → 0 | ibid. |
| Independent oracle | **PASS**: 60 of 60 runs, tiles and transitions | ibid. |
| Negative controls | **PASS**: 4 engine mutants FAIL, 5 dump corruptions detected, testing-6 caught on every keyed pak; droprect caught; suite injection FAILs; day/night and PAK-OLD/NEW controls differ | ibid., `WINDOWS-TEST-MATRIX.md` |
| Windows SDL2 validated | **PASS** with limitations | `WINDOWS-TEST-MATRIX.md` |
| Windows SDL3 validated | **PASS** with limitations | ibid. |
| No visual regressions | **PASS**: keyless scenes 0 px from testing-6 in real windows | `VISUAL-COMPARISON.md` |
| Dirty regions correct | **PASS**: 1,740 verified frames without mismatch (coast world 540, keyed corners 480, `windows` repeated on an idle machine 720), droprect caught (108 of 240). One run while another program was in use: 8 of 60 frames, not reproduced in the 720 | ibid. |
| A4 and A11 intact | **PASS**: unchanged code paths; mutant pak frames, coast frames and 2/1 byte identity (previous mission, same binaries) | `evidence/prev/` |
| TCC intact | **PASS**: real windows day and night, 0 px; all images of PAK128.german 0 differ (previous mission) | `WINDOWS-TEST-MATRIX.md` |
| A12 intact | **PASS**: the RGBA32 + 32-level model exact | `SPECIAL-COLOR-REGRESSION.md` |
| Memory acceptable | **PASS**: no sustained growth in 40 min; same warm-up as testing-6 | `MEMORY-STRESS.md` |
| Performance documented | **PASS**: no measurable difference without keys; +2.0 to +3.0 ms per zoom step with keys, reproduced | `PERFORMANCE.md` |
| Linux experimental prepared | **PASS**: built, packaged, smoke-tested under WSL 2; Experimental / Not certified | `LINUX-STATUS.md` |
| Defender without detections | **PASS** | `DEFENDER-SCAN.md` |
| Packages intact | **PASS**: derived from testing-6's published ones, only the expected members differ, executables re-hashed inside, zip twice = same bytes | `RELEASE-MANIFEST.md` |
| SHA256SUMS correct | **PASS** | `SHA256SUMS`, `RELEASE-MANIFEST.md` |
| Documentation without exaggerated claims | checked: limits stated in every report; makie credited, no certification attributed | `KNOWN-ISSUES.md` |
| No uncertified functional changes | **PASS**: the coast fix is not integrated; nothing else changed | `SOURCE-VERIFICATION.md` |

**No critical gate failed.** The limitations — no physical keyboard, Linux not certified, makie's older file unavailable, one machine — keep the verdict at **B**.
