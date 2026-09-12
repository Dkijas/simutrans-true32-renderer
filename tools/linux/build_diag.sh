#!/bin/bash
# Build the instrumented 32-bit binary and run it in the failing case.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-blackwindow-01
W=$HOME/stlab/blackwindow
LOG=$WIN/logs
mkdir -p "$LOG"

rm -rf "$W/diag"; mkdir -p "$W/diag"
cp -r "$WIN/src-diag/." "$W/diag/tree/"
cmake -S "$W/diag/tree" -B "$W/diag/build32" \
  -DCMAKE_BUILD_TYPE=Release -DSIMUTRANS_BACKEND=sdl2 \
  -DSIMUTRANS_MULTI_THREAD=ON -DSIMUTRANS_TRUE32=ON \
  -DSIMUTRANS_USE_REVISION=12261 > "$LOG/diag.configure.log" 2>&1
echo "configure rc=$?"
cmake --build "$W/diag/build32" --target simutrans -j "$(nproc)" > "$LOG/diag.build.log" 2>&1
echo "build rc=$?  errors=$(grep -ci 'error:' "$LOG/diag.build.log")"
ls -l "$W/diag/build32/simutrans/simutrans" 2>&1 | tail -1
