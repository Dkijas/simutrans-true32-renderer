#!/bin/bash
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-blackwindow-01
W=$HOME/stlab/blackwindow
bash "$WIN/tools/flicker.sh" 32 "$W/fix1/build32/simutrans/simutrans" 0
bash "$WIN/tools/flicker.sh" 16 "$W/fix1/build16/simutrans/simutrans" 0
echo DONE
