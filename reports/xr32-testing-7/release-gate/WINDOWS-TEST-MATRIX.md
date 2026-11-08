> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# Windows test matrix

Windows 11 x64, one 3440×1440 display, MSYS2 GCC 16.1 builds of `3f1b511` made fresh in this lane, **hash-identical** to the certified ones (`SOURCE-VERIFICATION.md`). testing-6: its certified files. **Multi-monitor not validated.** Levels reached: COMPILE, LINK, RUN, visible presentation in real windows.

Two kinds of evidence:

- **This lane's runs, on these binaries:** everything listed below.
- **The previous mission's runs, on the same binaries** (identical hashes): cited as such, never as new. They are in `<testing-7-fix-lab>\deliver`.

## Real windows (this lane)

| Test | SDL3 | SDL2 | Evidence |
| --- | --- | --- | --- |
| **Scroll**: town (lights at night) and corner scene with keys, 6 view positions + light level 4, testing-6 and candidate | 32 captures: display = framebuffer (only the `CopyFromScreen` 13,11,12 quirk, 7 px) | 32 captures: the same | `windows2.txt` |
| Town, testing-6 against candidate, day and night, every position | 0 px | 0 px | `windows2.txt` |
| Corners with keys, testing-6 against candidate | differs everywhere: the fix acting (control) | the same | `windows2.txt` |
| **TCC**: pak192.comic, TCC fixture and translucent company-colour stations, TCC scenario, day and night | testing-6 = candidate, 0 px | 0 px | `tcc-window.txt` |
| TCC control: day against night | 1.67 M px differ | the same | `tcc-window.txt` |
| **Zoom by keyboard**, focused window, posted characters and OS-level `SendInput` | exact round trips 1/1 ↔ 2/1 side and 1/1 ↔ 1/2 side, testing-6 = candidate | the same (posted characters) | `SDL3-ZOOM-INVESTIGATION.md` |

## Real windows (previous mission, same binaries)

320 steps over 40 runs:
- 10 scenes: corners with and without keys, PAK-OLD and PAK-NEW, the pyramid, snow, the coast with and without keys, a traditional pak;
- zooms 1/1, 2/1, 9/8, 3/4, 1/2 and back to 1/1;
- light levels 2 and 4.

Display = framebuffer in all, apart from the capture quirk; 40 of 40 round trips identical.

## Each tile drawn alone (dummy video, this lane)

| Check | SDL3 | SDL2 |
| --- | --- | --- |
| 15 lab paks + PAK-NEW, 520 tiles each, against the independent oracle | 30 runs PASS | 30 runs PASS |
| Transitions against the model | 0 px off wherever the spec says D | the same |
| Night planes, night, players | PASS | PASS |
| Engine mutants caught | 4 of 4 | |

## Screen updates (dirty regions, lab twins of this lane)

| Phase | Frames verified | Mismatches |
| --- | ---: | ---: |
| Coast world: window events, company colour scheme changes, day/night, coast zoom, coast night (candidate) | 600 | **8**, all in one SDL3 run of `windows`, taken while the machine was in use by another application |
| The same `windows` scenario repeated with the machine idle: testing-6 and candidate, SDL3 and SDL2, n = 3 | 720 | **0** |
| Corner scene with keys: static, scroll, zoom, day/night (candidate) | 480 | 0 |
| CONTROL: a renderer that drops dirty rectangles | 240 | **108**: caught |

The 8 mismatches (at most 29 px) did not come back in 720 frames with an idle machine, on either executable. I cannot say what caused them: the machine was in active use by another program at the time, and that is all that is known. Evidence: `verify-gate.txt`, `verify-windows-repeat.txt`, `logs/activity.csv`.

## Automated suites (this lane)

309 of 309: testing-6 SDL3 and SDL2; candidate SDL3, SDL2 and 16-bit. Control with one injected failing test: **FAIL** `[1/310]`. `suites.txt`.

## Matrix of the brief

| Item | Result | Where |
| --- | --- | --- |
| Initial rendering | PASS | windows2, prev. mission |
| Scroll | PASS | windows2 |
| Zoom 1:1, 2/1, 9/8, 3/4, 1/2 | PASS | prev. mission (gate), keys2 (keyboard) |
| Lighting changes | PASS | windows2 (level 4), prev. mission (2, 4) |
| Visible climate changes, transitions | PASS | prev. mission real windows; tiles and transitions here |
| Snow | PASS | prev. mission real windows; tiles here |
| High-density corners | PASS | windows2, prev. mission |
| Animated water | PASS (paused for the comparison) | prev. mission real windows |
| Night lights | PASS | windows2 (town at level 4), drawall prev. mission |
| Company colours | PASS | TCC real windows, players here, drawall prev. mission |
| SDL3 zoom out by keyboard | not a defect; focus | `SDL3-ZOOM-INVESTIGATION.md` |

## Not tested

- MSVC.
- Another GPU, another display, other DPI scales.
- Windows 10.
- A physical keyboard and mouse.
