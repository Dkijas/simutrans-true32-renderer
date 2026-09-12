#!/bin/bash
# The two reported problems, on Fedora 44 (glibc 2.43, the version that can run
# the published binary at all).
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-fedora-01
N=simutrans-rgba32-revised5-r12261-linux-x86_64-sdl2
W=/root/stlab
rm -rf "$W"; mkdir -p "$W"
tar -xzf "$WIN/pkg/$N.tar.gz" -C "$W"
D=$W/$N

echo "=== 1. DEPENDENCIES, with nothing linked by hand"
for b in simutrans-rgba32 simutrans-16bit; do
  echo "  --- $b"
  ldd "$D/$b" 2>&1 | grep -iE "not found|GLIBC" | sed 's/^/     /'
  echo "     unresolved: $(ldd "$D/$b" 2>&1 | grep -ci 'not found')"
done
echo "  what the binary asks for vs what Fedora declares:"
for lib in libbz2 libminiupnpc; do
  echo "   $lib:"
  echo "     requested by us: $(objdump -p "$D/simutrans-rgba32" | awk '/NEEDED/{print $2}' | grep -i "$lib")"
  for f in /usr/lib64/${lib}*; do
    [ -e "$f" ] || continue
    printf '     on Fedora     : %-28s %s\n' "$(basename "$f")" "$(objdump -p "$f" 2>/dev/null | awk '/SONAME/{print "SONAME="$2}')"
  done
done

echo "=== 2. the first error a tester sees"
( cd "$D" && ./simutrans-rgba32 -singleuser 2>&1 | head -2 | sed 's/^/     /' )

echo "=== 3. reproduce the tester's manual symlink, DIAGNOSIS ONLY, inside this throwaway box"
ln -sf /usr/lib64/libbz2.so.1.0.8 /usr/lib64/libbz2.so.1.0
ls -l /usr/lib64/libbz2.so.1.0 | sed 's/^/     /'
for b in simutrans-rgba32 simutrans-16bit; do
  echo "  --- $b after the symlink: unresolved=$(ldd "$D/$b" 2>&1 | grep -ci 'not found')"
  ldd "$D/$b" 2>&1 | grep -i "not found" | sed 's/^/     still MISSING /'
done

echo "=== 4. what fontconfig answers on this Fedora"
command -v fc-match >/dev/null && fc-match Sans 2>&1 | sed 's/^/     /' || echo "     fc-match not installed"
echo "     fonts present: $(ls /usr/share/fonts 2>/dev/null | tr '\n' ' ')"

echo "=== 5. the font, from the game directory and from somewhere else"
run() {  # run <label> <cwd> <binary>
  local label=$1 cwd=$2 b=$3
  ( cd "$cwd" && SDL_VIDEODRIVER=dummy timeout 15 "$D/$b" -singleuser -nosound -nomidi -debug 3 \
      >/dev/null 2>"$W/$label.log" )
  echo "  --- $label ($b, cwd=$cwd)"
  grep -iE "trying to load|Cannot load|No fonts|themes.tab" "$W/$label.log" | head -5 | sed 's/^/     /'
  [ -s "$W/$label.log" ] || echo "     (no log)"
}
run gamedir-32   "$D"   simutrans-rgba32
run gamedir-16   "$D"   simutrans-16bit
run elsewhere-32 "/root" simutrans-rgba32
run elsewhere-16 "/root" simutrans-16bit
