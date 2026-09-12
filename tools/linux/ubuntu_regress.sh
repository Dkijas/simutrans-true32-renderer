#!/bin/bash
# The Ubuntu reference must not regress: same package, same checks.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-fedora-01
N=simutrans-rgba32-revised7-r12261-linux-x86_64-sdl2
W=$HOME/stlab/ureg
OUT=$WIN/out/UBUNTU
mkdir -p "$OUT"; rm -rf "$W"; mkdir -p "$W"
tar -xzf "$WIN/pkg/$N.tar.gz" -C "$W"
D=$W/$N
cp -r /mnt/c/Users/Victor/dev/_stlab-rgba32-normal-scene-and-platform-matrix-01/run/pak "$D/pak"
mkdir -p "$D/save"; cp /mnt/c/Users/Victor/dev/simutrans/simutrans/save/tutorial64.sve "$D/save/"
echo "=== unresolved libraries on Ubuntu"
for b in simutrans-rgba32 simutrans-16bit; do
  echo "  $b: $(ldd "$D/$b" 2>&1 | grep -ci 'not found')  bz2 in NEEDED: $(objdump -p "$D/$b" | grep -c libbz2)"
done
run() {
  local tag=$1 b=$2; shift 2
  local DISP=:$(( 40 + RANDOM % 20 ))
  Xvfb "$DISP" -screen 0 1280x800x24 -nolisten tcp >/dev/null 2>&1 & local XPID=$!
  sleep 2
  ( cd "$D" && DISPLAY=$DISP SDL_VIDEODRIVER=x11 timeout 25 ./"$b" -singleuser -nosound -nomidi -debug 3 "$@" \
      >/dev/null 2>"$OUT/$tag.log" ) & local APID=$!
  sleep 11
  DISPLAY=$DISP xwd -root -silent > "$OUT/$tag.xwd" 2>/dev/null
  convert "$OUT/$tag.xwd" "$OUT/$tag.png" 2>/dev/null
  printf '  %-14s painted=%-10s world=%s errors=%s  font=%s\n' "$tag" \
    "$(convert "$OUT/$tag.png" -fill white +opaque black -format '%[fx:mean]' info: 2>/dev/null)" \
    "$(grep -c 'Running world' "$OUT/$tag.log")" "$(grep -ciE '^ERROR' "$OUT/$tag.log")" \
    "$(grep -oiE "trying to load '[^']*'" "$OUT/$tag.log" | tail -1)"
  kill "$APID" 2>/dev/null; wait "$APID" 2>/dev/null
  kill "$XPID" 2>/dev/null; wait "$XPID" 2>/dev/null
}
echo "=== runs"
run u-32-pak simutrans-rgba32 -load tutorial64.sve
run u-16-pak simutrans-16bit  -load tutorial64.sve
