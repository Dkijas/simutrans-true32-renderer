# Source verification

## Resources at the start (2026-11-12 ~03:00)

| | |
| --- | --- |
| C: free | 25.2 GB |
| RAM free | 13.5 of 28.8 GB |
| committed memory | 18.1 / 45.8 GB (40 %) |
| WSL | Ubuntu and Debian stopped; **not used and not modified** in this gate |
| toolchain | MSYS2 MinGW-w64 GCC 16.1.0, CMake (mingw64), GDB 17.2 |
| residual processes of these labs | none |

Windows Search, the page file and the WSL distributions were not touched, and no process outside this lab was stopped.

## Sources

| | |
| --- | --- |
| lab repo | `src/`, a clone of `<testing8-graphics-zoom-1-lab>/src`, plus `a12/*` = the A12-P1 lane's branches, fetched read-only |
| candidate | `3200e8e8bbc84880f74a32a2c5e3fb5d6c2d73de`, tree `70e54fc4a865d0ba850f2083df0c33d06a597aaa` (`INTEGRATED-COMMIT-IDENTITY.md`) |
| public testing-7 | `ae28a613cef07b77235fced2ab54439f02dcb1d4`; its `source/xr32-r12377` tree = `562448a2` = `3f1b511` |
| r12377 | `7cc6d452be63c55943d650bc5a72fce27ec54853` (not rebased) |

## Builds of the final executables (`logs/builds.txt`)

Fresh, non-incremental, one at a time:
- MSYS2 MinGW-w64 GCC 16.1, Release, MT, static, revision 12377;
- the build-only fix H (`old32` → `ole32`), the same as every XR32 release;
- `SOURCE_DATE_EPOCH` fixed.

| Build | sha256 | Reproducibility |
| --- | --- | --- |
| F3-s3 (XR32, SDL3) | `6f32326414e6a06c4130503a4e8dc838…` | built **twice** here: identical; identical to the previous lab's t8c-s3 |
| F3-s2 (XR32, SDL2) | `6f3dbd0451094db4ea5acbf12a7a1cc2…` | identical to the previous lab's t8c-s2 |
| F3-s3-16 (16-bit, SDL3) | `8feaaf49317cf1d7…` | **identical to testing-7's published 16-bit** |
| F3-s2-16 (16-bit, SDL2) | `20cbf7da7e183f52…` | **identical to testing-7's published 16-bit** |
| twins B0p, B1p, B2p (SDL3, SDL2) | `logs/builds.txt` | **byte-identical to the A12-P1 lane's prof-b0 / prof-b1 / prof-p1**, so their earlier certification applies to these exact binaries |
| twin B3p (B2p + TCC-R) | `8b46d0c9…` / `f63d83a2…` | new; TCC-R's changed lines identical to `3200e8e`'s |
| stress S3, control SN | `d93a4803…` / `32cc2373…` | the A12-P1 lane's stress instrumentation, unchanged |

The warning sets of F3 equal testing-7's (SDL3 and SDL2).
