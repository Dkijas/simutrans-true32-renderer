**Experimental testing build. Not integrated into Simutrans Standard, not an official Simutrans release, not Simutrans 125.0 or 126, not a stable version, and not trunk-ready.**

## What revised-11 is

revised-10 (the RGBA32 32-bit software renderer and the XR32 asset pipeline, on **Simutrans SVN trunk r12322**, the trunk right after the 125.0 release) **plus semantic masks**: a way for the artwork to say which pixels are company colour or lights, instead of leaving makeobj to guess.

Same base as revised-10, on purpose: everything that differs from revised-10 is the semantic-mask work, so you can compare the two builds directly.

As before: **every existing pakset and addon keeps working exactly as before**; the 16-bit renderer is still built and shipped (`simutrans-16bit`); XR32 is **off unless you switch it on** with `SIMUTRANS_XR32=1`. This is a software renderer, not GPU acceleration.

## What is new since revised-10: semantic masks

In 32-bit art, the edge of a company-colour patch or of a lamp is a **blend** with the paint next to it. A blend is not a special colour any more, so the game cannot recolour it. revised-10 drew those edge pixels in their stored colour (thin default-blue rims, lamp edges that stay day-coloured at night). Its makeobj tried to recover them from the final picture, and our measurements showed that guess was too often wrong (about one edge pixel in three right for company colour).

revised-11 does not guess. **The artwork says it explicitly**, the way people in the forum suggested: keep the special-colour information separate from the normal antialiased render.

- Next to a sheet `X.png`, one grey image per special colour: `X.sem.<name>.png` (white = fully that colour, grey = partly, black = not at all). Names: `pc1-0`…`pc1-7`, `pc2-0`…`pc2-7`, `window-dark`, `lamp-yellow`, … (all 31 special colours).
- One line in the object's `.dat`: `semantic_masks=1`.
- **makeobj** checks the masks (size, greyscale, at most 4 meanings per pixel, coverage not above full, the picture agrees with the masks) and stores them in a new XR32 node version (v2). It never infers meaning from colours.
- **The game** mixes the company's colour or the night light into every edge pixel in **linear light**, the way the renderer that drew the art mixed them. One pixel can carry up to four meanings (for example two lamps meeting).
- With masks, the masks decide: a lit grey that happens to come out exactly `FFFF53` no longer turns into a lamp (makeobj warns about it).

Ways to make the masks: by hand in any 2D editor (one layer per special colour), from Blender (a "semantic pass" in the Simutrans Blender Kit, prototype patch), and Object Studio can check them and write the `.dat` line (prototype, no editor UI yet). No 3D tool is required.

**Nothing changes for art without masks.** Legacy paks and XR32 paks made without masks draw exactly as in revised-10. Older builds (including revised-10) load semantic paks and simply draw the ordinary image.

## Downloads

| file | what |
|---|---|
| `simutrans-rgba32-revised11-r12322-win64-sdl2.zip` | Windows x86_64, SDL2: `simutrans-rgba32.exe` (32-bit, XR32 + semantic masks) + `simutrans-16bit.exe` (legacy / control) |
| `simutrans-rgba32-revised11-r12322-win64-sdl3.zip` | the same with SDL3 |
| `simutrans-rgba32-revised11-r12322-linux-x86_64-sdl2.tar.gz` | Linux x86_64, SDL2 (glibc 2.38+) |
| `simutrans-rgba32-revised11-r12322-semantic-testkit.zip` | **start here**: test cards for pak64 and pak128 (with / without masks / legacy / 2x), a minimal 2D example, a minimal Blender example, reference images, makeobj for Windows and Linux, quick start and test procedure |
| `simutrans-rgba32-revised11-r12322-patches.zip` | the nine engine patches and their order, the combined patch, the Blender Kit and Object Studio patches |
| `RGBA32-XR32-SEMANTIC-STACK-COMBINED.r12322.diff` | the whole engine change as one patch against r12322 |
| `REVISED11-MANIFEST.md` | exact identities of everything above |
| `SHA256SUMS.txt` | hashes of every asset |

No pakset and no savegame are included. Test in a separate folder, use copies of your savegames.

## How it was checked

The source is revised-10's published tree plus three patch layers, each frozen and hashed before this release was prepared. Composed twice, with different tools, to the same bytes: 2223 files, tree manifest `cced544a3ab154e0…`. Clean SVN r12322 plus the nine patches gives that tree exactly.

| Area | Result |
|---|---|
| Builds | Windows SDL2 32/16 and SDL3 32/16 (MSYS2 GCC 16.1); Linux SDL2 32/16 (Debian 13, GCC 14.2): 0 errors, **warning sets identical to revised-10** |
| makeobj, cross-platform | Windows and Linux write byte-identical paks: 13 of 13 test-kit paks and 3 of 3 reference paks (an independent MSVC build agreed during development) |
| makeobj, old inputs | everything without masks byte-identical to revised-10: **23 of 23** reference paks |
| Engine vs reference model | screen cache bit-identical, 6 companies × 3 night levels: real train (S453) **144/144**, test cards pak128 **144/144** and pak64 **144/144**, both examples **144/144**; zoom (9 scales) **72/72**; drawing (3 passes × 7 backgrounds × 10 zooms) **1680/1680**; 2x MRES **16/16**; Windows and Linux identical |
| Against the exact render | real train: edge pixels at most **5 of 255** off (average **0.43**), where legacy reaches 164 and revised-10's XR32 162; test cards at most 2 |
| Compatibility | revised-10 on semantic paks draws the legacy image (144/144 identical); revised-11 on revised-10's XR32 paks and test kit identical to revised-10 |
| Safety | malformed semantic data rejected cleanly (55/55 cases); fuzzing (earlier: ~466 million and ~25.6 million inputs; this build: ~100 + ~7 million); ASan/UBSan/LSan on the full game: no finding attributable to this work |
| Packages | extracted fresh: automated test scenario **310/310** for every executable on Windows (SDL2, SDL3) and Linux; a corrupted semantic pak is rejected and the game runs on |

## Known limitations

- **Testing only.** Not proposed for trunk yet; the format (XR32 v2) may still change before that.
- **8-bit masks.** Where the game substitutes another company's colour on a half-covered edge pixel, it can be a few levels off (at most 5 of 255 on the real train).
- **Zoom-out averaging.** The ordinary picture is still averaged as in revised-10. Linear treatment applies at 100 %, when zooming in and with 2x pictures.
- **REVISED11-PERF-01.** Rebuilding the zoomed cache of an XR32 image is about **10–12 % slower than revised-10, even without masks** (cards, 1/2 and 2/1 zoom; same compiler). This happens once per zoom change, not per frame. Per-frame drawing is unchanged. To be fixed separately.
- **Semantic objects cost more to prepare**, in proportion to their semantic pixels (≈ 21 ns per masked pixel per cache rebuild), and about 9 % more memory than the same art without masks.
- **Tools are prototypes.** The Blender Kit semantic export is a prototype patch, not in the published Kit. Object Studio reads and checks masks but has no editor UI. The full **night image** (a painted night picture per object) is not part of this release; it may be a later, separate extension.
- **Not tested:** macOS, Android, MSVC game builds, the Linux map-edge visual test. They are not claimed to work.
- **Inherited from revised-10 / trunk, unchanged:** see the revised-10 notes.

## Reporting

Issues: https://github.com/Dkijas/simutrans-true32-renderer/issues. The test kit's `TEST-PROCEDURE.txt` lists the report categories (visual, makeobj, authoring, legacy regression, zoom/MRES, performance, platform, documentation). Please give the category, this tag, your OS and package, the pakset, whether `SIMUTRANS_XR32=1` was set, zoom and night level, a **PNG** screenshot and steps from a fresh start.

revised-10 and all earlier releases and their downloads are unchanged.
