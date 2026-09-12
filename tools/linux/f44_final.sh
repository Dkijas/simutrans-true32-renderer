#!/bin/bash
# Final validation of the revised-7 package on Fedora 44, from the extracted
# archive, with the diagnostic symlink REMOVED first so nothing is being faked.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-fedora-01
N7=simutrans-rgba32-revised7-r12261-linux-x86_64-sdl2
N5=simutrans-rgba32-revised5-r12261-linux-x86_64-sdl2
W=/root/final
OUT=$WIN/out/FINAL
mkdir -p "$OUT"; rm -f "$OUT"/*.png "$OUT"/*.log

echo "=== removing the diagnostic symlink, so the system is a plain Fedora again"
rm -f /usr/lib64/libbz2.so.1.0
ls -l /usr/lib64/libbz2* | sed 's/^/  /'

rm -rf "$W"; mkdir -p "$W"
tar -xzf "$WIN/pkg/$N7.tar.gz" -C "$W"
tar -xzf "$WIN/pkg/$N5.tar.gz" -C "$W"
D7=$W/$N7
D5=$W/$N5

echo "=== dependency resolution, no manual links anywhere"
for p in "$D5/simutrans-rgba32:revised-5 (the reported build)" "$D7/simutrans-rgba32:revised-7 32 bit" "$D7/simutrans-16bit:revised-7 16 bit"; do
  b="${p%%:*}"; n="${p##*:}"
  echo "  --- $n"
  echo "     unresolved: $(ldd "$b" 2>&1 | grep -ci 'not found')"
  ldd "$b" 2>&1 | grep -i "not found" | sed 's/^/     MISSING /'
  echo "     starts    : $( ( cd "$(dirname "$b")" && timeout 5 "$b" --version >/dev/null 2>&1; echo rc=$? ) )"
done

echo "=== a pakset and a copy of a savegame, for the world test"
cp -r /mnt/c/Users/Victor/dev/_stlab-rgba32-normal-scene-and-platform-matrix-01/run/pak "$D7/pak" 2>/dev/null
mkdir -p "$D7/save"
cp /mnt/c/Users/Victor/dev/simutrans/simutrans/save/tutorial64.sve "$D7/save/" 2>/dev/null

shot() { DISPLAY=$1 magick import -window root "$OUT/$2.png" 2>/dev/null; }

run() {  # run <tag> <dir> <binary> <extra args...>
  local tag=$1 D=$2 b=$3; shift 3
  local DISP=:$(( 60 + RANDOM % 30 ))
  Xvfb "$DISP" -screen 0 1280x800x24 -nolisten tcp >/dev/null 2>&1 & local XPID=$!
  sleep 2
  ( cd "$D" && DISPLAY=$DISP SDL_VIDEODRIVER=x11 timeout 30 ./"$b" -singleuser -nosound -nomidi -debug 3 "$@" \
      >/dev/null 2>"$OUT/$tag.log" ) & local APID=$!
  sleep 12
  shot "$DISP" "$tag"
  magick "$OUT/$tag.png" -crop 700x560+290+120 +repage "$OUT/$tag-win.png" 2>/dev/null
  local painted colours
  painted=$(magick "$OUT/$tag-win.png" -fill white +opaque black -format '%[fx:mean]' info: 2>/dev/null)
  colours=$(magick "$OUT/$tag-win.png" -format '%k' info: 2>/dev/null)
  # resize, then look again
  local WID; WID=$(DISPLAY=$DISP xdotool search --name "." 2>/dev/null | tail -1)
  [ -n "${WID:-}" ] && DISPLAY=$DISP xdotool windowsize "$WID" 1000 700 2>/dev/null
  sleep 3
  shot "$DISP" "$tag-resized"
  local painted2; painted2=$(magick "$OUT/$tag-resized.png" -fill white +opaque black -format '%[fx:mean]' info: 2>/dev/null)
  printf '  %-18s painted=%-10s colours=%-6s after-resize=%-10s world=%s errors=%s\n' \
     "$tag" "$painted" "$colours" "$painted2" \
     "$(grep -c 'Running world' "$OUT/$tag.log")" "$(grep -ciE '^ERROR' "$OUT/$tag.log")"
  grep -oiE "trying to load '[^']*'|Cannot load [^ ]*|No fonts found" "$OUT/$tag.log" | tail -1 | sed 's/^/       font: /'
  kill "$APID" 2>/dev/null; wait "$APID" 2>/dev/null
  kill "$XPID" 2>/dev/null; wait "$XPID" 2>/dev/null
}

echo "=== revised-7 from the extracted package"
run r7-32-nopak "$D7" simutrans-rgba32
run r7-16-nopak "$D7" simutrans-16bit
run r7-32-pak   "$D7" simutrans-rgba32 -load tutorial64.sve
run r7-16-pak   "$D7" simutrans-16bit  -load tutorial64.sve
