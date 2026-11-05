> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# A12 integration into the testing-6 candidate

## Identity

| | |
| --- | --- |
| testing-5 (public) | tag `xr32-prissi-night-image-prototype-01-r12377-testing-5` → `cfcc03c6d713b7d645808433ee8ce84ec0c6b19e`; `source/xr32-r12377` = tree `c8297dcac4547b2a425058ba8723c96e2eeadb33` (fetched anonymously for this mission) |
| A12 commits | `4c71953f4215bb619bbc177b83f5ad09e1eb1166` (A12-A ClimateTexture), `f04638adc06130dc73121df50f9f2f0c89fd4e1d` (A12-B LightTexture) |
| testing-6 candidate | **`f04638adc06130dc73121df50f9f2f0c89fd4e1d`, tree `b0efabef4936001011440a02c4dbcd9cd9052650`**, the certified A12 candidate, unchanged |
| Official base | SVN trunk r12377 (git `7cc6d45`); no rebase to a later trunk |

## Provenance check (`evidence/A-provenance.txt`)

1. **Public base:** the public testing-5 `source/xr32-r12377` tree equals the tree of `038b538`, the testing-5 functional source.
2. **Cherry-pick route:** a commit was made from the public tree alone (`4ccc55bc`, tree `c8297dca`). Cherry-picking `4c71953`, then `f04638a`, onto it gives tree `b0efabef`, **equal** to `f04638a`.
3. **Patch route:** the public tree with the A12 delivery's patches `01` + `02`, applied to the index only (no worktree, no line-ending conversion), gives `b0efabef`, **equal**.
4. **Control:** the public tree with `01` alone gives `7d2195a6`, the tree of `4c71953`, not the candidate.
5. **Files changed testing-5 → candidate** (187 insertions, 0 deletions):
   - `descriptor/ground_desc.cc` (+12);
   - `display/stlab_xr32.h` (+5);
   - `display/stlab_xr32_hook.cci` (+170).
6. **XR32 format:** no file of the reader, writer or loader is touched (`image_writer`, `stlab_xr32.cc`, `stlab_xr32_load.cci`, `pakset_manager`, any `writer/`). makeobj is byte-identical (Windows `8a150381`, Linux `482e041f`), and `xr32_census` reads makie's paks identically.
7. **A12 delivery:** its `SHA256SUMS` verifies. Its patches are byte-identical to the ones regenerated here (`evidence/G-patches-vs-a12.txt`).

**Incident:** the first cherry-pick attempt ran with an empty base, because `commit-tree` rejected the `@0` date. It picked onto `f04638a` itself and is void. It is recorded and was redone on the correct base (`evidence/A-provenance.txt`, second block).

## Reproducibility (`evidence/G-builds-vs-certified.txt`, `evidence/builds-linux.txt`)

**Windows.** The recipe of the testing-5 and A12 certifications, on fresh non-incremental builds. Every one of the 11 builds equals the hash certified in A12, byte for byte:
- candidate SDL3 `ed58e66d`, SDL2 `b002d5a9`, 16-bit `8feaaf49` / `20cbf7da`, makeobj `8a150381`;
- BASE `f7f7ce82` / `e82a2ce8`;
- lab twins `f9e42af8`, `f6eeeaec`, `53809471`, `7d6d3f8c`.

BASE SDL3 is the executable of the published testing-5 package. Warning sets: BASE = candidate on every pair.

**Linux.** The release recipe of testing-4/testing-5 on WSL Debian 13.5, GCC 14.2:
- **Recipe check:** the testing-5 source rebuilds the **published** testing-5 Linux binaries byte for byte (SDL2 `49686ccf`, SDL3 `a65bf4f2`).
- **Candidate:** SDL2 `a3eef27e`, SDL3 `7a094d8b`. Its 16-bit binaries (`3e7cd80e` / `9822aee1`) and makeobj (`482e041f`) equal testing-5's published files.
- Warning sets are identical; maximum GLIBC symbol 2.38.

## Environment (`evidence/lane-setup.txt`, `evidence/toolchain.txt`)

| | |
| --- | --- |
| Clone | `git clone -c core.autocrlf=false`. Git for Windows' system config has `core.autocrlf=true`, which caused the CRLF trees of earlier lanes. Index/worktree EOL mismatches 0; status clean; `src/simutrans/builder` present and not ignored. |
| Windows toolchain | MSYS2 GCC 16.1.0, CMake 4.4.0, SDL2 2.32.10, SDL3 3.4.12, zlib 1.3.2, libpng 1.6.58, zstd 1.5.7, FreeType 2.14.3, bzip2 1.0.8 |
| Linux toolchain | Debian 13.5, GCC 14.2.0-19, glibc 2.41 (binaries need ≥ 2.38), CMake 3.31.6, SDL3 3.4.12 static |
| Labs left untouched | the testing-5 publication lab (its 1,701 line-ending-only changes) was not used as a source, and was neither cleaned nor written. The A12 lab was only read: its delivery verifies. |
