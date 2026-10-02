**Experimental testing build. Not integrated into Simutrans Standard, not an official Simutrans release, not Simutrans 125.0 or 126, not a stable version — and not trunk-ready.**

## What revised-10 is

The RGBA32 32-bit software renderer and the **XR32** asset pipeline, rebased from the r12261-era revised-9 onto **Simutrans SVN trunk r12322** — the trunk right after the **Simutrans 125.0 release (r12321)**. So it is now the 125.0 code generation plus this work.

The aim has not changed: **keep every existing 16-bit pakset and addon working as before, and add 32-bit art on top.** Nothing is removed. Old paks load and draw as they always did, the ordinary 16-bit renderer is still built and shipped (`simutrans-16bit`), and XR32 is **off unless you switch it on** with `SIMUTRANS_XR32=1`.

This is a software renderer. It is not GPU acceleration.

## What is new since revised-9

- **Rebased onto r12322.** The revised-9 renderer (which already contained FIX-6, the map-edge fix) applies to 125.0-era trunk with one small build-file conflict resolved; every file only this work touches is byte-identical to the certified revised-9 / closure versions.
- **XR32 assets.** `makeobj` can write a *dual* pak: the ordinary 16-bit image plus a full 32-bit RGBA picture of it (`STLAB_XR32_WRITE=1`). The game draws the 32-bit picture when `SIMUTRANS_XR32=1`, the ordinary one otherwise.
- **MRES (2x pictures).** `makeobj mres` combines a 1x and a 2x pak; the game uses the 2x picture when you zoom in.
- **Integrity.** XR32 nodes carry a CRC-32 and are bounds-checked; a damaged node is rejected and the ordinary image is drawn instead (logged as `candidate node rejected, using the legacy IMG`).
- **Partial transparency fixed (R1).** XR32 pixels with alpha 8…254 could be drawn fully opaque on some drawing paths. They are now blended correctly at 1x and 2x, at every zoom, over every background and on every drawing path the oracle covers.

## Downloads

| file | what |
|---|---|
| `simutrans-rgba32-revised10-r12322-win64-sdl2.zip` | Windows x86_64, SDL2: `simutrans-rgba32.exe` (32-bit, XR32-capable) + `simutrans-16bit.exe` (legacy / control) |
| `simutrans-rgba32-revised10-r12322-win64-sdl3.zip` | the same with SDL3 |
| `simutrans-rgba32-revised10-r12322-linux-x86_64-sdl2.tar.gz` | Linux x86_64, SDL2 (glibc 2.38+) |
| `simutrans-rgba32-revised10-r12322-xr32-testkit.zip` | a pak64 test tree in legacy / XR32 / XR32+2x form, its sources, and a Windows `makeobj` with XR32 and MRES |
| `simutrans-rgba32-revised10-r12322-patches.zip` | the six source patches, their order, the combined patch, SHA-256 |
| `RGBA32-XR32-STACK-COMBINED.r12322.diff` | the whole change as one patch against r12322 |
| `REVISED10-MANIFEST.md` | exact identities of everything above |
| `SHA256SUMS.txt` | hashes of every asset |

No pakset and no savegame are included. Please test in a separate folder, not over your normal Simutrans, and use copies of your savegames.

## How it was checked

All binaries were built from the tagged commit's tree (`git archive`), which is byte-identical to clean r12322 + the six patches (2220 files, manifest `a8a05ed7e0f61d95…`).

- **Builds:** Windows SDL2 32/16, SDL3 32/16 (MSYS2 MinGW-w64 GCC 16.1), Linux SDL2 32/16 (Debian 13, GCC 14.2): 0 errors, and the warning sets are **identical to clean r12322**.
- **makeobj:** the revised-10 makeobj writes **23 of 23** reference paks byte-identical to the certified ones (9 end-to-end, 8 transparency fixture, 6 vehicle).
- **Runtime oracles** (instrumented build of the same tree, pixel-exact):
  - transparency, 1x fixture: **761,208 / 761,208** framebuffer pixels exact; with 2x (MRES): **3,812,193 / 3,812,193**; the same test without R1 fails (475,095 and 2,470,953 exact), as it must;
  - XR32 end to end: **749,232** pixels exact (two lanes), 390,243 (legacy lane);
  - MRES selection matrix unchanged, 2x XR32: **44,800 / 44,800**;
  - real vehicle: **728,028 / 728,028** (6 companies × nights) and 2x **64,468 / 64,468**; company-colour edge halo unchanged;
  - blit (cache = plane alpha, framebuffer = cache composited): PASS, 788,928 and 7,095,970 pixels; zoom enlargement: **0 of 405,477** cache words differ.
- **The packages as published**, extracted, on Windows (native, no window) and Linux: start, pak64 + the test kit's XR32+MRES pak loaded with `SIMUTRANS_XR32=1`, the automated test scenario **310 / 310** for both executables of all three packages; the XR32 nodes accepted with 0 warnings, and a copy with one corrupted byte rejected and drawn from the legacy image.

**Not re-tested on this base:** the Linux map-edge visual test, MSVC builds, Android. They are not claimed to work.

## Known limitations

- **Memory.** An XR32 image is kept as an uncompressed RGBA plane next to its legacy image: measured extra resident memory **1.96×–5.16×** the legacy image data depending on the asset class (ground … buildings); a 2x MRES picture adds about 4× the 1x XR32 cost. On disk a dual pak is 1.7×–2.3× the legacy pak.
- **Compression.** Planes are deflated. zstd was measured and saved only **2.4–6.6 %** on four of five asset classes (24.5 % on large ground textures) — not enough to make zstd a requirement for makeobj and the game, so deflate stays.
- **Special colours on antialiased edges are NOT solved.** Company colours and lights work on pixels that are exactly a special colour. The antialiased edges between a special colour and ordinary art (company-colour edges, light edges, several lights overlapping one pixel) are drawn with their stored colour; doing better needs explicit metadata from the artist / exporter, which does not exist yet. That is future work.
- **Inherited from trunk, unchanged:** the left shift of a negative value in the clipping code, an unlocked read of recode flags in the main viewport, and a one-element overread when a GROUND image has a semi-transparent pixel.

## Reporting

Issues: https://github.com/Dkijas/simutrans-true32-renderer/issues ("Bug / regression"). Please include this tag, your OS, the pakset, whether `SIMUTRANS_XR32=1` was set, steps from a fresh start, the zoom level, a screenshot and the `-debug 3` log.

revised-9 and its downloads are unchanged.
