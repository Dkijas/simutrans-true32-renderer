> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# LightTexture alpha crash

**Verdict: the crash exists in clean official r12377, without any XR32 code.** One translucent pixel in `LightTexture` is enough. Not fixed in testing-7, as the brief orders. Newer trunk revisions were not tested.

Evidence: `evidence/alpha-crash.txt`. Tools: `tools/alpha_crash.sh`, `tools/mk_alpha_lab.py`.

## Minimal input

| | |
| --- | --- |
| Source | PAK-NEW's `LightTexture` rows, unchanged |
| Change | **one pixel** of row 37 (sheet position y 585, x 145, colour 121,121,121) with alpha 128 |
| Control | the same file with that pixel at alpha 255 |
| Pak | `PAK128.german 2.4` (traditional), only `ground.LightTexture.pak` replaced, in a hard-link copy |
| makeobj | XR32 makeobj `8a150381` with `STLAB_XR32_WRITE` off; no XR32 node in the file (0 of 81 rows) |

## Result

Clean source: trunk mirror commit `7cc6d452` (`U: official SVN trunk r12377`, `svn export`), 16-bit, SDL3, `tools/build_win.sh` recipe with the build-only fix H. Executable `5ff22e00…`. One run per case, under gdb, in the lane's own run folder.

| Case | Result |
| --- | --- |
| alpha 255 (control) | reaches the first displayed frame |
| alpha 128 | **SIGSEGV in `ground_desc_t::init_ground_textures()`**, before any window content, called from `karte_t::karte_t()` |

The corner mission saw the same crash with its lab pak V2 on testing-6, on `defca456` and on the 16-bit `8feaaf49`.

## Why

`create_textured_tile()` reads the stream of a copy of the light map assuming only opaque runs:

```cpp
sint16 runlen = *dest++;
assert(runlen >= 0);
```

A translucent run carries `TRANSPARENT_RUN` (0x8000) in its length. Read as `sint16`, that is negative. Release builds compile the assert out, the loop does not run, and the walk continues from the wrong word until it leaves the buffer. The function is byte-identical between r12377 and testing-6 (`evidence/alpha-crash.txt`: 0 differing lines).

This is a reading of the code that fits the crash. I did not step through it instruction by instruction. The debugger gives the function, not the source line: this is a release build.

## Limits

- Only r12377. I did not test the current trunk and do not claim anything about it.
- The executable is my build of the official source, not a binary published by the project.
- The pak was made with the XR32 makeobj with its writer off. Official r12377 makeobj (`66dc2815…`, built from the same commit) gives a **byte-identical file**, for both A0 and A1: `evidence/alpha-crash-makeobj.txt`.
- Only alpha 128 in the light map was run on r12377. Alpha 250 and 254 (lab pak LB) load on testing-6 and on the candidate: `INDEPENDENT-ORACLE.md`. Alpha 128 in the **climate texture** (lab pak TA) does not crash: 65 tiles refused, the same bytes on testing-6 and on the candidate, SDL3 and SDL2 (`evidence/TA-translucent-texture.txt`).

## Safe way to reproduce

Use a copy of the pak, never the original. Run the build under a debugger or expect the process to die at load. There is nothing to clean up: the crash happens before a world exists, and no file is written. One run is enough; repeating it adds nothing.

## Recommendation

Report it apart from XR32: the input is a pakset mistake, but a crash at load with no message is a poor way to learn that. Whether the fix is a clear error in makeobj or at load is the maintainers' decision.
