#!/bin/bash
# Smoke the EXTRACTED archive, the way a tester would get it.
#
# Two display paths are tried, and they are not the same claim:
#   dummy  - SDL's headless video driver. Proves the renderer, the pakset load
#            and the game loop, with no X server involved.
#   Xvfb   - a real X server, virtual. Proves the SDL2 X11 path opens a window
#            and presents. It is NOT a test on anyone's desktop.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-linux-delivery-01
W=$HOME/stlab/rgba32-linux
NAME=simutrans-rgba32-revised4-r12261-linux-x86_64-sdl2
S=$W/smoke
LOG=$WIN/logs

rm -rf "$S"; mkdir -p "$S"
tar -xzf "$W/pkg/$NAME.tar.gz" -C "$S"
E=$S/$NAME
echo "extracted: $(find "$E" -type f | wc -l) files"
echo "exec bit : $(stat -c '%A %n' "$E/simutrans-rgba32")"
echo "hash ok  : $( (cd "$E" && sha256sum -c SHA256SUMS.txt 2>&1 | tr '\n' ' ') )"

# a pakset and a save, LOCAL to the smoke test, not part of the package
cp -r /mnt/c/Users/Victor/dev/_stlab-rgba32-normal-scene-and-platform-matrix-01/run/pak "$E/pak"
mkdir -p "$E/save"
cp /mnt/c/Users/Victor/dev/simutrans/simutrans/save/tutorial64.sve "$E/save/"

run_one() {
  local tag=$1 exe=$2 driver=$3 xr32=$4
  local out="$LOG/smoke-$tag"
  ( cd "$E" && \
    SDL_VIDEODRIVER=$driver SIMUTRANS_XR32=$xr32 \
    timeout -s TERM 40 ./"$exe" -singleuser -nosound -nomidi -debug 3 -load tutorial64.sve \
    > "$out.out" 2> "$out.err" )
  local rc=$?
  echo "--- $tag ($exe, driver=$driver, SIMUTRANS_XR32='${xr32}') rc=$rc"
  echo "    errors            : $(grep -ciE '^ERROR|Pixel format error' "$out.err")"
  echo "    SDL driver        : $(grep -m1 'SDL Driver' "$out.err" | sed 's/.*SDL Driver: //')"
  echo "    framebuffer       : $(grep -m1 'results in disp_width' "$out.err" | sed 's/.*\.\. //')"
  echo "    pakset read       : $(grep -c 'load_paks_from_directory' "$out.err")"
  echo "    entered the world : $(grep -c 'Running world' "$out.err")"
  echo "    XR32 nodes skipped: $(grep -c 'Skipping unknown XR32-node' "$out.err")"
}

run_one 32-dummy   simutrans-rgba32 dummy ''
run_one 16-dummy   simutrans-16bit  dummy ''
run_one 32-xr32    simutrans-rgba32 dummy '1'

if command -v xvfb-run >/dev/null; then
  out=$LOG/smoke-32-xvfb
  ( cd "$E" && \
    xvfb-run -a -s "-screen 0 1280x800x24" env SDL_VIDEODRIVER=x11 \
      timeout -s TERM 40 ./simutrans-rgba32 -singleuser -nosound -nomidi -debug 3 -load tutorial64.sve \
      > "$out.out" 2> "$out.err" )
  echo "--- 32-xvfb (real X server, VIRTUAL - not a desktop test) rc=$?"
  echo "    errors            : $(grep -ciE '^ERROR|Pixel format error' "$out.err")"
  echo "    SDL driver        : $(grep -m1 'SDL Driver' "$out.err" | sed 's/.*SDL Driver: //')"
  echo "    framebuffer       : $(grep -m1 'results in disp_width' "$out.err" | sed 's/.*\.\. //')"
  echo "    entered the world : $(grep -c 'Running world' "$out.err")"
fi
echo DONE
