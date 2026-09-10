#!/bin/bash
# The colour tables as the LINKER sees them, not as arithmetic claims.
# Sizes come out of the built 32-bit binary's symbol table.
set -u
W=$HOME/stlab/rgba32-linux
B=$W/build32/simutrans/simutrans
B16=$W/build16/simutrans/simutrans

show() {
  local bin=$1 label=$2
  echo "=== $label"
  local total=0
  for s in rgbmap_day_night rgbmap_all_day specialcolormap_day_night specialcolormap_all_day special_pal display_day_lights display_night_lights player_offsets; do
    line=$(nm -S --size-sort -C "$bin" 2>/dev/null | grep -w "$s" | head -1)
    if [ -n "$line" ]; then
      sz=$((16#$(echo "$line" | awk '{print $2}')))
      printf '  %-28s %8d bytes\n' "$s" "$sz"
      total=$((total + sz))
    else
      printf '  %-28s %s\n' "$s" "not a separate symbol"
    fi
  done
  printf '  %-28s %8d bytes = %s KiB\n' "TOTAL" "$total" "$(awk -v t=$total 'BEGIN{printf "%.1f", t/1024}')"
}

show "$B"   "32-bit build (COLOUR_DEPTH=32)"
show "$B16" "16-bit build (COLOUR_DEPTH=16)"

echo "=== the largest colour-indexed table, and what indexes it"
echo "  rgbmap entries = 0x8000 (32768 ordinary RGB555) + 16 player + 15 light + 1024 RGB343 = 33823"
echo "  a table indexed by the full 24-bit RGB space would need 16777216 entries."
echo "  grep for one:"
grep -rn "1 *<< *24\|0x1000000\|16777216" "$W/tree/src/simutrans/display/" | sed 's/^/    /' || echo "    no such table anywhere in display/"
