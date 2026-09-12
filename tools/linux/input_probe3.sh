#!/bin/bash
# Last attempt at driving the window on a bare Xvfb: XSetInputFocus via
# `xdotool windowfocus`, which needs no window manager, followed by XTEST keys.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-blackwindow-01
W=$HOME/stlab/blackwindow
NAME=simutrans-rgba32-revised4-r12261-linux-x86_64-sdl2
OUT=$WIN/out/input-probe3
rm -rf "$OUT"; mkdir -p "$OUT"
C=$W/inp3; rm -rf "$C"; mkdir -p "$C"
tar -xzf "$HOME/stlab/rgba32-linux/pkg/$NAME.tar.gz" -C "$C"
D=$C/$NAME
cp "$HOME/stlab/blackwindow/fix1/build32/simutrans/simutrans" "$D/simutrans-test"; chmod 755 "$D/simutrans-test"
cp -r /mnt/c/Users/Victor/dev/_stlab-rgba32-normal-scene-and-platform-matrix-01/run/pak "$D/pak"
mkdir -p "$D/save"; cp /mnt/c/Users/Victor/dev/simutrans/simutrans/save/tutorial64.sve "$D/save/"

DISP=:$(( 30 + RANDOM % 8 ))
Xvfb "$DISP" -screen 0 1280x800x24 -nolisten tcp >/dev/null 2>&1 & XPID=$!
sleep 2
( cd "$D" && DISPLAY=$DISP SDL_VIDEODRIVER=x11 ./simutrans-test -singleuser -nosound -nomidi \
    -pause -debug 2 -load tutorial64.sve >"$OUT/run.out" 2>"$OUT/run.err" ) & APID=$!
sleep 12
export DISPLAY=$DISP
WID=$(xdotool search --name "." | tail -1)
eval "$(xdotool getwindowgeometry --shell "$WID")"
xdotool mousemove $((X+WIDTH/2)) $((Y+HEIGHT/2))
xdotool windowfocus --sync "$WID" 2>/dev/null; echo "  windowfocus rc=$?"
echo "  focus now: $(xdotool getwindowfocus 2>/dev/null)  (window is $WID)"
sleep 1
cap() { xwd -root -silent > "$OUT/$1.xwd"; convert "$OUT/$1.xwd" "$OUT/$1.png"; }
cap base
for i in 1 2 3; do xdotool key minus; sleep 0.7; done
sleep 2; cap zoomout
echo "  3x minus  -> changed pixels: $(compare -metric AE "$OUT/base.png" "$OUT/zoomout.png" null: 2>&1 | tr -d '\n')"
for i in $(seq 1 40); do xdotool key Up; done
sleep 2; cap scrolled
echo "  40x Up    -> changed pixels: $(compare -metric AE "$OUT/zoomout.png" "$OUT/scrolled.png" null: 2>&1 | tr -d '\n')"
kill "$APID" 2>/dev/null; wait "$APID" 2>/dev/null
kill "$XPID" 2>/dev/null; wait "$XPID" 2>/dev/null
