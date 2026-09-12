#!/bin/bash
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-fedora-01
N=simutrans-rgba32-revised5-r12261-linux-x86_64-sdl2
W=/root/stlab
rm -rf "$W/base"; mkdir -p "$W/base"
tar -xzf "$WIN/pkg/$N.tar.gz" -C "$W/base"
bash "$WIN/tools/f44_validate.sh" BEFORE "$W/base/$N/simutrans-rgba32" "$W/base/$N/simutrans-16bit"
bash "$WIN/tools/f44_validate.sh" AFTER  "$WIN/fix3bin/simutrans-rgba32" "$WIN/fix3bin/simutrans-16bit"
echo DONE
