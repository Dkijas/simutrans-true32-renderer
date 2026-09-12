#!/bin/bash
# Which font does the PUBLISHED binary actually load on Ubuntu?
set -u
W=$HOME/stlab/fontprobe
N=simutrans-rgba32-revised5-r12261-linux-x86_64-sdl2
rm -rf "$W"; mkdir -p "$W"
tar -xzf /mnt/c/Users/Victor/dev/_stlab-rgba32-fedora-01/pkg/$N.tar.gz -C "$W"
D=$W/$N
for b in simutrans-rgba32 simutrans-16bit; do
  ( cd "$D" && SDL_VIDEODRIVER=dummy timeout 12 ./$b -singleuser -nosound -nomidi -debug 3 >/dev/null 2>"$W/$b.log" )
  echo "=== $b on Ubuntu"
  grep -iE "load_from_freetype|Cannot load|fontconfig|themes.tab|No fonts" "$W/$b.log" | head -8 | sed 's/^/  /'
done
echo "=== what fontconfig would answer here"
fc-match Sans 2>/dev/null || echo "  fc-match MISSING"
echo "=== glibc on this Ubuntu: $(ldd --version | head -1)"
