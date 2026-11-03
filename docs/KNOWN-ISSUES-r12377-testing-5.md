# Known issues — XR32 DAY+NIGHT test build, trunk r12377 (testing-5)

testing-5 is testing-4 plus two fixes: translucent company colours (TCC) and the zoom 2/1 coast pixels (A11). No new
graphics features; the XR32 format is unchanged.

- Part A lists what is still open.
- Part B lists what testing-5 changed.
- Part C lists the defects of official trunk r12377 itself.

The numbering of testing-1 to testing-4 is kept.

Every result below was measured on this build's **Windows** executables, with a control that shows the check can fail,
unless it says otherwise. The certification reports are in `reports/xr32-testing-5/`. **The Linux builds of testing-5
are experimental and not certified** (A9).

## A. Still open in this test build

### TCC-R. Zoom colour residual of company colours (inherited)

When a company-colour key and ordinary **translucent** pixels fall into the same zoom sample, the drawn colour can differ
from an ideal premultiplied mix:
- about 2 levels on average and up to 3.8 with a translucent key;
- up to about 6 with an opaque key, **exactly as in testing-4**.

This was measured on testing-5 and is unchanged from the certified fix. Glass against an opaque frame and keys alone
match the ideal.

### A12. Other generated ground pictures and the beach outline stay 16-bit

Unchanged from testing-4:
- the ground pictures the game makes itself (climate transitions, snow on slopes) are made from 16-bit words;
- the beach outline (`ShoreTrans`) blends the coast with 5-bit weights.

### T1. After a live theme change two small toolbar elements can keep their old picture (trunk behaviour)

Unchanged. The official 16-bit renderer of r12377 leaves exactly the same pixels stale.

### A7. Night pictures cost memory

Unchanged (by design): a second full-size 32-bit picture per object that has one. testing-5's peak memory against
testing-4: no measurable difference.

### A8. The pak syntax and the XR32 format are prototypes

Unchanged (by design). testing-5's makeobj is new (it writes the company-colour meaning of translucent special-colour
pixels), but the format of the XR32 data does not change: testing-4 reads testing-5 paks, and testing-5 reads
testing-4 paks.

### A9. Builds and platforms

- **Windows x64, SDL2 and SDL3:** certified with limitations. AddressSanitizer ran on an MSVC **SDL2** build only
  (there is no SDL3 in the MSVC dependency set used); MinGW has no AddressSanitizer on Windows. Tested on one display.
- **Linux x86_64, SDL2 and SDL3: experimental, NOT certified.** Built from the same testing-5 source with the release
  recipe, which rebuilds testing-4's published Linux executables byte for byte. Only basic checks were run (build,
  architecture, dynamic libraries, a short start, play and clean exit under WSL 2). The Linux graphics certification
  of the earlier builds was not repeated.
- **Not tested:** macOS, Android, ARM, a native Linux desktop.

### A10. A zoom change costs about 1 ms more

Unchanged. testing-5's zoom changes, including 2/1, cost no measurable difference against testing-4.

### N2, N3, N4

Unchanged. Open.

### D1. Microsoft Defender flagged testing-4's Windows SDL3 executable

- **What:** after testing-4 was published, Microsoft Defender's machine-learning detection
  (`Trojan:Win32/Bearfoos.A!ml`) flagged `simutrans-rgba32.exe` of testing-4's **Windows SDL3** package (SHA-256
  `43420d63…`), with definitions 1.463.101.0, and quarantined copies of it during the testing-5 tests.
- **With definitions 1.463.102.0** (2026-11-03) the same file, extracted from testing-4's published zip, is **no longer
  detected** (on-demand scan and real-time protection). Every other file of testing-4 scanned clean.
- **What is known about the file:** it is rebuilt **byte for byte** from testing-4's published source by the build recipe
  of these releases.
- **testing-5:** every asset and every executable inside them scanned clean with definitions 1.463.102.0.
- testing-4's release is left unchanged.

## B. Changed in testing-5

| # | What | testing-5 | Measured (Windows SDL2 and SDL3) |
| --- | --- | --- | --- |
| TCC | Translucent special-colour pixels (company colours, lights) lost their meaning: glass showed the authoring key colour for every company, translucent lights stayed dark at night | **Fixed** in makeobj and the renderer; 8-bit alpha kept; zoom weights each sample by its alpha; ordinary RGB is never recoloured; existing paks draw as before | a real station's glass recolours per company exactly as in the certified fix (X = 9.90, testing-4: 0.47); a test pak with every alpha 0–255 matches its model at 1:1, 9/8, 2/1, 3/4 and 1/2; night darkening applied once; translucent lights glow at night |
| A11 | At zoom 2/1, single sand-coloured pixels in the water next to sloped coasts | **Fixed**: the 32-bit coast water gets the classic 2/1 outline of the 16-bit beach outline it is blended with | 7 → 0 gaps on the 93-coast test map, also after repeated zoom changes; the coast water's 2/1 outline equals an independent model of the classic rule (0 pixels); colours unchanged; every other zoom and picture byte-identical to testing-4 |

testing-4's results were measured again on testing-5's Windows executables with the same tests:
- coast and A4 cache gates;
- screen updates, every frame;
- the regression battery and the automated tests (309 / 309);
- clean shutdown;
- compiler warnings.

They gave the same results.

## C. Official trunk r12377 itself

Unchanged from testing-1 to testing-4 (same trunk): B1–B7 of `docs/KNOWN-ISSUES-r12377.md`, plus T1 above. These are
not caused by XR32.
