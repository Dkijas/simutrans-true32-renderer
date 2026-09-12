#!/bin/bash
# Tree flicker at the map edge: a FRAME SEQUENCE, not a still.
#
#   flicker.sh <tag> <binary> [zoom-in presses]
#
# The simulation is paused and the camera is left alone, so legitimate motion -
# vehicles, seasons, day/night - cannot be mistaken for the defect. With the
# world paused and the view still, two consecutive frames must be identical;
# every differing pixel is the thing being hunted.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-blackwindow-01
W=$HOME/stlab/blackwindow
NAME=simutrans-rgba32-revised4-r12261-linux-x86_64-sdl2
TAG=$1; BIN=$2; ZOOMIN=${3:-0}
OUT=$WIN/out/flicker-$TAG
rm -rf "$OUT"; mkdir -p "$OUT"

C=$W/fl-$TAG; rm -rf "$C"; mkdir -p "$C"
tar -xzf "$HOME/stlab/rgba32-linux/pkg/$NAME.tar.gz" -C "$C"
D=$C/$NAME
cp "$BIN" "$D/simutrans-test"; chmod 755 "$D/simutrans-test"
cp -r /mnt/c/Users/Victor/dev/_stlab-rgba32-normal-scene-and-platform-matrix-01/run/pak "$D/pak"
mkdir -p "$D/save"
cp /mnt/c/Users/Victor/dev/simutrans/simutrans/save/tutorial64.sve "$D/save/"

DISP=:$(( 60 + RANDOM % 8 ))
Xvfb "$DISP" -screen 0 1280x800x24 -nolisten tcp >/dev/null 2>&1 & XPID=$!
sleep 2
( cd "$D" && DISPLAY=$DISP SDL_VIDEODRIVER=x11 ./simutrans-test -singleuser -nosound -nomidi \
    -pause -debug 2 -load tutorial64.sve >"$OUT/run.out" 2>"$OUT/run.err" ) & APID=$!
sleep 12

export DISPLAY=$DISP
WID=$(xdotool search --name "." 2>/dev/null | tail -1)
echo "=== flicker-$TAG  window=$WID"
xdotool windowactivate "$WID" 2>/dev/null
sleep 1

# scroll to the northern edge of the map, where a tree can overhang
xdotool key --window "$WID" --repeat 80 --repeat-delay 12 Up 2>/dev/null
sleep 2
# optional zoom
i=0; while [ "$i" -lt "$ZOOMIN" ]; do xdotool key --window "$WID" plus 2>/dev/null; i=$((i+1)); done
sleep 3

# let everything settle, then capture a still sequence
for n in $(seq -w 1 14); do
  xwd -root -silent > "$OUT/f$n.xwd" 2>/dev/null
  convert "$OUT/f$n.xwd" "$OUT/f$n.png" 2>/dev/null
  sleep 0.25
done

echo "  consecutive-frame differences (world paused, camera still):"
prev=""
for f in "$OUT"/f*.png; do
  if [ -n "$prev" ]; then
    d=$(compare -metric AE "$prev" "$f" null: 2>&1 | tr -d '\n')
    printf '    %s -> %s : %s differing pixels\n' "$(basename "$prev")" "$(basename "$f")" "$d"
  fi
  prev=$f
done

kill "$APID" 2>/dev/null; wait "$APID" 2>/dev/null
kill "$XPID" 2>/dev/null; wait "$XPID" 2>/dev/null
