#!/bin/bash
# The DIRECT dependencies a tester's system must provide (DT_NEEDED), as opposed
# to everything ldd pulls in transitively through SDL2 and fontconfig.
set -u
W=$HOME/stlab/rgba32-linux
for a in build32 build16; do
  echo "=== $a direct NEEDED"
  objdump -p "$W/$a/simutrans/simutrans" | awk '/NEEDED/ {print "  " $2}'
done
echo "=== the packages Ubuntu 26.04 provides them from"
for so in libSDL2-2.0.so.0 libpng16.so.16 libz.so.1 libbz2.so.1.0 libfontconfig.so.1 libminiupnpc.so.21 libfreetype.so.6 libzstd.so.1; do
  p=$(dpkg -S "$(readlink -f /usr/lib/x86_64-linux-gnu/$so 2>/dev/null)" 2>/dev/null | cut -d: -f1)
  printf '  %-24s %s\n' "$so" "${p:-unknown}"
done
