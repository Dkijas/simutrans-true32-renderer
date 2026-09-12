#!/bin/bash
for d in "$HOME/stlab/rgba32-linux/build32" "$HOME/stlab/blackwindow/fix1/build32" "$HOME/stlab/winsdl/sdl3-32"; do
  echo "=== $d"
  echo -n "  USE_FONTCONFIG in flags: "; grep -o 'USE_FONTCONFIG[^ ]*' "$d/CMakeFiles/simutrans.dir/flags.make" 2>/dev/null | sort -u | tr '\n' ' '; echo
  echo -n "  cache SIMUTRANS_USE_FONTCONFIG: "; grep -i '^SIMUTRANS_USE_FONTCONFIG' "$d/CMakeCache.txt" 2>/dev/null; echo
  echo -n "  FONT_PATH_X: "; grep -o 'FONT_PATH_X=[^ ]*' "$d/CMakeFiles/simutrans.dir/flags.make" 2>/dev/null | sort -u | tr '\n' ' '; echo
  b="$d/simutrans/simutrans"
  [ -f "$b" ] && echo "  links fontconfig: $(objdump -p "$b" | grep -c fontconfig)"
done
