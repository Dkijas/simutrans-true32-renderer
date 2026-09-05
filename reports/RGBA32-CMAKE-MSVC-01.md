# STLAB-SIMUTRANS-RGBA32-CMAKE-MSVC-01

## Result
VERDICT:     A - CMAKE/MSVC TRUE32 BUILD DEBT CLOSED
CONFIDENCE:  HIGH

CMake (MinGW and the Visual Studio generator) and the hand-maintained
Visual Studio projects now select the renderer from `COLOUR_DEPTH`
exactly as the Makefile does, reject any other value, and build the same
frozen renderer: MSVC 2022 was available and executed both depths through
both MSVC paths. The public review repository is updated with a v2 patch,
the exact build files, evidence and documentation; the 18 renderer files
are untouched.

## Base
SVN:                        r12254 (official base of the candidate)
live HEAD:                  r12254 at mission start (no movement; informational)
TRUE32 candidate identity:  18/18 raw SHA256 identical to the public package
                            hashes and to the RECERTIFICATION-02 lane, verified
                            in the lane tree before the build-file edits and
                            re-verified after them (`diff -rq` of `src/`: 0)

## Build inventory
Makefile selection:      `SOURCES += src/simutrans/display/simgraph$(COLOUR_DEPTH).cc`
                         and `CFLAGS += -DCOLOUR_DEPTH=$(COLOUR_DEPTH)` (Makefile
                         371 / 787); generic, untouched
CMake selection before:  root `CMakeLists.txt` hard-coded
                         `src/simutrans/display/simgraph16.cc` and
                         `COLOUR_DEPTH=16` in each of the three GUI backend
                         branches (sdl2 214/227, sdl3 248/267, gdi 289/291);
                         `none` -> simgraph0.cc, COLOUR_DEPTH=0; no
                         COLOUR_DEPTH variable existed; `cmake/*.cmake`,
                         makeobj/nettool CMakeLists carry only COLOUR_DEPTH=0
Visual Studio model:     independent (hand-maintained `Simutrans-GDI/SDL2/SDL3/
                         Server.vcxproj` + `.filters` + shared
                         `Simutrans-Main.vcxitems`; the official Windows CI
                         uses MSBuild on them; nothing is generated from CMake)
                         CMake can ALSO generate VS projects (Visual Studio
                         generator + vcpkg) - both paths exist
generated / independent: independent (plus CMake-generated as a second path)

hard-coded simgraph16 locations:  CMakeLists.txt x3 (sources) + x3 (defines);
                         Simutrans-GDI.vcxproj (ClCompile + 3 PreprocessorDefinitions);
                         Simutrans-SDL2.vcxproj (same); Simutrans-SDL3.vcxproj (same);
                         Simutrans-GDI.vcxproj.filters (ClCompile). Server keeps
                         simgraph0.cc / COLOUR_DEPTH=0 (out of scope). None in
                         `Simutrans-Main.vcxitems`.

## Implementation
files:    5 - CMakeLists.txt, Simutrans-GDI.vcxproj, Simutrans-SDL2.vcxproj,
          Simutrans-SDL3.vcxproj, Simutrans-GDI.vcxproj.filters
hunks:    18 (LF-canonical diff `build-integration-r12254.diff`)
+/-:      +60 / -20  (CMakeLists +27/-7; each vcxproj +11/-4; filters +1/-1)

CMake:    cache variable `COLOUR_DEPTH` ("16", STRINGS 16;32); for every
          backend but `none`: FATAL_ERROR unless it matches ^(16|32)$,
          `SIMUTRANS_RENDERER_SOURCE = src/simutrans/display/simgraph${COLOUR_DEPTH}.cc`
          (FATAL_ERROR if the file does not exist), used by the sdl2/sdl3/gdi
          `target_sources`; the three `COLOUR_DEPTH=16` definitions become
          `COLOUR_DEPTH=${COLOUR_DEPTH}`; one comment updated. `none` keeps 0.
MSBuild:  each GUI project gets `<COLOUR_DEPTH Condition="'$(COLOUR_DEPTH)'==''">16`,
          `COLOUR_DEPTH=$(COLOUR_DEPTH)` in the three PreprocessorDefinitions,
          `<ClCompile Include="src\simutrans\display\simgraph$(COLOUR_DEPTH).cc" />`,
          and a target `SimutransCheckColourDepth` (BeforeTargets=ClCompile)
          that errors on any value other than 16/32; the GDI filters follow.
          Applied by `apply_build_selection.py` (exact-string edits, count
          assertions, CRLF preserved).

renderer production files modified:
0 REQUIRED  (0 modified: `diff -rq base-tree/src tree/src` empty; hashes 18/18)

## CMake 16
configure:        PASS (MSYS Makefiles generator, MinGW g++ 16.1.0; also
                  with COLOUR_DEPTH not given -> 16 by default)
build:            PASS (rc 0, 362 objects, 35 warnings, 0 errors, 15012474 B exe)
renderer object:  simgraph16.cc.obj present, simgraph32 absent
302/302:          NOT RUN - no such gate exists in this programme; the
                  equivalents were run: the 29-scenario legacy hash is a
                  property of the unchanged `simgraph16.cc` (identical to
                  trunk, RECERTIFICATION-02) and the runtime smoke of the
                  CMake-built GDI-16 executable PASSED (startup, scroll,
                  zoom, night, resize 2/2 clip MATCH, screenshot, save,
                  reload; frames 0.0 % off the RGB565 lattice)
20/20:            the legacy blend-vector gate (`vector_gate.cc`, 20 vectors)
                  PASS / negative control FAIL - it checks the frozen legacy
                  blend formulas, independent of the build system
warnings:         category set identical to the unmodified tree built by
                  CMake (17 missing-field-initializers, 6 undef, 5 multichar,
                  3 unused-function, 1 unused-variable, 1 unused-but-set,
                  1 stringop-truncation, 1 cpp)

## CMake 32
configure:        PASS (gdi and sdl3)
build:            PASS gdi (rc 0, 362 objects, 38 warnings, 15026289 B) and
                  sdl3 (rc 0, 14 warnings)
renderer object:  simgraph32.cc.obj present, simgraph16 absent (both backends)
width guards:     STORED 2 / SCREEN32 4 / SAVED 4 / WIRE 2 / FLAGGED 8, no
                  flag bit inside the colour word, colour mask 0xFFFFFFFF:
                  7/7 PASS (6/6 at 16)
runtime smoke:    PASS - hooked CMake-built GDI-32 and SDL3-32, pak128
                  q25.sve, live, 4 lanes: 18 shots each, road tool ok/ok,
                  3/3 clip MATCH, save + reload ok, 0 FAILED/unknown, frames
                  99.7-99.8 % off the RGB565 lattice; memory 926 -> 936 MB
                  (GDI) / 969 -> 984 MB (SDL3), as in the certification
T1-T4:            PASS, linked against the CMake gdi-32 objects: nc 0xDA18C545,
                  wc 0x19D5EC05, pc 0x0F526995, class+alpha 0x7B4F5C85 = frozen

## MSVC
toolchain available:
YES  (Visual Studio 2022 Community 17.14.38, MSVC v143 14.44, MSBuild 17.14,
      bundled CMake, vcpkg with x64-windows-static ports)

project generation:        CMake Visual Studio 17 2022 generator + vcpkg:
                           configure PASS for 32 and 16 (26-33 s, ports from
                           the binary cache); generated `simutrans.vcxproj`
                           lists simgraph32.cc + COLOUR_DEPTH=32 (resp. 16)
16-bit renderer selection: hand project: simgraph16.obj compiled (default and
                           explicit 16); generated project: simgraph16.cc
32-bit renderer selection: hand project: simgraph32.obj; generated: simgraph32.cc
actual build 16:           hand project x64 Release rc 0, 0 errors, `Simutrans
                           GDI Nightly.exe` 6735872 B (byte-equal in size to
                           the pristine r12254 build); CMake generator rc 0,
                           simutrans.exe 5950976 B
actual build 32:           hand project rc 0, 0 errors, 6745088 B; CMake
                           generator rc 0, 5963776 B. Both 32-bit MSVC
                           executables run pak128 q25 (alive at 16-21 s, world
                           on screen); external window captures 97.7 % of
                           pixels off the RGB565 lattice (control: the MSVC
                           16-bit executable 0.0 % off GDI's 16-bit lattice)

Environment note (not the candidate): the MSBuild pre-build step
`tools/get_revision.bat` writes `src/simutrans/revision.h` as UTF-16 on this
host (PowerShell redirection, Spanish locale); pristine r12254 then fails
to compile identically (C2660 `atol`, `strlen`). The MSBuild runs above used
`/p:PreBuildEventUseInBuild=false` with an ASCII `revision.h`; the CMake
runs on the export used `-DSIMUTRANS_USE_REVISION=12254`.

## Negative control
invalid COLOUR_DEPTH:  24 (also the literal string `$cd` once, by accident)
expected:              configuration / build fails clearly, no silent 16
actual:                CMake (MinGW and VS generator): `CMake Error at
                       CMakeLists.txt:224: COLOUR_DEPTH='24' is not supported:
                       use -DCOLOUR_DEPTH=16 (RGB565) or -DCOLOUR_DEPTH=32
                       (ARGB8888).` rc 1, no build tree; MSBuild:
                       `Simutrans-GDI.vcxproj(205,5): error : COLOUR_DEPTH='24'
                       is not supported: build with /p:COLOUR_DEPTH=16 (RGB565)
                       or /p:COLOUR_DEPTH=32 (ARGB8888).` rc 1 in 1 s, no
                       object compiled

## Makefile control
16:  PASS (rebuilt from the modified tree: rc 0, 51 warnings, sim.exe 10267108 B
     = certified size)
32:  PASS (rc 0, 54 warnings, 10280923 B = certified size; simgraph32.o only)

## Warnings
legacy:          CMake-16 categories = unmodified tree built by CMake (identical)
TRUE32:          CMake-32 = CMake-16 + {unused-parameter (clip_num.h),
                 2 x unused-function (simgraph32.cc)} - the same lines the
                 certified Makefile-32 build already reports
new categories:  NONE caused by the selection (Makefile vs CMake category
                 sets differ by flags each system passes - -Wcast-qual etc.
                 vs -Wundef - equally for the unmodified tree)

## Formats
pak:      unchanged (build files only)
makeobj:  unchanged (its CMakeLists keeps COLOUR_DEPTH=0, untouched)
save:     unchanged
network:  unchanged
MOTD:     unchanged

## Renderer candidate
18 frozen production files:
UNCHANGED  (raw SHA256 18/18 before and after; `source/candidate/` in the
            public repo re-verified)

## Public repo
remote:         https://github.com/Dkijas/simutrans-true32-renderer.git (owner
                Dkijas, repository simutrans-true32-renderer - verified before push)
branch:         main
commits added:  2 (build: CMake/MSVC renderer selection - patches, exact build
                files, hashes; docs: certification of the CMake/MSVC path -
                documentation, evidence, report)
push:
PASS

## Patch versions
v1:  `patches/true32-r12254.diff` - original frozen renderer candidate
     (18 files, 47 hunks, +4424/-771, SHA256 1092abe5...) - kept unchanged

v2:  `patches/true32-r12254-v2.diff` - renderer + build integration
     (23 files, 65 hunks, +4484/-791) = v1 followed by
     `patches/build-integration-r12254.diff` (5 files, 18 hunks, +60/-20,
     SHA256 b9d99f53...). Verified: `patch -p1` on an LF-normalised fresh
     export applies cleanly with 18/18 renderer files + 5/5 build files
     identical (LF); `svn patch --strip 1` on a clean r12254 working copy:
     17 M + 6 A, 0 conflicts.

v2 SHA256:  2254278c071018fbf2d8cbe9be7fa2ae7690b1a0b5068f776a075600561cdbc4

## Documentation
README:        build systems paragraph rewritten (three build systems), patch
               list v1/v2/build-integration, review steps, source/build
BUILDING:      rewritten: Makefile / CMake (MinGW, VS generator, Linux) /
               MSBuild, the certified outcomes, the revision note
LIMITATIONS:   "Makefile only" replaced by the closed status + the two
               environment notes (export revision.h, MSBuild UTF-16 revision
               step); line-ending note extended to the build files
CERTIFICATION: new "Build-system integration" section with the measured facts
PROVENANCE:    v1/v2/build-integration patches with hashes, build-file hashes,
               date and toolchains, updated apply instructions (17 M + 6 A)
also:          CHANGELOG-TRUE32 (build files section), MAINTAINER-REVIEW
               (build-system questions), evidence/README, reports/README,
               this report copied (paths anonymised) as reports/RGBA32-CMAKE-MSVC-01.md

## Public hygiene
result:  PASS - recursive scan of the changed files for the user name and
         C:\Users, /c/Users, /home/, token, password, secret, vscode-webview,
         claude.ai: 0 hits; no build directories, paksets or binaries added;
         the three new screenshots are window captures with label bands

## Debt status
CMake:
CLOSED

Visual Studio:
CLOSED  (hand projects and the CMake-generated projects both built and run
         with MSVC 2022 at 16 and 32)

Makefile:
CERTIFIED

## Forum update draft
(printed in the mission's final message; not posted)

## Safety
- build-system debt only: yes (5 build files; `apply_build_selection.py`)
- frozen renderer unchanged: yes (18/18 SHA256 before and after)
- no SVN write: yes (export/checkout/patch/revert locally only; HEAD r12254)
- no upstream PR: yes
- no official GitHub write: yes
- only Dkijas/simutrans-true32-renderer updated: yes (remote verified)
- no automatic forum post: yes

## Harness notes
- The MSYS Makefiles generator needs `CMAKE_MAKE_PROGRAM` pointing at the
  msys `make`; the Ninja and Visual Studio generators on Windows are wired
  to vcpkg by `cmake/SimutransVcpkgTriplet.cmake` (a Ninja+MinGW configure
  therefore fails without a toolchain file - upstream behaviour).
- MSBuild's `OutDir`/`IntDir` were first set inside the source tree, which
  polluted it (objects, vcpkg_installed, the UTF-16 revision.h); cleaned,
  and the second round used directories outside the tree. The CMake
  `--install` of this tree writes into the build's output directory and
  runs NSIS, which fails here for the unmodified tree too.
- PowerShell: `$r` and `$R` are the same variable; a `-DVAR=$x` token is
  passed literally to native commands unless quoted.
