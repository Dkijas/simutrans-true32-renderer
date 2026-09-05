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

## Build systems

Closed after the renderer certification (CMAKE-MSVC-01): CMake and the
hand-maintained Visual Studio projects now select `simgraph16.cc` or
`simgraph32.cc` from `COLOUR_DEPTH`, exactly as the Makefile does, and
reject any other value. Certified with fresh builds through CMake+MinGW,
CMake+Visual Studio generator (MSVC 2022) and MSBuild on the hand projects
(see CERTIFICATION.md). Two environment notes that are not the renderer's:

- a tree exported without `.svn`/`.git` has no `revision.h`; CMake then
  needs `-DSIMUTRANS_USE_REVISION=<n>` (checkouts do not)
- on this machine the MSBuild pre-build step `tools/get_revision.bat`
  writes `revision.h` as UTF-16 (a PowerShell redirection in a Spanish
  locale), which breaks the compile of pristine r12254 exactly the same
  way; the certification runs disabled that step
  (`/p:PreBuildEventUseInBuild=false`) with an ASCII `revision.h` in place.
  Official CI does not hit this.

The `none` (headless) backend keeps `COLOUR_DEPTH=0`; the Simutrans-Server
project is untouched.

## Line endings in the frozen files

The certified files are stored byte-exact under `source/candidate/` (and
the build files under `source/build/`) and carry the line endings they had
in the Windows laboratory: most are CRLF (as a Windows SVN checkout is),
three of the new headers are LF, and `simgraph16.cc` has mixed endings
from laboratory editing. The review patches are LF-canonical and apply on
both LF and CRLF checkouts (verified with `patch -p1` and
`svn patch --strip 1`); after normalisation the content is identical
(18/18 renderer files, 5/5 build files). On integration, SVN's
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
