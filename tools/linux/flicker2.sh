#!/bin/bash
# Tree flicker at the map edge, take 2.
#
#   flicker2.sh <tag> <binary> <paused|running>
#
# Take 1 never left the middle of the map: with no window manager on the Xvfb
# display there is no keyboard focus, so the arrow keys went nowhere. This one
# scrolls by dragging with the right mouse button through XTEST, which needs no
# focus, and it CHECKS that the northern edge is actually on screen before it
# measures anything.
#
# Two diffs are reported:
#   full    the whole window
#   strip   only the band containing the map edge and what overhangs it
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-blackwindow-01
W=$HOME/stlab/blackwindow
NAME=simutrans-rgba32-revised4-r12261-linux-x86_64-sdl2
TAG=$1; BIN=$2; MODE=${3:-paused}
OUT=$WIN/out/fl2-$TAG-$MODE
rm -rf "$OUT"; mkdir -p "$OUT"

C=$W/fl2-$TAG-$MODE; rm -rf "$C"; mkdir -p "$C"
tar -xzf "$HOME/stlab/rgba32-linux/pkg/$NAME.tar.gz" -C "$C"
D=$C/$NAME
cp "$BIN" "$D/simutrans-test"; chmod 755 "$D/simutrans-test"
cp -r /mnt/c/Users/Victor/dev/_stlab-rgba32-normal-scene-and-platform-matrix-01/run/pak "$D/pak"
mkdir -p "$D/save"; cp /mnt/c/Users/Victor/dev/simutrans/simutrans/save/tutorial64.sve "$D/save/"

PAUSEARG=""; [ "$MODE" = paused ] && PAUSEARG="-pause"
DISP=:$(( 50 + RANDOM % 8 ))
Xvfb "$DISP" -screen 0 1280x800x24 -nolisten tcp >/dev/null 2>&1 & XPID=$!
sleep 2
( cd "$D" && DISPLAY=$DISP SDL_VIDEODRIVER=x11 ./simutrans-test -singleuser -nosound -nomidi \
    $PAUSEARG -debug 2 -load tutorial64.sve >"$OUT/run.out" 2>"$OUT/run.err" ) & APID=$!
sleep 12
export DISPLAY=$DISP
WID=$(xdotool search --name "." 2>/dev/null | tail -1)
eval "$(xdotool getwindowgeometry --shell "$WID" 2>/dev/null)"
echo "=== fl2-$TAG-$MODE  window=$WID geometry=${WIDTH:-?}x${HEIGHT:-?} at ${X:-?},${Y:-?}"

# drag the map down with the right mouse button: this scrolls the view north
CX=$(( ${X:-0} + ${WIDTH:-704} / 2 ))
CY=$(( ${Y:-0} + ${HEIGHT:-560} / 2 ))
for i in $(seq 1 14); do
  xdotool mousemove "$CX" "$((CY-150))" mousedown 3 mousemove "$CX" "$((CY+200))" mouseup 3 2>/dev/null
  sleep 0.35
done
sleep 3

shot() { xwd -root -silent > "$OUT/$1.xwd" 2>/dev/null; convert "$OUT/$1.xwd" "$OUT/$1.png" 2>/dev/null; }

shot probe
# is the top of the map on screen? the band just under the toolbar should now
# contain the out-of-map background rather than ground tiles
STRIP="${WIDTH:-704}x120+${X:-0}+$(( ${Y:-0} + 40 ))"
convert "$OUT/probe.png" -crop "$STRIP" +repage "$OUT/probe-strip.png" 2>/dev/null
echo "  strip mean colour: $(convert "$OUT/probe-strip.png" -format '%[fx:mean.r],%[fx:mean.g],%[fx:mean.b]' info: 2>/dev/null)"
echo "  strip unique colours: $(convert "$OUT/probe-strip.png" -format '%k' info: 2>/dev/null)"

for n in $(seq -w 1 16); do
  shot "f$n"
  convert "$OUT/f$n.png" -crop "$STRIP" +repage "$OUT/s$n.png" 2>/dev/null
  sleep 0.2
done

echo "  consecutive frames ($MODE, camera still):"
prev=""; pstrip=""
for n in $(seq -w 1 16); do
  f="$OUT/f$n.png"; s="$OUT/s$n.png"
  if [ -n "$prev" ]; then
    d=$(compare -metric AE "$prev" "$f" null: 2>&1 | tr -d '\n')
    ds=$(compare -metric AE "$pstrip" "$s" null: 2>&1 | tr -d '\n')
    printf '    f%s full=%-10s edge-strip=%s\n' "$n" "$d" "$ds"
  fi
  prev=$f; pstrip=$s
done

kill "$APID" 2>/dev/null; wait "$APID" 2>/dev/null
kill "$XPID" 2>/dev/null; wait "$XPID" 2>/dev/null
