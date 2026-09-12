#!/bin/bash
# The controlled comparison the mission asks for: same archive, same conditions,
# separate user directories, 32-bit against 16-bit, with and without a pakset.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-blackwindow-01
A=$HOME/stlab/rgba32-linux/pkg/simutrans-rgba32-revised4-r12261-linux-x86_64-sdl2.tar.gz

echo "=== published archive identity"
sha256sum "$A"
echo "  expected de1b768e018f85224a174806e9eac5317b0e5069388fd85e032d476092e14ecb"
echo

bash "$WIN/tools/repro.sh" 32-nopak simutrans-rgba32 nopak
bash "$WIN/tools/repro.sh" 16-nopak simutrans-16bit  nopak
bash "$WIN/tools/repro.sh" 32-pak   simutrans-rgba32 pak
bash "$WIN/tools/repro.sh" 16-pak   simutrans-16bit  pak
echo DONE
