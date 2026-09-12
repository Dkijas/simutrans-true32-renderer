#!/bin/bash
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-blackwindow-01
B32=$HOME/stlab/blackwindow/fix1/build32/simutrans/simutrans
B16=$HOME/stlab/blackwindow/fix1/build16/simutrans/simutrans
bash "$WIN/tools/flicker3.sh" 32 "$B32" paused
bash "$WIN/tools/flicker3.sh" 32 "$B32" running
bash "$WIN/tools/flicker3.sh" 16 "$B16" running
echo DONE
