#!/bin/bash
# Build FIX-1 (32 and 16 bit from the same tree) and test both startup cases.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-blackwindow-01
W=$HOME/stlab/blackwindow
NAME=simutrans-rgba32-revised4-r12261-linux-x86_64-sdl2
LOG=$WIN/logs

rm -rf "$W/fix1"; mkdir -p "$W/fix1"
cp -r "$WIN/src-fix1/." "$W/fix1/tree/"
for d in 32 16; do
  t=ON; [ "$d" = 16 ] && t=OFF
  cmake -S "$W/fix1/tree" -B "$W/fix1/build$d" -DCMAKE_BUILD_TYPE=Release \
    -DSIMUTRANS_BACKEND=sdl2 -DSIMUTRANS_MULTI_THREAD=ON -DSIMUTRANS_TRUE32=$t \
    -DSIMUTRANS_USE_REVISION=12261 > "$LOG/fix1-$d.configure.log" 2>&1
  echo "configure fix1-$d rc=$?"
  cmake --build "$W/fix1/build$d" --target simutrans -j "$(nproc)" > "$LOG/fix1-$d.build.log" 2>&1
  echo "build     fix1-$d rc=$?  warnings=$(grep -c 'warning:' "$LOG/fix1-$d.build.log")  errors=$(grep -ci 'error:' "$LOG/fix1-$d.build.log")"
done

test_one() {   # test_one <tag> <built binary> <pak|nopak>
  local tag=$1 bin=$2 pakmode=$3
  local OUT=$WIN/out/$tag; mkdir -p "$OUT"
  local C=$W/t-$tag; rm -rf "$C"; mkdir -p "$C"
  tar -xzf "$HOME/stlab/rgba32-linux/pkg/$NAME.tar.gz" -C "$C"
  local D=$C/$NAME
  cp "$bin" "$D/simutrans-test"; chmod 755 "$D/simutrans-test"
  [ "$pakmode" = pak ] && cp -r /mnt/c/Users/Victor/dev/_stlab-rgba32-normal-scene-and-platform-matrix-01/run/pak "$D/pak"
  local DISP=:$(( 70 + RANDOM % 8 ))
  Xvfb "$DISP" -screen 0 1280x800x24 -nolisten tcp >/dev/null 2>&1 & local XPID=$!
  sleep 2
  ( cd "$D" && DISPLAY=$DISP SDL_VIDEODRIVER=x11 ./simutrans-test -singleuser -nosound -nomidi -debug 3 \
      >"$OUT/run.out" 2>"$OUT/run.err" ) & local APID=$!
  sleep 9
  DISPLAY=$DISP xwd -root -silent > "$OUT/before.xwd" 2>/dev/null
  convert "$OUT/before.xwd" "$OUT/before.png" 2>/dev/null
  local b; b=$(convert "$OUT/before.png" -fill white +opaque black -format '%[fx:mean]' info: 2>/dev/null)
  local WID; WID=$(DISPLAY=$DISP xdotool search --name "." 2>/dev/null | tail -1)
  [ -n "$WID" ] && DISPLAY=$DISP xdotool windowsize "$WID" 1000 700 2>/dev/null
  sleep 4
  DISPLAY=$DISP xwd -root -silent > "$OUT/after.xwd" 2>/dev/null
  convert "$OUT/after.xwd" "$OUT/after.png" 2>/dev/null
  local a; a=$(convert "$OUT/after.png" -fill white +opaque black -format '%[fx:mean]' info: 2>/dev/null)
  kill "$APID" 2>/dev/null; wait "$APID" 2>/dev/null
  kill "$XPID" 2>/dev/null; wait "$XPID" 2>/dev/null
  printf '  %-22s BEFORE resize=%-10s AFTER resize=%-10s errors=%s\n' "$tag" "$b" "$a" "$(grep -ciE '^ERROR' "$OUT/run.err")"
}

echo "=== FIX-1 results (non-black fraction of the screen)"
test_one fix1-32-nopak "$W/fix1/build32/simutrans/simutrans" nopak
test_one fix1-32-pak   "$W/fix1/build32/simutrans/simutrans" pak
test_one fix1-16-nopak "$W/fix1/build16/simutrans/simutrans" nopak
test_one fix1-16-pak   "$W/fix1/build16/simutrans/simutrans" pak
echo DONE
