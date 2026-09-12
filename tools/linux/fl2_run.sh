#!/bin/bash
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-blackwindow-01
B=$HOME/stlab/blackwindow/fix1/build32/simutrans/simutrans
B16=$HOME/stlab/blackwindow/fix1/build16/simutrans/simutrans
bash "$WIN/tools/flicker2.sh" 32 "$B" paused
bash "$WIN/tools/flicker2.sh" 32 "$B" running
bash "$WIN/tools/flicker2.sh" 16 "$B16" running
echo DONE
