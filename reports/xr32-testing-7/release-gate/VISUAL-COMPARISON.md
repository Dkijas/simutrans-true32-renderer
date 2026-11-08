> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# Visual comparison

All pictures here are what the display showed: **real windows**, captured with `CopyFromScreen` of the client area (`tools/pw_capture.ps1`). TCC uses `PrintWindow`, the TCC lab's validated method. Panels: `evidence/visual/`, built by `tools/visual_gate.py`. They contain PAK128.german and pak192.comic art and are **for review only**. Measures: `evidence/visual-metrics.txt`.

## Panels of this lane

| Panel | Sides | What it shows |
| --- | --- | --- |
| `A-corners-keys-scroll-SDL3.png` | testing-6 / testing-7, keyed pak LT, after scrolling | the defect and the fix |
| `B-corners-keys-night-SDL2.png` | the same at light level 4, SDL2 | the fix at night |
| `C-town-night-SDL3.png` | testing-6 / testing-7, coast world town with building lights, light level 4, PAK-NEW | identical: not one pixel apart |
| `D-tcc-night-SDL3.png` | testing-6 / testing-7, pak192.comic TCC scene, night | identical: translucent company colours unchanged |

| Side | Share on the RGB555 grid | Distinct colours | Blue dots |
| --- | ---: | ---: | ---: |
| Corners, keys, **testing-6** | **0.937** | 8,601 | **224,169** |
| Corners, keys, **testing-7** | 0.002 | 41,894 | 0 |
| Corners, keys, night, testing-6 | 0.032 | 3,164 | 151,812 |
| Corners, keys, night, testing-7 | 0.003 | 5,638 | 229 |
| Town, night, testing-6 / testing-7 | 0.003 / 0.003 | 391 / 391 | 0 / 0 |
| TCC, night, testing-6 / testing-7 | 0.155 / 0.155 | 1,714 / 1,714 | 40 / 40 |

The 229 "dots" of testing-7 at night are pixels of the lab texture's painted `0101FF` key. Their red and green values, 1 by day, become 0 when darkened: they are a painted colour, not a defect. The 40 in the TCC scene are part of its art, equal on both sides.

## Updated Pak128.German: identical

On the pak as published on 6 November 2026, testing-6 and testing-7 draw the same pictures:

- **this lane:** the town by day and at night, 0 px, SDL3 and SDL2 (`windows2.txt`); 520 tiles and 570 transitions equal to the RGBA32 model on both executables (`matrix-summary.txt`);
- **previous mission, same binaries:** 4,025 tile and transition cases at 5 zooms, byte-identical; 320 real-window steps, 0 px between the executables on the keyless scenes.

**No difference was introduced to make testing-7 look better.** The improvement visible between the 3 November and 6 November versions of the pak is makie's art (previous mission, panel `A-makie-resource-corners-2x1.png`).

## What I have not judged

Whether the corners of a pak with real keys look right as art. No artist has reviewed the candidate.
