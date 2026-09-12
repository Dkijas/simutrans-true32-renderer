#!/bin/bash
# Tree flicker at the map edge, take 3 - with input that actually arrives.
#
#   flicker3.sh <tag> <binary> <paused|running>
#
# xdotool windowfocus (XSetInputFocus) gives the window the input focus without
# a window manager; XTEST keys then reach it. Verified separately: 40x Up moves
# 35% of the screen.
#
# The view is scrolled north until the map edge is on screen, which is CHECKED,
# then the camera is left alone and a sequence is captured. Two measurements:
# the whole window, and the band that holds the edge and whatever overhangs it.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-blackwindow-01
W=$HOME/stlab/blackwindow
NAME=simutrans-rgba32-revised4-r12261-linux-x86_64-sdl2
TAG=$1; BIN=$2; MODE=${3:-paused}
OUT=$WIN/out/fl3-$TAG-$MODE
rm -rf "$OUT"; mkdir -p "$OUT"
C=$W/fl3-$TAG-$MODE; rm -rf "$C"; mkdir -p "$C"
tar -xzf "$HOME/stlab/rgba32-linux/pkg/$NAME.tar.gz" -C "$C"
D=$C/$NAME
cp "$BIN" "$D/simutrans-test"; chmod 755 "$D/simutrans-test"
cp -r /mnt/c/Users/Victor/dev/_stlab-rgba32-normal-scene-and-platform-matrix-01/run/pak "$D/pak"
mkdir -p "$D/save"; cp /mnt/c/Users/Victor/dev/simutrans/simutrans/save/tutorial64.sve "$D/save/"

PAUSEARG=""; [ "$MODE" = paused ] && PAUSEARG="-pause"
DISP=:$(( 20 + RANDOM % 8 ))
Xvfb "$DISP" -screen 0 1280x800x24 -nolisten tcp >/dev/null 2>&1 & XPID=$!
sleep 2
( cd "$D" && DISPLAY=$DISP SDL_VIDEODRIVER=x11 ./simutrans-test -singleuser -nosound -nomidi \
    $PAUSEARG -debug 2 -load tutorial64.sve >"$OUT/run.out" 2>"$OUT/run.err" ) & APID=$!
sleep 12
export DISPLAY=$DISP
WID=$(xdotool search --name "." | tail -1)
eval "$(xdotool getwindowgeometry --shell "$WID")"
xdotool mousemove $((X+WIDTH/2)) $((Y+HEIGHT/2))
xdotool windowfocus --sync "$WID" 2>/dev/null
sleep 1
echo "=== fl3-$TAG-$MODE window=$WID ${WIDTH}x${HEIGHT} at $X,$Y"

cap() { xwd -root -silent > "$OUT/$1.xwd"; convert "$OUT/$1.xwd" "$OUT/$1.png"; }
STRIP="${WIDTH}x140+${X}+$((Y+36))"
strip_of() { convert "$OUT/$1.png" -crop "$STRIP" +repage "$OUT/$1-strip.png"; }

# scroll north in steps and stop when the band under the toolbar stops being ground
for step in 1 2 3 4 5 6 7 8; do
  for i in $(seq 1 40); do xdotool key Up; done
  sleep 1.5
  cap probe$step; strip_of probe$step
  m=$(convert "$OUT/probe$step-strip.png" -format '%[fx:mean]' info:)
  k=$(convert "$OUT/probe$step-strip.png" -format '%k' info:)
  echo "  after $((step*40)) Up: strip mean=$m unique=$k"
done
sleep 2

for n in $(seq -w 1 16); do
  cap "f$n"; strip_of "f$n"
  sleep 0.2
done
echo "  consecutive frames ($MODE, camera still, edge on screen):"
for n in $(seq -w 2 16); do
  p=$(printf '%02d' $((10#$n - 1)))
  d=$(compare -metric AE "$OUT/f$p.png" "$OUT/f$n.png" null: 2>&1 | tr -d '\n')
  ds=$(compare -metric AE "$OUT/f$p-strip.png" "$OUT/f$n-strip.png" null: 2>&1 | tr -d '\n')
  printf '    f%s->f%s full=%-14s edge-strip=%s\n' "$p" "$n" "$d" "$ds"
done
kill "$APID" 2>/dev/null; wait "$APID" 2>/dev/null
kill "$XPID" 2>/dev/null; wait "$XPID" 2>/dev/null
