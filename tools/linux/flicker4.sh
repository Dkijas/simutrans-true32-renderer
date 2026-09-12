#!/bin/bash
# Tree flicker at the map edge, take 4: stop AT the edge, not past it.
#
#   flicker4.sh <tag> <binary> <paused|running> <up-presses>
#
# Take 3 scrolled 320 tiles north and ended entirely off the map, where the band
# is flat background with 15 colours and nothing can flicker. The transition -
# map edge inside the measured band, trees able to overhang it - was around 120
# presses. This one stops at a given position and measures there.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-blackwindow-01
W=$HOME/stlab/blackwindow
NAME=simutrans-rgba32-revised4-r12261-linux-x86_64-sdl2
TAG=$1; BIN=$2; MODE=$3; UPS=$4
OUT=$WIN/out/fl4-$TAG-$MODE-$UPS
rm -rf "$OUT"; mkdir -p "$OUT"
C=$W/fl4-$TAG-$MODE-$UPS; rm -rf "$C"; mkdir -p "$C"
tar -xzf "$HOME/stlab/rgba32-linux/pkg/$NAME.tar.gz" -C "$C"
D=$C/$NAME
cp "$BIN" "$D/simutrans-test"; chmod 755 "$D/simutrans-test"
cp -r /mnt/c/Users/Victor/dev/_stlab-rgba32-normal-scene-and-platform-matrix-01/run/pak "$D/pak"
mkdir -p "$D/save"; cp /mnt/c/Users/Victor/dev/simutrans/simutrans/save/tutorial64.sve "$D/save/"

PAUSEARG=""; [ "$MODE" = paused ] && PAUSEARG="-pause"
DISP=:$(( 10 + RANDOM % 8 ))
Xvfb "$DISP" -screen 0 1280x800x24 -nolisten tcp >/dev/null 2>&1 & XPID=$!
sleep 2
( cd "$D" && DISPLAY=$DISP SDL_VIDEODRIVER=x11 ./simutrans-test -singleuser -nosound -nomidi \
    $PAUSEARG -debug 2 -load tutorial64.sve >"$OUT/run.out" 2>"$OUT/run.err" ) & APID=$!
sleep 12
export DISPLAY=$DISP
WID=$(xdotool search --name "." | tail -1)
eval "$(xdotool getwindowgeometry --shell "$WID")"
xdotool mousemove $((X+WIDTH/2)) $((Y+HEIGHT/2)); xdotool windowfocus --sync "$WID" 2>/dev/null
sleep 1
for i in $(seq 1 "$UPS"); do xdotool key Up; done
sleep 3

cap()  { xwd -root -silent > "$OUT/$1.xwd"; convert "$OUT/$1.xwd" "$OUT/$1.png"; }
# the measured band: the upper half of the view, which now holds the map edge
STRIP="${WIDTH}x240+${X}+$((Y+36))"
strip_of() { convert "$OUT/$1.png" -crop "$STRIP" +repage "$OUT/$1-strip.png"; }

cap probe; strip_of probe
echo "=== fl4-$TAG-$MODE ${UPS} Up : strip mean=$(convert "$OUT/probe-strip.png" -format '%[fx:mean]' info:) unique=$(convert "$OUT/probe-strip.png" -format '%k' info:)"

for n in $(seq -w 1 20); do cap "f$n"; strip_of "f$n"; sleep 0.2; done
echo "  consecutive frames:"
tot=0
for n in $(seq -w 2 20); do
  p=$(printf '%02d' $((10#$n - 1)))
  d=$(compare -metric AE "$OUT/f$p.png" "$OUT/f$n.png" null: 2>&1 | sed 's/ .*//')
  ds=$(compare -metric AE "$OUT/f$p-strip.png" "$OUT/f$n-strip.png" null: 2>&1 | sed 's/ .*//')
  printf '    f%s->f%s full=%-10s edge-band=%s\n' "$p" "$n" "$d" "$ds"
done
kill "$APID" 2>/dev/null; wait "$APID" 2>/dev/null
kill "$XPID" 2>/dev/null; wait "$XPID" 2>/dev/null
