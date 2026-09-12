#!/bin/bash
# Reproduce Makie's report: a completely black window at startup, which a resize
# fixes and then shows the pakset selection.
#
#   repro.sh <case-tag> <executable> [pak|nopak] [extra args...]
#
# Each case gets its OWN extracted copy of the published archive, so -singleuser
# writes its settings into that copy and the cases cannot influence each other.
# Nothing outside $W is touched, and no personal savegame or config is used.
#
# Capture is xwd on a private Xvfb display, so this is a REAL X server - virtual,
# but a real X11 path through SDL2, not the dummy driver.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-blackwindow-01
W=$HOME/stlab/blackwindow
ARCHIVE=$HOME/stlab/rgba32-linux/pkg/simutrans-rgba32-revised4-r12261-linux-x86_64-sdl2.tar.gz
NAME=simutrans-rgba32-revised4-r12261-linux-x86_64-sdl2

TAG=$1; EXE=$2; PAKMODE=${3:-nopak}; shift 3 2>/dev/null || shift $#
OUT=$WIN/out/$TAG
mkdir -p "$OUT"
rm -f "$OUT"/*.png "$OUT"/*.xwd "$OUT"/*.txt 2>/dev/null

C=$W/$TAG
rm -rf "$C"; mkdir -p "$C"
tar -xzf "$ARCHIVE" -C "$C"
D=$C/$NAME
if [ "$PAKMODE" = "pak" ]; then
  cp -r /mnt/c/Users/Victor/dev/_stlab-rgba32-normal-scene-and-platform-matrix-01/run/pak "$D/pak"
fi

DISP=:$(( 90 + RANDOM % 8 ))
Xvfb "$DISP" -screen 0 1280x800x24 -nolisten tcp >"$OUT/xvfb.log" 2>&1 &
XPID=$!
sleep 2

( cd "$D" && DISPLAY=$DISP SDL_VIDEODRIVER=x11 ./"$EXE" -singleuser -nosound -nomidi -debug 3 "$@" \
    >"$OUT/run.out" 2>"$OUT/run.err" ) &
APID=$!
sleep 8

shot() {   # shot <label>
  DISPLAY=$DISP xwd -root -silent > "$OUT/$1.xwd" 2>/dev/null
  convert "$OUT/$1.xwd" "$OUT/$1.png" 2>/dev/null
  local stats
  stats=$(convert "$OUT/$1.png" -format "mean=%[fx:mean] max=%[fx:maxima] nonblack=%[fx:mean>0?1:0]" info: 2>/dev/null)
  # fraction of pixels that are not pure black
  local frac
  frac=$(convert "$OUT/$1.png" -fill white +opaque black -format "%[fx:mean]" info: 2>/dev/null)
  printf '  %-14s %s  non-black fraction=%s\n' "$1" "$stats" "$frac"
  echo "$1 $stats nonblack_fraction=$frac" >> "$OUT/frames.txt"
}

echo "=== $TAG ($EXE, $PAKMODE, display $DISP)"
shot before-resize-1
sleep 3
shot before-resize-2

# the resize Makie performed, sent to the window rather than done by hand
WID=$(DISPLAY=$DISP xdotool search --class -- "[Ss]imutrans" 2>/dev/null | head -1)
if [ -z "$WID" ]; then WID=$(DISPLAY=$DISP xdotool search --name "." 2>/dev/null | tail -1); fi
echo "  window id: ${WID:-none}"
if [ -n "$WID" ]; then
  DISPLAY=$DISP xdotool windowsize "$WID" 1000 700 2>/dev/null
  sleep 3
  shot after-resize-1
  DISPLAY=$DISP xdotool windowsize "$WID" 1100 760 2>/dev/null
  sleep 3
  shot after-resize-2
fi

kill "$APID" 2>/dev/null; wait "$APID" 2>/dev/null
kill "$XPID" 2>/dev/null; wait "$XPID" 2>/dev/null
echo "  log: $(grep -c . "$OUT/run.err") lines, errors=$(grep -ciE '^ERROR' "$OUT/run.err")"
