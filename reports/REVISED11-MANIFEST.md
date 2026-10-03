# REVISED11-MANIFEST: rgba32-revised-11-r12322-testing-1 (PROPOSED, not published)

## Source

| Item | Identity |
|---|---|
| base | SVN trunk r12322 (r12321 = Release of 125.0); same base as revised-10 |
| revised-10 baseline | tag `rgba32-revised-10-r12322-testing-1` = `59bdc0d5a08ff96d4c2fadb3d8afea396c58e032`; tree manifest `a8a05ed7e0f61d95f14603552657e002bcb395672fa628bef3eb11021b3645b2` (2220 files) |
| revised-11 source tree | `source/rgba32-revised-11-r12322`, 2223 files, manifest `cced544a3ab154e02753c6f603428e403e0dff10d568d6d52524aed9712d060d` |
| source commit (tree) | branch `rgba32/revised-11-r12322` @ `642bc8fe682db2894b74ae833a2273459af7d4e8` (tree commit; a docs-only commit follows) |
| source tarball used for the Linux builds | `git archive` of that commit, sha256 `9bf36e5a597936c0e03c03516c86e1769ecc9af018ed1fab66c47e8574167cbc` |

## Patch stack (apply to clean SVN r12322 in this order, fuzz 0)

| # | Patch | SHA-256 | Origin |
|---|---|---|---|
| 1 | `RGBA32-CANDIDATE-REVISED-9.r12322.diff` | `04cac01b3f91ea405463686817cc16faef6cd1ce07068e21b211709205b193a0` | revised-10 (unchanged) |
| 2 | `MAKEOBJ-XR32-REVISED9.r12322.diff` | `7d4510d05a9e261666910fa174fa25ea04b58cb8e02bd3cd992612eb615ac806` | revised-10 (unchanged) |
| 3 | `MRES-V2-WRITER.r12322.diff` | `ded0b499275f777bbb8dba59e63210e698664249a09d85a6df4969060d5fda21` | revised-10 (unchanged) |
| 4 | `MRES-V2-RUNTIME.r12322.diff` | `fcd090f732a5fe360f113f3bdf737ac287fa8eded3381b32929e610de6f1e06d` | revised-10 (unchanged) |
| 5 | `XR32-INTEGRITY-HARDENING.r12322.diff` | `c3509c4c597a9dafe0514b3c274ca0df3b37c5c0e115e00d313b3d03c681f386` | revised-10 (unchanged) |
| 6 | `XR32-TRANSPARENCY-RENDERER.r12322.diff` | `eee9b916e78156bbb0c615568754f1cb89e0ff67ec5a02cd005db81ef04d836f` | revised-10 (unchanged) |
| 7 | `02-XR32-SEMANTIC-FORMAT.diff` | `a99823cc0596b8c938b87e7c20f64767cd9f933c5d2e48042e18e80355eb7983` | semantic layer, frozen in XR32-SEMANTIC-MASK-PIPELINE-01 |
| 8 | `01-MAKEOBJ-SEMANTIC-INPUT.diff` | `b7ef3e2fa650ea7bdc6d97b24c120a2698d8acb60b148f27dba5866fb731075e` | semantic layer, frozen in XR32-SEMANTIC-MASK-PIPELINE-01 |
| 9 | `03-RENDERER-SEMANTIC-COMPOSITION.diff` | `ea464a5774c252a812256af1febd6e583beebab5371132fbdb0f8218b90fd59a` | semantic layer, frozen in XR32-SEMANTIC-MASK-PIPELINE-01 |
| all | `RGBA32-XR32-SEMANTIC-STACK-COMBINED.r12322.diff` | `59ba82a45d7647e8b565f185961926c1e662841ec94a21235e6146a61dd44ac4` | the nine as one diff |

Tool layers (not part of the game build): `04-BLENDER-SEMANTIC-EXPORT.diff` `c323c57d34cd1705f6d2ab840df905506f829a9f7b90577b9ef5abea43f4a9c3` (Kit `0bc64b7`), `05-OBJECT-STUDIO-SEMANTIC-IO.diff` `a0e2c1153bb7e4c78f1102df5a7d3e2f635efcba5971eb39b26f624c0501c815` (Object Studio `020ecac`).

## Binaries (inside the packages)

| Binary | SHA-256 | Toolchain |
|---|---|---|
| win64-sdl2 simutrans-rgba32.exe | `6e2c931c4c9209aba8350363f8b77d9409d6ad3751bb8f636dee24ca94d4a30a` | MSYS2 MinGW-w64 GCC 16.1.0, CMake 4.4.0, SDL2 2.32.10, static |
| win64-sdl2 simutrans-16bit.exe | `c5e80dd8d11d6227af6baff4a27425265bfb971e819111bfa3857331eafd7767` | same |
| win64-sdl3 simutrans-rgba32.exe | `f19744651df83be171e4a84ec461b2edff6030d79d5d376aef81d86b5198c14d` | same, SDL3 3.4.12 |
| win64-sdl3 simutrans-16bit.exe | `55567db7158fd36e85cd5c3eb03af3a2701960eb1187b14061a15f6c4b491eb2` | same |
| linux simutrans-rgba32 | `1beef595a1d44a69d014aa4e9a5ab0f68ad24cab28235c07671fc9cf9f490869` | Debian 13, GCC 14.2.0, CMake 3.31.6, SDL2 2.32.4; bzip2 + miniupnpc static |
| linux simutrans-16bit | `2e08572741e12a68d2dfcd03d9e4a592b1dd881eb39e37c5d85dd9c04fc4ee38` | same |
| makeobj.exe (Windows) | `b1361fd28e6d46a6d7b2d0af2daf759fadfe2173fca96e0b3c055ddb747279dc` | MSYS2 GCC 16.1.0 |
| makeobj (Linux) | `8eb187b05d481752e4d0056c0ffe709a972f54b40387f048e43779c9b676ceb2` | Debian 13 GCC 14.2.0 |

All built Release, `-DSIMUTRANS_MULTI_THREAD=ON -DSIMUTRANS_USE_REVISION=12322`, `-DSIMUTRANS_TRUE32=ON` (rgba32) / `OFF` (16bit). Executables are not claimed to be reproducible bit for bit across machines (not measured); makeobj OUTPUT is (measured: Windows = Linux, byte for byte).

## Assets

| Asset | Bytes | SHA-256 | Arch | Backend | Contents |
|---|---|---|---|---|---|
| `simutrans-rgba32-revised11-r12322-win64-sdl2.zip` | 15643152 | `9b047dd87457db808727db172375411e9fb35ca75640670e9ad080efd7d9e46b` | x86_64 Windows | SDL2 | game 32-bit (XR32) + 16-bit control, data, 9 patches |
| `simutrans-rgba32-revised11-r12322-win64-sdl3.zip` | 17474482 | `f5db15aaa9001e142cfaed358103dd04d2ba249972f86831975efa3b785fe2bb` | x86_64 Windows | SDL3 | game 32-bit (XR32) + 16-bit control, data, 9 patches |
| `simutrans-rgba32-revised11-r12322-linux-x86_64-sdl2.tar.gz` | 9022695 | `eb3bb8c45a829c923cc95a4c326a149c2f49dc911fe776add5afbbca3105e37a` | x86_64 Linux (glibc >= 2.38) | SDL2 | game 32-bit (XR32) + 16-bit control, data, 9 patches |
| `simutrans-rgba32-revised11-r12322-semantic-testkit.zip` | 1303754 | `1256e602ed2dda9c421697aa900eaa02f48c4258867deafd7bb40f98ca74ee4c` | any (makeobj: x86_64 Windows + Linux) | - | test cards pak64/pak128, examples, references, makeobj, docs |
| `simutrans-rgba32-revised11-r12322-patches.zip` | 256045 | `f8ff8b87ef574a5c430813a1a2c2dc42b8a07354a916a9a33894fc72d5256a47` | - | - | 9 engine patches + APPLY-ORDER, combined diff, Kit/OS tool layers, layer MANIFEST |
| `RGBA32-XR32-SEMANTIC-STACK-COMBINED.r12322.diff` | 435036 | `59ba82a45d7647e8b565f185961926c1e662841ec94a21235e6146a61dd44ac4` | - | - | SVN r12322 -> revised-11 source, one diff |

## Test kit paks (inside the test kit; Windows and Linux makeobj give these exact bytes)

```
c311b072070633e49f7dc191e10f9461ad0dd5c5074d6e3c9c0a8ff666075002 *cards128-legacy.pak
1ab8e22f0f4a322cd34b2892d7ec54f8f12603aecdf98c352c55899a5d071315 *cards128-nomask.pak
0f15015213e15b917f24d1a73fc34eaede94608ddbc2ad33a6bbb9396333f8ad *cards128-semantic-mres.pak
b202b9bb4991206e07b8498a654026db2a8d055d89c692bd8a0ce27a14de651e *cards128-semantic.pak
5c4a21eb0f5046db54043cc256074d4ba8dcc4977d15859a7279fab46e4a15d2 *cards256-legacy.pak
4a82e43c176676a678f4e88dcfe8d54b4480541b67726d33b953a93cff48fc32 *cards256-nomask.pak
5a453ad238ebea0509872ea19a4aa2f15bdcf7fde83e4c1ca209c3aacdd6d509 *cards256-semantic.pak
b3429aec8aae06080c529a70a18d9b1e65349089ccccbe3b7b1d520e8abff8fb *cards64-legacy.pak
7532eee2cbc44301cd915763610b3be91cdc44707070be5f35a170d9380937ec *cards64-nomask.pak
7b15edd60eadee38ac69adf0d726a5c4ac19c7fd1857b0848c8bd25a5da04a9c *cards64-semantic-mres.pak
49cd1ce18da957872d7801579722a9292e7e285b9f236bc4c7a63567b5c4f7c8 *cards64-semantic.pak
767ac5773506eeff59a4047b3ec6eae65a8333e7a2a0c53e0258d20a986ac586 *example-blender.pak
9eacc52119806e1ae733bd44763912b4b9f3b4b50fcdd95995655497fa82cdb9 *example-railcar.pak
```
