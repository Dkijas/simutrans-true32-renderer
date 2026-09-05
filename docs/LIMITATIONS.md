# Limitations

## Linux runtime: ENVIRONMENT_BLOCKED, not certified

In the WSL Linux environment used for certification (Ubuntu, g++ 15.2,
WSLg display):

- FreeType does not load the required BDF fonts shipped with Simutrans
  (`font/cyr.bdf` cannot be loaded; pixel size 11 cannot be set for
  `Prop-Latin1.bdf`)
- Simutrans then finds no glyph for its test character and opens the
  modal font-selection dialogue in `simu_main` **before** entering the
  interactive game loop; the dialogue waits for user input

Affected in that environment, identically: the 16-bit runtime, the 32-bit
runtime and the AddressSanitizer runtime (world loaded, loop never
entered, 0 sanitizer reports before the timeout).

Therefore:

    Linux runtime     = ENVIRONMENT_BLOCKED   (not FAIL, not PASS)
    ASan runtime      = ENVIRONMENT_BLOCKED

Measured Linux status:

    SDL2-16 compile   PASS  (rc 0, 27 warnings, none new)
    SDL2-32 compile   PASS  (rc 0, 30 warnings)
    SDL3-32 compile   PASS  (rc 0, 30 warnings)

This is not demonstrated to be TRUE32-specific, and it is not
extrapolated to bare-metal Linux. Linux runtime is simply **not
certified** by this package, either way. A Linux reviewer with a working
FreeType/BDF setup would close this gap by running the quick test path
in TESTING.md.

## Build systems: Makefile only

The GNU Makefile selects the renderer generically
(`src/simutrans/display/simgraph$(COLOUR_DEPTH).cc`), so
`make COLOUR_DEPTH=32` works unchanged. The CMake project and the Visual
Studio project files list `simgraph16.cc` explicitly and were not adapted
in the certified candidate; a 32-bit CMake or MSVC build needs
`simgraph32.cc` added to those lists. Deliberately left out of the frozen
candidate to keep it identical to what was certified; a one-line follow-up
for integration.

## Line endings in the frozen files

The certified files are stored byte-exact under `source/candidate/` and
carry the line endings they had in the Windows laboratory: most are CRLF
(as a Windows SVN checkout is), three of the new headers are LF, and
`simgraph16.cc` has mixed endings from laboratory editing. The review
patch is LF-canonical and applies on both LF and CRLF checkouts (verified
with `patch -p1` and `svn patch --strip 1`); after normalisation the
content is identical (18/18 SHA256). On integration, SVN's
`svn:eol-style native` would normalise the files - a maintainer step, not
done here so that the certified identity stays verifiable.

## Not measured

- Load time at 32 bits (initial image recode), observed about 3x longer
  in the first certification, not re-measured since.
- Sanitizer coverage of the renderer at runtime (MinGW ships no libasan;
  Linux blocked as above).
- Multi-monitor behaviour (one display in the laboratory).
- Bare-metal Linux, macOS, and any GPU-accelerated presentation.

## Memory

About 1.9x the 16-bit working set on pak128 (about 920-960 MB vs
490-520 MB). A property of the design (per-pixel caches twice as wide),
stable and without leak in the measured runs; worth stating to users of
small machines.

## One unclassified observation

During the remediation cycle the author once saw cut image fragments over
water on the welcome screen of a laboratory build. It did not reproduce
in 24 external captures of that screen (before and after the final fix)
nor in any of the hundreds of frames of the two certifications, and is
recorded as NOT_REPRODUCED, not as a defect.
