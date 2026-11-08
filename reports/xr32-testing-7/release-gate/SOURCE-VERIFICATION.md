> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# Source verification

Evidence: `evidence/source-verification.txt`, `evidence/builds-gate.txt`, `evidence/installed-exes.sha256`.

## The candidate is exactly `3f1b511`

| Check | Result |
| --- | --- |
| Commit and tree | `3f1b511be97e9fb122780c3da42c7fd07f29e2fc`, tree `562448a2cdf090543f7890e8a0fc2300ced85e86`, both as certified |
| History on top of testing-6 | `defca45` (light map), `3f1b511` (climate texture); nothing else |
| Base | `f04638a`, tree `b0efabef…`, equal to the public `08c62288:source/xr32-r12377` |
| Files changed against testing-6 | **one**: `src/simutrans/display/stlab_xr32_hook.cci`, +66 −17 |
| Files changed against r12377 | the same 51 as testing-6; 0 differ in the list |
| XR32 format, makeobj, 16-bit renderer sources | **0 files touched** (`stlab_xr32.cc/.h`, `descriptor/writer/*`, `src/makeobj/*`, `simgraph16.cc`) |
| The changed file in the 16-bit build | included only by `simgraph32.cc`: the 16-bit renderer cannot see it |

## Patches

The four certified diffs, regenerated from the repository with the published recipe (`git diff --full-index --binary`), are **byte-identical** to the certified files:

| File | sha256 |
| --- | --- |
| `XR32-NIGHT-PROTOTYPE-FULL.r12377-testing-7-candidate.diff` | `41867300…` |
| `T7-ON-TESTING-6.diff` | `c6377153…` |
| `01-A12-C-LIGHT-KEYS.diff` | `6c050f05…` (also the corner mission's file) |
| `02-A12-D-TEXTURE-KEYS.diff` | `13cb6279…` |

Applied to clean trees, each rebuilds tree `562448a2` exactly: the full diff on r12377, `T7-ON-TESTING-6` on `f04638a`, and 01 then 02 on `f04638a`. **Control:** 02 alone on testing-6 refuses to apply.

## Fresh builds against the certified hashes

`tools/build_gate.sh` (fresh, `git archive`, the release recipe). **Every one equal:**

| Build | sha256 |
| --- | --- |
| SDL3 32-bit | `fce1ab24…` |
| SDL2 32-bit | `f2901545…` |
| SDL3 16-bit | `8feaaf49…` = testing-6 |
| SDL2 16-bit | `20cbf7da…` = testing-6 |
| makeobj | `8a150381…` = testing-6 |
| testing-6 from `f04638a` (recipe check) | `ed58e66d…` = published |
| Lab twin SDL3, SDL2 (`STLAB_PERF`) | `e027fca2…`, `fd65a64e…` = previous mission |
| Counters twin | `84d21471…` = previous mission |

**No SOURCE_MISMATCH.**

## Private data

The full diff's added lines were searched for user paths, user names, e-mail addresses, keys and passwords: 3 hits, all the word `token` in the semantic-mask file-name parser, which is code. No private data.

## Lanes

| Lane | Role |
| --- | --- |
| `<testing-7-gate-lab>` | this mission; its `src` is a clean clone (`core.autocrlf=false`) of the previous lane's |
| `<testing-7-fix-lab>` | previous certification: read only. Its `deliver/SHA256SUMS` re-checked: all files intact |
| `<testing-6-publication-lab>` | testing-6's published assets: read only, the base of the packages |
| `<tcc-consolidation-lab>` | TCC scene and paks: read only (hard-link copy; the scenario folder copied for real, since the runner writes there) |
