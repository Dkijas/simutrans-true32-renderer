> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# Linux status

> **Linux x86_64 SDL2 and SDL3: Experimental / Not certified.** Prepared and smoke-tested; no Linux graphics certification.

Evidence: `evidence/linux-gate.txt` (the final run, after the README change; the first run gave the same builds and smoke results). Tool: `tools/linux_gate.sh`.

## Where and how

WSL 2, Debian 13.5, kernel 6.18 (Microsoft), GCC 14.2, glibc 2.41, CMake 3.31.6. **WSLg is not a native Linux desktop.** Release recipe of testing-6: Release, MT, `SIMUTRANS_USE_REVISION=12377`, `SOURCE_DATE_EPOCH=1793534400`, bzip2 and miniupnpc static, SDL3 3.4.12 static. Source: `git archive` of `xr32/testing-7-candidate` (`t7.tar`, sha256 `ec3e2abd…`), the same tree as Windows.

## Builds

| Build | sha256 | Check |
| --- | --- | --- |
| testing-6 SDL3 32-bit (recipe check) | `7a094d8b…` | equal to the published one |
| Candidate SDL3 32-bit | `08e1322e…` | equal to the previous mission's build |
| Candidate SDL2 32-bit | `530bb460…` | equal to the previous mission's build |
| Candidate SDL3 16-bit | `9822aee1…` | **byte-identical** to testing-6's |
| Candidate SDL2 16-bit | `3e7cd80e…` | **byte-identical** to testing-6's packaged file |
| makeobj | `482e041f…` | **byte-identical** to testing-6's |
| Warnings | 18 in each | |

## Packages

Derived from testing-6's **published** Linux tarballs, checked against its `SHA256SUMS.txt`. Recipe control: re-packing testing-6's own tree gives its published tarball byte for byte.

| Package | sha256 | Content |
| --- | --- | --- |
| `simutrans-…-testing-7-candidate-linux-x86_64-sdl2.tar.gz` | `a57f1237…` | `simutrans-rgba32-candidate` (`530bb460`), `simutrans-16bit-candidate` (testing-6's), full diff, README with the notice, SHA256SUMS.txt |
| `…-sdl3.tar.gz` | `e0ad1a1e…` | the same, SDL3 (`08e1322e`, 16-bit `9822aee1`) |
| `makeobj-…-testing-7-candidate-linux-x86_64.tar.gz` | `a9be27c9…` | makeobj `482e041f` |

Against testing-6, exactly the expected paths differ: README, SHA256SUMS, the two executables (renamed) and the diff. Executable bits: 2 of 2. Fixed metadata (owner 0, sorted, fixed mtime, `gzip -n`).

## Smoke (WSL 2)

Both backends, from the tarballs, in an empty folder:

| Check | SDL2 | SDL3 |
| --- | --- | --- |
| `SHA256SUMS.txt` inside holds | yes | yes |
| Architecture | ELF 64-bit x86-64 PIE | the same |
| Libraries not found | 0 | 0 |
| Highest GLIBC symbol | 2.38 | 2.38 |
| 32-bit, dummy video, mutant pak + coast world, until 1930.6, **clean exit** | exit 0, no FATAL/Error, 0 XR32 nodes skipped: PASS | PASS |
| 16-bit (control: must skip XR32 nodes) | exit 0, 192 skipped: PASS | PASS |
| 32-bit, keyed lab pak LT (the fix's path), terrain scene, until 1930.7 | exit 0, no error: PASS | PASS |
| 32-bit in a WSLg X11 window, until 1930.6 | exit 0: PASS | PASS |
| makeobj | runs, version 60.12 | |

## Not done

- No picture on Linux was compared with an oracle, and no frame was checked on screen.
- No native Linux desktop.
- No other distribution, no Wayland session outside WSLg.
- No ARM.

**What these results say:** the Linux packages are what Windows is, compiled for Linux. They start, load paks including the fix's path, and quit cleanly. **They do not say the pictures are right on Linux.**
