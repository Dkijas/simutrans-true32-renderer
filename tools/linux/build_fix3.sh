#!/bin/bash
# Build FIX-3 (32 and 16 bit) on Ubuntu, and report what CMake has to say about
# bzip2 - the other half of this mission.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-fedora-01
W=$HOME/stlab/fedorafix
LOG=$WIN/logs
mkdir -p "$LOG"
rm -rf "$W"; mkdir -p "$W"
cp -r "$WIN/src-fix3/." "$W/tree/"

for d in 32 16; do
  t=ON; [ "$d" = 16 ] && t=OFF
  cmake -S "$W/tree" -B "$W/b$d" -DCMAKE_BUILD_TYPE=Release \
    -DSIMUTRANS_BACKEND=sdl2 -DSIMUTRANS_MULTI_THREAD=ON -DSIMUTRANS_TRUE32=$t \
    -DSIMUTRANS_USE_REVISION=12261 > "$LOG/fix3-$d.configure.log" 2>&1
  echo "configure fix3-$d rc=$?"
  cmake --build "$W/b$d" --target simutrans -j "$(nproc)" > "$LOG/fix3-$d.build.log" 2>&1
  echo "build     fix3-$d rc=$?  warnings=$(grep -c 'warning:' "$LOG/fix3-$d.build.log")  errors=$(grep -ci 'error:' "$LOG/fix3-$d.build.log")"
done

echo "=== what bzip2 looks like on this build host"
ls -l /usr/lib/x86_64-linux-gnu/libbz2.* 2>/dev/null | sed 's/^/  /'
echo "  SONAME of the shared one: $(objdump -p /usr/lib/x86_64-linux-gnu/libbz2.so.1.0 2>/dev/null | awk '/SONAME/{print $2}')"
echo "  static archive present  : $([ -e /usr/lib/x86_64-linux-gnu/libbz2.a ] && echo yes || echo NO)"
echo "=== how the project asks for it"
grep -rn "BZip2\|BZIP2\|bz2" "$W/tree/CMakeLists.txt" | head -8 | sed 's/^/  /'
