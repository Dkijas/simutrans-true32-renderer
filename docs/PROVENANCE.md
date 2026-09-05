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

The canonical patch:

    patches/true32-r12254.diff
    SHA256 1092abe59e177a9eff1da1ad1ea35ff121be95957556d230f8c3be8f7301e3e3
    18 files, 47 hunks, +4424 / -771 lines, LF line endings, paths a/ b/

## How to obtain the base and apply the candidate

    svn checkout -r 12254 svn://servers.simutrans.org/simutrans/trunk simutrans-r12254
    cd simutrans-r12254
    svn patch --strip 1 /path/to/true32-r12254.diff
    svn status          # expect 12 M + 6 A, no C

Verified during packaging on a fresh working copy: `svn patch --strip 1`
applied with 0 conflicts on Windows (CRLF working copy) and `patch -p1`
applied cleanly on an LF-normalised export; in both cases the 18 files
matched the certified candidate after line-ending normalisation
(18/18 SHA256).

Then verify:

    # on an LF checkout
    sha256sum -c candidate-18-files.lf-normalised.sha256
    # on a CRLF checkout: strip CR first, e.g.  tr -d '\r' < file | sha256sum

## Statements

- No SVN integration has occurred. The official trunk is unchanged.
- This repository is for validation and review; it is not an official
  Simutrans repository and not a release.
- The candidate has not been modified since its certification: the
  hashes above are the certified ones.
