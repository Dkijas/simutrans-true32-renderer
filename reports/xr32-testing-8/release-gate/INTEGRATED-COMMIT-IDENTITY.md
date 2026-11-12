# Integrated commit identity (`logs/identity.txt`, `tools/identity.sh`)

| | Commit | Tree |
| --- | --- | --- |
| r12377 base | `7cc6d452be63c55943d650bc5a72fce27ec54853` | `9f152f52e68553f9680d462d4b9654563bf25c58` |
| testing-7 source | `3f1b511be97e9fb122780c3da42c7fd07f29e2fc` | `562448a2cdf090543f7890e8a0fc2300ced85e86` |
| 1. A4 | `b03d729d53321af860b157213ee43a1da6e34db1` (parent `3f1b511`) | `509c21ecac069449602b8c6bbd0bd1e32abbedbb` |
| 2. A12-P1 | `a644622b6c6b034eedea46a6cf47e25a4fbac489` (parent `b03d729`) | `9e64d11f44f960b5a6296c4a82bd22447ec7457a` |
| 3. TCC-R = **candidate** | `3200e8e8bbc84880f74a32a2c5e3fb5d6c2d73de` (parent `a644622b`) | **`70e54fc4a865d0ba850f2083df0c33d06a597aaa`** |

**The public testing-7** (`Dkijas/simutrans-true32-renderer`, commit `ae28a613cef07b77235fced2ab54439f02dcb1d4`) carries its source
under `source/xr32-r12377/`, and that tree is `562448a2…`, **identical** to `3f1b511`. The lab base is the published base.

## Reconstruction (fresh clone at `3f1b511`)

| Method | Result |
| --- | --- |
| the three certified patches (`patch/01-A4.diff`, `02-A12-P1.diff`, `03-TCC-R.diff`) with `git apply`, in order (no fuzz possible) | trees `509c21ec` → `9e64d11f` → **`70e54fc4` = the candidate** |
| `git cherry-pick` of the three commits, in order | no conflict; **`70e54fc4`** |
| control: the same tree check on the candidate's parent | differs, as it must |

## Scope of 3f1b511 → 3200e8e

| Check | Result |
| --- | --- |
| commits | exactly 3, separate: `b03d729`, `a644622`, `3200e8e` |
| files | `src/simutrans/display/stlab_xr32_hook.cci` (+162 −10), `src/simutrans/display/stlab_xr32_zoom.cci` (+8 −8); **nothing else** |
| pak format, makeobj (`descriptor/`, `dataobj/`, `makeobj/`, image writers) | 0 files |
| A10 (marker `l_opq`) | 0 in the candidate; **1 in the A10 record** (control: the check sees it where it exists) |
| LEGACY16-SC (marker `ground_source_colour`) | 0 |
| traditional 16-bit renderer | no file of it changed; the 16-bit builds of the candidate are byte-identical to testing-7's (`WINDOWS-CERTIFICATION.md`) |

**XR32 format unchanged** (no reader or writer touched). **makeobj functionally unchanged:** no makeobj or writer file touched, so
testing-7's makeobj is the same functional version.
