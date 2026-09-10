#!/bin/bash
# Assemble the Linux tester package and the tarball.
#
# Built entirely on the Linux filesystem so the executable bit and any symlink
# survive into the archive; /mnt/c cannot carry either.
set -eu
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-linux-delivery-01
W=$HOME/stlab/rgba32-linux
NAME=simutrans-rgba32-revised4-r12261-linux-x86_64-sdl2
P=$W/pkg/$NAME

rm -rf "$W/pkg"; mkdir -p "$P"

# the game's own data directory, laid out next to the executables, which is
# where Simutrans looks by default
cp -r "$W/tree/simutrans/." "$P/"
cp "$W/build32/simutrans/simutrans" "$P/simutrans-rgba32"
cp "$W/build16/simutrans/simutrans" "$P/simutrans-16bit"
chmod 755 "$P/simutrans-rgba32" "$P/simutrans-16bit"
cp "$W/tree/LICENSE.txt" "$P/LICENSE.txt"
cp "$WIN/patches/RGBA32-CANDIDATE-REVISED-4.diff" "$P/"
cp "$WIN/pkg/README-TESTERS-LINUX.txt" "$P/"

# generated files must not travel: revision.h and friends are CMake output
rm -f "$P/revision.h"

( cd "$P" && sha256sum simutrans-rgba32 simutrans-16bit RGBA32-CANDIDATE-REVISED-4.diff > SHA256SUMS.txt )

echo "=== package"
ls -l "$P" | sed 's/^/  /'
echo "  files: $(find "$P" -type f | wc -l)  size: $(du -sh "$P" | cut -f1)"

# reproducible-ish tarball: sorted, no owner/group names, preserving mode
( cd "$W/pkg" && tar --sort=name --owner=0 --group=0 --numeric-owner -czf "$NAME.tar.gz" "$NAME" )
cp "$W/pkg/$NAME.tar.gz" "$WIN/pkg/"
echo "=== archive"
ls -l "$W/pkg/$NAME.tar.gz" | sed 's/^/  /'
sha256sum "$W/pkg/$NAME.tar.gz" | sed 's/^/  /'
echo "  executable bits inside the archive:"
tar -tvzf "$W/pkg/$NAME.tar.gz" | grep -E "simutrans-(rgba32|16bit)$" | sed 's/^/    /'
