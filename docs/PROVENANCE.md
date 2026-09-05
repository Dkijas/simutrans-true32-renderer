# Provenance

## Official base

    repository:   svn://servers.simutrans.org/simutrans/trunk
    revision:     r12254
    last change:  2026-09-05T00:45:31Z
    HEAD at packaging time (2026-09-05): r12254 (no trunk movement since the base)

This candidate is presented **against r12254**. If the trunk moves, the
patch stays defined against r12254; it has not been and will not be
rebased silently.

## Candidate lineage

The renderer was developed as a series of isolated laboratory cuts, each
with its own report and identity gates (see [../reports/README.md](../reports/README.md)).
The production candidate reviewed here is the output of

    STLAB-SIMUTRANS-RGBA32-RUNTIME-REMEDIATION-01

(three lifecycle defects found by the first full product certification
were closed there) and was then certified without any further production
change by

    STLAB-SIMUTRANS-RGBA32-FULL-RUNTIME-RECERTIFICATION-02   (authoritative)

## The 18 production files

12 modified, 6 new, relative to the trunk root. Nothing else in the tree
differs (no Makefile, CMake or project-file change; the build-generated
`revision.h` is not part of the candidate).

    M  src/simutrans/dataobj/gameinfo.cc
    M  src/simutrans/descriptor/ground_desc.cc
    M  src/simutrans/descriptor/reader/image_reader.cc
    M  src/simutrans/display/simgraph.cc
    M  src/simutrans/display/simgraph.h
    M  src/simutrans/display/simgraph16.cc
    M  src/simutrans/simcolor.h
    M  src/simutrans/simmesg.cc
    M  src/simutrans/sys/simsys.h
    M  src/simutrans/sys/simsys_s2.cc
    M  src/simutrans/sys/simsys_s3.cc
    M  src/simutrans/sys/simsys_w.cc
    A  src/simutrans/display/blit_clip_core.h
    A  src/simutrans/display/blit_core.h
    A  src/simutrans/display/simgraph32.cc
    A  src/simutrans/display/simgraph32.h
    A  src/simutrans/display/simgraph_palette.h
    A  src/simutrans/display/zoom_core.h

## Identities

Two hash lists are provided in [`../evidence/hashes/`](../evidence/hashes/):

- `candidate-18-files.raw.sha256-md5` - SHA256 and MD5 of the exact
  certified bytes (the files carry the line endings they had in the
  Windows laboratory; some are CRLF, some LF, `simgraph16.cc` mixed). These
  are the values the certification reports quote (for example
  `simgraph32.cc` MD5 `504b9409e6...`, `simsys_w.cc` MD5 `115b417fed...`).
  `source/candidate/` holds these bytes unchanged (`.gitattributes`
  disables git's line-ending conversion).
- `candidate-18-files.lf-normalised.sha256` - SHA256 after CR removal.
  This is what a maintainer obtains after `svn checkout -r 12254` on a
  system whose `svn:eol-style native` is LF, plus the patch. Use this
  list to verify an applied tree; line endings are the only difference.

The patches (all LF line endings, paths a/ b/):

    v1  patches/true32-r12254.diff                 - the renderer candidate as certified (Makefile-only build)
        SHA256 1092abe59e177a9eff1da1ad1ea35ff121be95957556d230f8c3be8f7301e3e3
        18 files, 47 hunks, +4424 / -771

    v2  patches/true32-r12254-v2.diff              - the same renderer + the CMake / Visual Studio selection (current)
        SHA256 2254278c071018fbf2d8cbe9be7fa2ae7690b1a0b5068f776a075600561cdbc4
        23 files, 65 hunks, +4484 / -791   (= v1 followed by the build-integration diff, byte for byte)

        patches/build-integration-r12254.diff      - the build-file part alone
        SHA256 b9d99f53b33617e5c20f155643570808e702fad743f6430e636563890f7b466c
        5 files, 18 hunks, +60 / -20: CMakeLists.txt, Simutrans-GDI.vcxproj,
        Simutrans-SDL2.vcxproj, Simutrans-SDL3.vcxproj, Simutrans-GDI.vcxproj.filters

The build files are stored byte-exact under `source/build/`; their hashes
(raw and LF-normalised) are `evidence/hashes/build-files.*`. The 18
renderer files are unchanged since RECERTIFICATION-02 (verified before
and after the build-file work, raw SHA256 18/18). Build-integration
certification: 2026-09-05, MinGW-w64 g++ 16.1.0 / CMake 4.4 (MSYS2),
MSVC 2022 17.14 (v143 14.44, MSBuild 17.14, bundled CMake, vcpkg
x64-windows-static).

## How to obtain the base and apply the candidate

    svn checkout -r 12254 svn://servers.simutrans.org/simutrans/trunk simutrans-r12254
    cd simutrans-r12254
    svn patch --strip 1 /path/to/true32-r12254-v2.diff
    svn status          # expect 17 M + 6 A, no C   (v1: 12 M + 6 A)

Verified on fresh working copies: `svn patch --strip 1` applied v1 and v2
with 0 conflicts on Windows (CRLF working copy) and `patch -p1` applied
them cleanly on an LF-normalised export; in every case the files matched
the certified ones after line-ending normalisation (18/18 renderer files,
5/5 build files).

Then verify:

    # on an LF checkout
    sha256sum -c candidate-18-files.lf-normalised.sha256
    # on a CRLF checkout: strip CR first, e.g.  tr -d '\r' < file | sha256sum

## Auxiliary patch: GDI resize-event delivery (not part of the candidate)

`patches/gdi-resize-event-delivery-r12254.diff` - base r12254, `src/simutrans/sys/simsys_w.cc`
(3 hunks) and `src/simutrans/simevent.cc` (1 hunk), SHA256
`13f283a63235bfb913a231b945f02ea93a71b5f336da135088944bd8dbeeb76b`; applying it to pure r12254
reproduces the certified files byte for byte. The certified `svn diff` of the same change has
SHA256 `b07d33cbaf87e95782d80526d1923f0134299ed7091318a12c8f71bffed58fa0` (localised svn headers,
identical hunk content). Certified by GDI-RESIZE-EVENT-DELIVERY-SVN-CERT-01; not integrated.
The canonical TRUE32 patch stays v2; the two are published separately on purpose.

## Statements

- No SVN integration has occurred. The official trunk is unchanged.
- This repository is for validation and review; it is not an official
  Simutrans repository and not a release.
- The candidate has not been modified since its certification: the
  hashes above are the certified ones.
