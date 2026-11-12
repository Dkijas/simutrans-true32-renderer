# Microsoft Defender scan (`logs/defender-scan.txt`, `tools/defender.ps1`)

**Engine:** Defender 4.18.26100.4, signatures 1.463.257.0 (updated 2026-11-11 23:12), real-time protection **on**.

**Command:** `MpCmdRun -Scan -ScanType 3 -File <f> -DisableRemediation`. The scan reports and does not quarantine. No exclusion
was added, no setting was changed, and no protection was turned off.

| Scanned | Result |
| --- | --- |
| `simutrans-…-testing-8-candidate-win64-sdl3.zip` (`961deb01…`) | found no threats |
| `simutrans-…-testing-8-candidate-win64-sdl2.zip` (`bc11f538…`) | found no threats |
| `xr32-night-prototype-r12377-testing-8-candidate-source.tar.gz` (`5c574389…`) | found no threats |
| the four patches | found no threats |
| the four executables, **unpacked** from the zips and checked against their certified hashes | found no threats |
| both unpacked trees (952 files each) | found no threats |

**Result: 0 detections.**

**Limitation:** the scanner's ability to detect was not demonstrated with a control (an EICAR test file). That would raise a
real-time alert and a quarantine entry on the user's machine, so it is left to the user's decision.
