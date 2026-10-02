# RGBA32 / XR32 revised-10 — manifest

PUBLIC TESTING build. Not trunk, not stable, not an official Simutrans release.

## Base

| | |
|---|---|
| Simutrans base | SVN trunk **r12322** "Back to nightly mode" (125.0.1 nightly), `svn://servers.simutrans.org/simutrans/trunk` |
| 125.0 release reference | r12321 "Release of 125.0" |
| git mirror of r12322 | `f19fe4c8d` (release commit r12321: `b4b5f3d7f`) |
| export | `svn export --native-eol LF -r 12322`, 2201 files |
| previous public testing release | revised-9, tag `rgba32-revised-9-r12261-testing-1` → `bf3c98fda7c1a303d90e6e4c051692a90ef8d223` (unchanged) |

## The six patches (apply in this order, `patch -p1 --binary`, fuzz 0, no offset)

| # | patch | SHA-256 |
|---|---|---|
| 1 | RGBA32-CANDIDATE-REVISED-9.r12322.diff (revised-9 renderer, includes FIX-6) | `04cac01b3f91ea405463686817cc16faef6cd1ce07068e21b211709205b193a0` |
| 2 | MAKEOBJ-XR32-REVISED9.r12322.diff (closure A) | `7d4510d05a9e261666910fa174fa25ea04b58cb8e02bd3cd992612eb615ac806` |
| 3 | MRES-V2-WRITER.r12322.diff (closure B) | `ded0b499275f777bbb8dba59e63210e698664249a09d85a6df4969060d5fda21` |
| 4 | MRES-V2-RUNTIME.r12322.diff (closure C) | `fcd090f732a5fe360f113f3bdf737ac287fa8eded3381b32929e610de6f1e06d` |
| 5 | XR32-INTEGRITY-HARDENING.r12322.diff (closure D) | `c3509c4c597a9dafe0514b3c274ca0df3b37c5c0e115e00d313b3d03c681f386` |
| 6 | XR32-TRANSPARENCY-RENDERER.r12322.diff (R1) | `eee9b916e78156bbb0c615568754f1cb89e0ff67ec5a02cd005db81ef04d836f` |
| — | RGBA32-XR32-STACK-COMBINED.r12322.diff (= 1..6 in one) | `a8f118e1e224760fc9a03e33fcd14c2585c8a7f6d49ddaebf01359de23f7d471` |

Pre-rebase identities (r12261): revised-9 `484a4635…`, closure A `3ed88168…`, B `e211883d…`, C `82d1c9b5…`, D `6779541d…`, R1 `556b4d27…`. One rebase conflict (CMakeLists.txt, sdl3 block: trunk's re-worded comment kept with revised-9's TRUE32 conditional); every file only the stack touches is byte-identical to its certified r12261-stack version.

## Final source tree

| | |
|---|---|
| tree | clean r12322 + the six patches = clean r12322 + the combined patch (byte-identical) = the certified rebase candidate (0 differences) |
| files | 2220; 48 changed or added against r12322 (+8,950 / −821 lines) |
| manifest | sorted `sha256  path` list: `a8a05ed7e0f61d95f14603552657e002bcb395672fa628bef3eb11021b3645b2` |
| repository | `source/rgba32-revised-10-r12322` at commit `debaec0` (git archive of that commit = the tree above, 0 differences) |

## Build environments

| target | toolchain | flags |
|---|---|---|
| Windows x86_64 SDL2 / SDL3 | MSYS2 MinGW-w64 GCC 16.1.0, CMake 4.4.0, SDL2 2.32.10 / SDL3 3.4.12 static | Release, `SIMUTRANS_MULTI_THREAD=ON`, `SIMUTRANS_TRUE32=ON` (rgba32) / `OFF` (16bit), `SIMUTRANS_USE_REVISION=12322` |
| Linux x86_64 SDL2 | Debian 13, GCC 14.2.0, CMake 3.31.6, SDL2 2.32.4; bzip2 and miniupnpc static | same |

| executable | SHA-256 |
|---|---|
| Windows SDL2 `simutrans-rgba32.exe` | `561ed7c8cb00b451f1fcc50eb75af6773b46919bdb84aff1ec0a0aba17cf2c87` |
| Windows SDL2 `simutrans-16bit.exe` (legacy / control) | `4e6ff4dd07b73214867e5202957bf3c84bc5a55abeb3e838f8cf7d9aa10bfabe` |
| Windows SDL3 `simutrans-rgba32.exe` | `e987af25395f0da7ff695b9448fdc160560dc2034691dc110c8356029d51122a` |
| Windows SDL3 `simutrans-16bit.exe` (legacy / control) | `60aa4535adc91dee2431ef20d8f483ec181876f7092091fed2d8d068f4e9500d` |
| Linux `simutrans-rgba32` | `9eaaff3fa56d6a648ea2cde028f64c2480fa602a27d47908dab3606dc554b2b5` |
| Linux `simutrans-16bit` (legacy / control) | `6688b3bf54b1a8a88f568cd2cfc4fedc87856acdc313de0a9fc3e647f885c644` |
| Windows `makeobj.exe` (test kit) | `d0a55d8123ed9ec9e31101a5558374de3332d3624501b057b5e699e11c0458a9` |

Warning sets of all six game builds: identical to clean r12322 built the same way. 0 errors.

## Evidence

- makeobj (built from the commit): 23/23 reference paks byte-identical (9 E2E, 8 alpha fixture, 6 vehicle).
- Runtime oracles, instrumented build of the same tree: alpha 1x 761,208/761,208, with 2x 3,812,193/3,812,193 (control without R1: 475,095 and 2,470,953 — FAIL as expected); XR32 E2E 749,232 (lanes C and Craw) and 390,243 (lane A); MRES matrix and 2x 44,800/44,800; vehicle 728,028/728,028 and 2x 64,468/64,468; halo unchanged; blit PASS (788,928 / 7,095,970); enlargement 0/405,477 differ. Every line identical to the certified rebase run.
- Packages as extracted: automated tests 310/310 for all six executables; XR32+MRES test pak accepted (0 warnings), corrupted copy rejected (1 warning) — Windows native (SDL dummy) and Debian 13.

## Not re-tested on this base

Linux map-edge visual test (Xvfb), MSVC builds, Android, a native Windows desktop window, multi-monitor.

## Test kit paks

| pak | SHA-256 |
|---|---|
| STLab_AlphaFix-legacy.pak | `f199c76dc3623b037188b2802dd323dff7f352e7981e157e3c1b802922555486` |
| STLab_AlphaFix-xr32.pak | `c4aa7f37be81197e49d886bb73119abd1929c066c7a13a2b69c9acf4a6ec8f85` |
| STLab_AlphaFix-xr32-mres.pak | `877b09106f843899ce76070609032793ce0ee25cc764a5eddf075fdb6469803d` |

No S453 asset is distributed.
