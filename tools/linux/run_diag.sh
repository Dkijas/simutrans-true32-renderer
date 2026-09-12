#!/bin/bash
# Run the instrumented binary in the failing case and in the working one.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-blackwindow-01
W=$HOME/stlab/blackwindow
NAME=simutrans-rgba32-revised4-r12261-linux-x86_64-sdl2
BIN=$W/diag/build32/simutrans/simutrans

one() {
  local tag=$1 pakmode=$2
  local OUT=$WIN/out/diag-$tag
  mkdir -p "$OUT"
  local C=$W/diag-$tag
  rm -rf "$C"; mkdir -p "$C"
  tar -xzf "$W/../rgba32-linux/pkg/$NAME.tar.gz" -C "$C"
  local D=$C/$NAME
  cp "$BIN" "$D/simutrans-diag"
  chmod 755 "$D/simutrans-diag"
  [ "$pakmode" = pak ] && cp -r /mnt/c/Users/Victor/dev/_stlab-rgba32-normal-scene-and-platform-matrix-01/run/pak "$D/pak"

  local DISP=:$(( 80 + RANDOM % 8 ))
  Xvfb "$DISP" -screen 0 1280x800x24 -nolisten tcp >/dev/null 2>&1 &
  local XPID=$!
  sleep 2
  ( cd "$D" && DISPLAY=$DISP SDL_VIDEODRIVER=x11 ./simutrans-diag -singleuser -nosound -nomidi -debug 3 \
      >"$OUT/run.out" 2>"$OUT/run.err" ) &
  local APID=$!
  sleep 10
  DISPLAY=$DISP xwd -root -silent > "$OUT/before.xwd" 2>/dev/null
  convert "$OUT/before.xwd" "$OUT/before.png" 2>/dev/null
  echo "=== diag-$tag ($pakmode)"
  echo "  screen non-black fraction BEFORE resize: $(convert "$OUT/before.png" -fill white +opaque black -format '%[fx:mean]' info: 2>/dev/null)"
  local WID
  WID=$(DISPLAY=$DISP xdotool search --name "." 2>/dev/null | tail -1)
  [ -n "$WID" ] && DISPLAY=$DISP xdotool windowsize "$WID" 1000 700 2>/dev/null
  sleep 4
  DISPLAY=$DISP xwd -root -silent > "$OUT/after.xwd" 2>/dev/null
  convert "$OUT/after.xwd" "$OUT/after.png" 2>/dev/null
  echo "  screen non-black fraction AFTER  resize: $(convert "$OUT/after.png" -fill white +opaque black -format '%[fx:mean]' info: 2>/dev/null)"
  kill "$APID" 2>/dev/null; wait "$APID" 2>/dev/null
  kill "$XPID" 2>/dev/null; wait "$XPID" 2>/dev/null
  echo "  --- probe lines:"
  grep "stlab_probe" "$OUT/run.err" | head -24 | sed 's/^/    /'
}

one nopak nopak
one pak pak
echo DONE
