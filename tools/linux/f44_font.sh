#!/bin/bash
# The tester's font situation, reproduced: what happens when fontconfig has
# nothing to offer, which is the only way dr_get_system_font() can return the
# relative "font/cyr.bdf" that his log shows.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-fedora-01
N=simutrans-rgba32-revised5-r12261-linux-x86_64-sdl2
W=/root/stlab
D=$W/$N

# a fontconfig configuration that knows about no font directories at all
cat > /root/empty-fonts.conf <<'XML'
<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "urn:fontconfig:fonts.dtd">
<fontconfig>
  <cachedir>/root/fccache</cachedir>
</fontconfig>
XML
mkdir -p /root/fccache

echo "=== with an empty fontconfig, what does fc-match say?"
FONTCONFIG_FILE=/root/empty-fonts.conf fc-match Sans 2>&1 | sed 's/^/  /'

run() {  # run <label> <cwd> <binary> <empty|normal>
  local label=$1 cwd=$2 b=$3 mode=$4
  local env=""
  [ "$mode" = empty ] && env="FONTCONFIG_FILE=/root/empty-fonts.conf"
  ( cd "$cwd" && env $env SDL_VIDEODRIVER=dummy timeout 15 "$D/$b" -singleuser -nosound -nomidi -debug 3 \
      >/dev/null 2>"$W/$label.log" )
  echo "  --- $label  ($b, fontconfig=$mode, cwd=$cwd)"
  grep -iE "trying to load|Cannot load|No fonts found|Cannot set pixel" "$W/$label.log" | head -6 | sed 's/^/     /'
  grep -c . "$W/$label.log" >/dev/null
}

echo "=== 32 bit and 16 bit, with fontconfig empty"
run empty-gamedir-32   "$D"    simutrans-rgba32 empty
run empty-gamedir-16   "$D"    simutrans-16bit  empty
run empty-elsewhere-32 "/root" simutrans-rgba32 empty
run empty-elsewhere-16 "/root" simutrans-16bit  empty

echo "=== does FreeType on this Fedora load the shipped BDF at all?"
echo "  freetype: $(rpm -q freetype 2>/dev/null)"
echo "  the file: $(ls -l "$D/font/cyr.bdf" | tr -s ' ' | cut -d' ' -f5,9)"
