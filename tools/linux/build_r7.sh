#!/bin/bash
# revised-7 (= revised-6 + FIX-3) built for Linux with bzip2 linked STATICALLY.
#
# The only unresolved dependency on Fedora is libbz2.so.1.0, and it is
# unresolvable there by construction: Debian's bzip2 declares SONAME
# libbz2.so.1.0, Fedora's declares libbz2.so.1. Linking the static archive
# removes the dependency instead of asking the tester to fake a SONAME with a
# symlink, which is what the report had to resort to.
#
# bzip2's licence is BSD-style and permits redistribution in binary form.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-fedora-01
W=$HOME/stlab/r7
LOG=$WIN/logs
BZ_STATIC=/usr/lib/x86_64-linux-gnu/libbz2.a
mkdir -p "$LOG"
rm -rf "$W"; mkdir -p "$W"
cp -r "$WIN/src-r7/." "$W/tree/"

for d in 32 16; do
  t=ON; [ "$d" = 16 ] && t=OFF
  cmake -S "$W/tree" -B "$W/b$d" -DCMAKE_BUILD_TYPE=Release \
    -DSIMUTRANS_BACKEND=sdl2 -DSIMUTRANS_MULTI_THREAD=ON -DSIMUTRANS_TRUE32=$t \
    -DSIMUTRANS_USE_REVISION=12261 \
    -DBZIP2_LIBRARY_RELEASE="$BZ_STATIC" -DBZIP2_LIBRARIES="$BZ_STATIC" \
    > "$LOG/r7-$d.configure.log" 2>&1
  echo "configure r7-$d rc=$?"
  cmake --build "$W/b$d" --target simutrans -j "$(nproc)" > "$LOG/r7-$d.build.log" 2>&1
  echo "build     r7-$d rc=$?  warnings=$(grep -c 'warning:' "$LOG/r7-$d.build.log")  errors=$(grep -ci 'error:' "$LOG/r7-$d.build.log")"
  b="$W/b$d/simutrans/simutrans"
  if [ -f "$b" ]; then
    echo "  bz2 still dynamic? $(objdump -p "$b" | grep -c 'libbz2')"
    echo "  NEEDED: $(objdump -p "$b" | awk '/NEEDED/{printf "%s ", $2}')"
  fi
done
