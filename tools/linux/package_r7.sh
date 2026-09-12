#!/bin/bash
# Package revised-7 for Linux. Built on Ubuntu, bzip2 linked statically.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-fedora-01
W=$HOME/stlab/r7
NAME=simutrans-rgba32-revised7-r12261-linux-x86_64-sdl2
P=$W/pkg/$NAME
rm -rf "$W/pkg"; mkdir -p "$P"

cp -r "$W/tree/simutrans/." "$P/"
cp "$W/b32/simutrans/simutrans" "$P/simutrans-rgba32"
cp "$W/b16/simutrans/simutrans" "$P/simutrans-16bit"
chmod 755 "$P/simutrans-rgba32" "$P/simutrans-16bit"
cp "$W/tree/LICENSE.txt" "$P/LICENSE.txt"
cp "$WIN/patches/"*.diff "$P/" 2>/dev/null || true
cp "$WIN/pkg/README-TESTERS-LINUX-R7.txt" "$P/README-TESTERS.txt" 2>/dev/null || true
rm -f "$P/revision.h"
( cd "$P" && { echo "# SHA256 of the parts that matter in this package"; sha256sum simutrans-rgba32 simutrans-16bit *.diff 2>/dev/null; } > SHA256SUMS.txt )
( cd "$W/pkg" && tar --sort=name --owner=0 --group=0 --numeric-owner -czf "$NAME.tar.gz" "$NAME" )
mkdir -p "$WIN/pkg"
cp "$W/pkg/$NAME.tar.gz" "$WIN/pkg/"
echo "=== package"
echo "  files: $(find "$P" -type f | wc -l)"
ls -l "$W/pkg/$NAME.tar.gz" | sed 's/^/  /'
sha256sum "$W/pkg/$NAME.tar.gz" | sed 's/^/  /'
( cd "$P" && sha256sum simutrans-rgba32 simutrans-16bit | sed 's/^/  /' )
echo "  exec bits in the archive: $(tar -tvzf "$W/pkg/$NAME.tar.gz" | grep -cE 'rwxr-xr-x.*simutrans-(rgba32|16bit)$') of 2"
