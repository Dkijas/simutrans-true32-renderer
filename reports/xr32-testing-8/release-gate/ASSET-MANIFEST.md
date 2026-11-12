# Asset manifest — testing-8 candidate (`deliver/candidate/`, `deliver/patch/`)

**Every name still carries `candidate`.** No file has been renamed, published, uploaded or attached anywhere.

## Release assets (candidate)

| File | Bytes | SHA-256 | Contents |
| --- | ---: | --- | --- |
| `candidate/simutrans-xr32-night-prototype-r12377-testing-8-candidate-win64-sdl3.zip` | 17,613,379 | `961deb01492f75bb29778b622773a3659dd68e430cf90b18017e6b56faedcbf1` | Windows x64, SDL3: 952 entries |
| `candidate/simutrans-xr32-night-prototype-r12377-testing-8-candidate-win64-sdl2.zip` | 15,797,917 | `bc11f538cfd2a8e91f6b481206de39581a492d202298a82046e6eef9093938bf` | Windows x64, SDL2: 952 entries |
| `candidate/xr32-night-prototype-r12377-testing-8-candidate-source.tar.gz` | 5,420,378 | `5c5743891f42f8a50306025c5bec2e1b65c81589f5c66d8f64fac7eac5e5f063` | the full source of `3200e8e`, 2,335 entries (`git archive`, deterministic gzip) |
| `patch/XR32-NIGHT-PROTOTYPE-FULL.r12377-testing-8-candidate.diff` | 548,827 | `e7df3935df785c70a1f741ce50738e6e11d67bdbdd8a8abbfc58e4344a2f8550` | everything XR32 changes against r12377 (`7cc6d45..3200e8e`, full index, binary) |
| `patch/01-A4.diff` | 1,462 | `e1e6edff66d7c8817861e35ff1698895ef769442bbcbe2f45e604789db0b0aaa` | `3f1b511..b03d729` |
| `patch/02-A12-P1.diff` | 12,638 | `cb38cf5c98f7f2bf47a1fab2e4df79a3c8e704c36bca77db7f5db9d4e8de2e30` | `b03d729..a644622b` |
| `patch/03-TCC-R.diff` | 2,040 | `0817c4e75d2beeac5b15ed7daaafbde9491190e5fda94287057485d63e3a5c44` | `a644622b..3200e8e` |

## Inside each zip (both backends; `logs/package-check.txt`)

| Item | SDL3 | SDL2 |
| --- | --- | --- |
| `simutrans-rgba32-candidate.exe` (XR32) | `6f32326414e6a06c…` = build F3-s3 | `6f3dbd0451094db4…` = build F3-s2 |
| `simutrans-16bit-candidate.exe` | `8feaaf49317cf1d7…` = **testing-7's 16-bit** | `20cbf7da7e183f52…` = **testing-7's 16-bit** |
| PE machine | 0x8664 (x86-64), both | 0x8664, both |
| `patches/XR32-NIGHT-PROTOTYPE-FULL…candidate.diff` | = `patch/` file | = `patch/` file |
| `patches/upstream-build-fix/H-r12362-cmake-ole32.diff` | build-only fix, as in every XR32 release | the same |
| `README-TESTERS.txt`, `SHA256SUMS.txt` (inner: 5 lines, 0 mismatches), `THIRD-PARTY-STATIC.txt` | yes | yes |
| upstream files (`readme.txt`, `license*.txt`, `copyright.txt`, `history.txt`, `thanks.txt`, …), `ai/`, `config/`, `font/`, `music/`, `script/`, `text/`, `themes/` | the same 952-entry layout as testing-7 | the same |
| CRC test | OK | OK |

**No pakset is included, and no art of ours.** The only data files are Simutrans' own base files (fonts, themes, music, texts), the same as in testing-7, so no art without permission is shipped.

## Checks on the assets

| Check | Result | Where |
| --- | --- | --- |
| ZIP integrity (CRC) | PASS, both | `logs/package-check.txt` |
| exe in zip ↔ fresh build of `3200e8e` | byte-identical, all 4 | the same |
| source ↔ commit | the tar is `git archive` of `3200e8e`; the full diff and the three patches recomputed from the commits: identical | `SOURCE-VERIFICATION.md` |
| private paths, user name, mail, lab names (the lab machine's home and lab paths, the user's name and mail address) | **0** in the patches and in the zips' text. In the source tar: 2 hits, both upstream text also present in testing-7's published source ("Victor Sørensen" in `translate_users.txt`, the miniz author's address) | `SOURCE-VERIFICATION.md` |
| credentials: private keys, GitHub / Google / AWS / Slack tokens, OAuth secrets, password or token assignments, in every text member of the zips, the tar and the patches | **0 hits**; CONTROL: a synthetic text with one of each pattern hits all 7 | `tools/cred_scan.py`, `logs/cred-scan.txt` |
| A10 code (`l_opq`) | 0 in candidate source and patches (the check finds it in the A10 record) | `INTEGRATED-COMMIT-IDENTITY.md` |
| LEGACY16-SC code | 0 | the same |
| Microsoft Defender | 0 detections, including the unpacked executables and trees | `DEFENDER-SCAN.md` |

## makeobj

**Not included. Its function is unchanged:** testing-8 changes only `display/stlab_xr32_hook.cci` and `display/stlab_xr32_zoom.cci`.
Only `display/simgraph32.cc` includes them, and makeobj's build (`src/makeobj/CMakeLists.txt`) compiles no `display/` file. So
every source file compiled into makeobj is identical between testing-7 and testing-8.

**testing-7's published makeobj is the same functional version.** Testers who build paks keep using it. makeobj was not
rebuilt, so no byte-identity claim is made for a rebuilt binary.

## Not included

- **Linux packages:** not built. The testing-7 Linux packages are not reused or renamed.
- **A10:** excluded.
- **LEGACY16-SC:** excluded.

## To fix at the renaming step (text only, not functional)

The renaming step also edits each zip's `README-TESTERS.txt`, then recomputes every hash and re-runs the package check.

`README-TESTERS.txt` says light changes are about "15-19 % less time on a test machine". That is the A12-P1 lane's measurement.
This gate measured **12.8 %** on its pyramid scene (`BENCHMARK-INTEGRATED.md`). The README should then say "about 13 %" or
"13–19 %".
