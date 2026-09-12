#!/bin/bash
# Before/after on Fedora 44, for the missing text.
#
#   f44_validate.sh <label> <32-bit binary> <16-bit binary>
#
# The tester's situation is reproduced by giving fontconfig nothing to offer,
# which is the only way dr_get_system_font() can return the relative
# "font/cyr.bdf" his log shows. Text is checked by looking at the screen, not by
# counting non-black pixels: the pakset dialogue is cropped and the number of
# distinct colours and the ink coverage inside it are reported, and the image is
# written out for inspection.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-fedora-01
N=simutrans-rgba32-revised5-r12261-linux-x86_64-sdl2
W=/root/stlab
LABEL=$1; B32=$2; B16=$3
OUT=$WIN/out/$LABEL
mkdir -p "$OUT"; rm -f "$OUT"/*.png "$OUT"/*.log

cat > /root/empty-fonts.conf <<'XML'
<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "urn:fontconfig:fonts.dtd">
<fontconfig><cachedir>/root/fccache</cachedir></fontconfig>
XML
mkdir -p /root/fccache

one() {  # one <tag> <binary> <empty|normal>
  local tag=$1 bin=$2 mode=$3
  local C=$W/v-$LABEL-$tag
  rm -rf "$C"; mkdir -p "$C"
  tar -xzf "$WIN/pkg/$N.tar.gz" -C "$C"
  local D=$C/$N
  cp "$bin" "$D/simutrans-test"; chmod 755 "$D/simutrans-test"

  local fcenv=""
  [ "$mode" = empty ] && fcenv="FONTCONFIG_FILE=/root/empty-fonts.conf"

  local DISP=:$(( 70 + RANDOM % 20 ))
  Xvfb "$DISP" -screen 0 1280x800x24 -nolisten tcp >/dev/null 2>&1 & local XPID=$!
  sleep 2
  ( cd "$D" && env $fcenv DISPLAY=$DISP SDL_VIDEODRIVER=x11 timeout 25 ./simutrans-test \
      -singleuser -nosound -nomidi -debug 3 >/dev/null 2>"$OUT/$tag.log" ) & local APID=$!
  sleep 12
  DISPLAY=$DISP magick import -window root "$OUT/$tag.png" 2>/dev/null
  # the dialogue sits in the middle of the window; crop generously
  magick "$OUT/$tag.png" -crop 700x560+290+120 +repage "$OUT/$tag-win.png" 2>/dev/null
  local colours ink
  colours=$(magick "$OUT/$tag-win.png" -format '%k' info: 2>/dev/null)
  # ink: how much of the dialogue is neither the panel grey nor pure black
  ink=$(magick "$OUT/$tag-win.png" -fill white +opaque black -format '%[fx:mean]' info: 2>/dev/null)
  printf '  %-14s font=%-7s colours=%-6s painted=%-10s %s\n' "$tag" "$mode" "$colours" "$ink" \
         "$(grep -oiE "Cannot load [^ ]*|Cannot set pixel size [0-9]+|No fonts found|trying to load '[^']*'" "$OUT/$tag.log" | tail -2 | tr '\n' ' ')"
  kill "$APID" 2>/dev/null; wait "$APID" 2>/dev/null
  kill "$XPID" 2>/dev/null; wait "$XPID" 2>/dev/null
}

echo "=== $LABEL on $(cat /etc/fedora-release)"
one 32-empty  "$B32" empty
one 16-empty  "$B16" empty
one 32-normal "$B32" normal
one 16-normal "$B16" normal
