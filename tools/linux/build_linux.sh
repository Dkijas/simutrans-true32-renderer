#!/bin/bash
# Linux x86_64 tester builds from the PUBLISHED tree: r12261 plus the certified
# patch, uniform LF, no probes and no lab residue.
#
#   build_linux.sh
#
# Both arms come out of this one script, so the toolchain and the options are
# identical by construction; only SIMUTRANS_TRUE32 differs.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-linux-delivery-01
SRCWIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-testing-release-01/tree-final
W=$HOME/stlab/rgba32-linux
LOG=$WIN/logs
mkdir -p "$LOG"

# build on the Linux filesystem, not on /mnt/c: 9p is slow and does not carry
# the permission bits the package needs
rm -rf "$W"; mkdir -p "$W"
cp -r "$SRCWIN/." "$W/tree/"
echo "tree copied: $(find "$W/tree" -type f | wc -l) files"

build_one() {
  local name=$1 true32=$2
  cmake -S "$W/tree" -B "$W/$name" \
    -DCMAKE_BUILD_TYPE=Release \
    -DSIMUTRANS_BACKEND=sdl2 \
    -DSIMUTRANS_MULTI_THREAD=ON \
    -DSIMUTRANS_TRUE32=$true32 \
    -DSIMUTRANS_USE_REVISION=12261 \
    > "$LOG/$name.configure.log" 2>&1
  echo "configure $name (SIMUTRANS_TRUE32=$true32) rc=$?"
  cmake --build "$W/$name" --target simutrans -j "$(nproc)" > "$LOG/$name.build.log" 2>&1
  echo "build     $name rc=$?  warnings=$(grep -c 'warning:' "$LOG/$name.build.log")"
  echo "  renderer sources: $(grep -o 'simgraph[0-9]*\.cc\|stlab_xr32\.cc' "$W/$name/CMakeFiles/simutrans.dir/build.make" | sort -u | tr '\n' ' ')"
  echo "  COLOUR_DEPTH    : $(grep -o 'COLOUR_DEPTH=[0-9]*' "$W/$name/CMakeFiles/simutrans.dir/flags.make" | sort -u | tr '\n' ' ')"
  ls -l "$W/$name/simutrans/simutrans" 2>&1 | tail -1 | sed 's/^/  /'
}

build_one build32 ON
build_one build16 OFF
echo DONE
