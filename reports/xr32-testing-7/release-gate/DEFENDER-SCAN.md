> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# Microsoft Defender

**No detection on the final packages.** Evidence: `evidence/defender-scan.txt` (final), `evidence/defender-scan-first-packages.txt` (first round). Tools: `tools/defender_gate.ps1`, `tools/defender_unpacked.ps1`.

| | |
| --- | --- |
| Product / engine | 4.18.26100.4 / 1.1.26100.4 |
| Signatures, first round | updated 1.463.180.0 → 1.463.181.0 |
| **Signatures, final round** | updated 1.463.181.0 → **1.463.187.0** |
| Update | `Update-MpSignature` and `MpCmdRun -SignatureUpdate`, before each round |
| Real-time protection | on |
| Method | `MpCmdRun -Scan -ScanType 3 -File <f> -DisableRemediation`; archives are scanned inside |
| Exclusions, settings changed, files altered | none |

## Final packages (after the README change)

| Scanned | SHA-256 | Result |
| --- | --- | --- |
| `simutrans-…-testing-7-candidate-win64-sdl3.zip` | `737419f3…` | found no threats |
| `…-win64-sdl2.zip` | `4b568695…` | found no threats |
| `makeobj-…-windows-x86_64.zip` | `95fe577c…` | found no threats |
| `simutrans-…-linux-x86_64-sdl3.tar.gz` | `e0ad1a1e…` | found no threats |
| `…-linux-x86_64-sdl2.tar.gz` | `a57f1237…` | found no threats |
| `makeobj-…-linux-x86_64.tar.gz` | `a9be27c9…` | found no threats |
| Source archive | `5122e566…` | found no threats |
| Test kit (testing-6's published file) | `200902e8…` | found no threats |
| Windows executables taken out of the zips: rgba32 and 16-bit, SDL3 and SDL2, each with its certified hash; makeobj | | found no threats (5 files) |
| The built executables in the run folder | | found no threats (5 files) |
| **Active threats; detections today** | | **0; 0** |

## Notes

- In the first round the game zips could not be unpacked under the lane's long folder: the paths went over Windows' 260-character limit. Two attempts left "NO VERDICT" lines in which **nothing was scanned**, and the evidence says so. A short folder fixed it.
- For a tester, "Extract all" of the largest zip into `Downloads` stays at **229 characters** (`evidence/zip-path-length.txt`).
- Historical D1 of testing-4 was a detection; nothing comparable here.
