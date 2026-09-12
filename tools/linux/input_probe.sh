#!/bin/bash
# Can I drive this window at all? Without a window manager the X input focus is
# PointerRoot, so XTEST keys go to the window under the POINTER. xdotool's
# --window form uses XSendEvent, which SDL ignores. Establish which works before
# building any measurement on top of it.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-blackwindow-01
W=$HOME/stlab/blackwindow
NAME=simutrans-rgba32-revised4-r12261-linux-x86_64-sdl2
OUT=$WIN/out/input-probe
rm -rf "$OUT"; mkdir -p "$OUT"

C=$W/inp; rm -rf "$C"; mkdir -p "$C"
tar -xzf "$HOME/stlab/rgba32-linux/pkg/$NAME.tar.gz" -C "$C"
D=$C/$NAME
cp "$HOME/stlab/blackwindow/fix1/build32/simutrans/simutrans" "$D/simutrans-test"; chmod 755 "$D/simutrans-test"
cp -r /mnt/c/Users/Victor/dev/_stlab-rgba32-normal-scene-and-platform-matrix-01/run/pak "$D/pak"
mkdir -p "$D/save"; cp /mnt/c/Users/Victor/dev/simutrans/simutrans/save/tutorial64.sve "$D/save/"

DISP=:$(( 40 + RANDOM % 8 ))
Xvfb "$DISP" -screen 0 1280x800x24 -nolisten tcp >/dev/null 2>&1 & XPID=$!
sleep 2
( cd "$D" && DISPLAY=$DISP SDL_VIDEODRIVER=x11 ./simutrans-test -singleuser -nosound -nomidi \
    -pause -debug 2 -load tutorial64.sve >"$OUT/run.out" 2>"$OUT/run.err" ) & APID=$!
sleep 12
export DISPLAY=$DISP
WID=$(xdotool search --name "." | tail -1)
eval "$(xdotool getwindowgeometry --shell "$WID")"
CX=$(( X + WIDTH/2 )); CY=$(( Y + HEIGHT/2 ))
echo "window $WID  ${WIDTH}x${HEIGHT} at $X,$Y ; pointer to $CX,$CY"
xdotool mousemove "$CX" "$CY"; sleep 1

cap() { xwd -root -silent > "$OUT/$1.xwd"; convert "$OUT/$1.xwd" "$OUT/$1.png"; }
cap base

# XTEST keys, no --window: they follow the pointer when focus is PointerRoot
for k in minus minus minus; do xdotool key "$k"; sleep 0.6; done
sleep 2
cap zoomout
echo "  after 3x 'minus': changed pixels vs base = $(compare -metric AE "$OUT/base.png" "$OUT/zoomout.png" null: 2>&1 | tr -d '\n')"

# and the map size, straight from the save
echo "  map size from the log: $(grep -iE "welt.*size|Map size|groesse|width.*height" "$OUT/run.err" | head -3 | tr '\n' ' ')"

kill "$APID" 2>/dev/null; wait "$APID" 2>/dev/null
kill "$XPID" 2>/dev/null; wait "$XPID" 2>/dev/null
