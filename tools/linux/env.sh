#!/bin/bash
# What this Linux box actually offers for a Simutrans SDL2 build.
echo "=== distro"; lsb_release -ds 2>/dev/null; uname -srm
echo "=== toolchain"
for t in gcc g++ cmake make pkg-config patch svn git file ldd; do
  p=$(command -v "$t")
  printf '  %-12s %s\n' "$t" "${p:-MISSING}"
done
gcc --version | head -1 | sed 's/^/  /'
cmake --version 2>/dev/null | head -1 | sed 's/^/  /'
echo "=== dev libraries (pkg-config)"
for p in sdl2 zlib libpng libbz2 freetype2 libzstd fontconfig miniupnpc; do
  v=$(pkg-config --modversion "$p" 2>/dev/null)
  printf '  %-12s %s\n' "$p" "${v:-MISSING}"
done
echo "=== headers"
for h in /usr/include/SDL2/SDL.h /usr/include/zlib.h /usr/include/png.h /usr/include/bzlib.h; do
  if [ -f "$h" ]; then echo "  present $h"; else echo "  MISSING $h"; fi
done
echo "=== display / virtual display"
echo "  DISPLAY=${DISPLAY:-unset}  WAYLAND_DISPLAY=${WAYLAND_DISPLAY:-unset}"
for t in Xvfb xvfb-run; do
  p=$(command -v "$t")
  printf '  %-12s %s\n' "$t" "${p:-MISSING}"
done
