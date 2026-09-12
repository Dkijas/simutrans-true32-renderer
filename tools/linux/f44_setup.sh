#!/bin/bash
# Bring the throwaway Fedora up to what a desktop tester would have, one package
# at a time so a failure names itself instead of sinking the whole install.
set -u
echo "=== $(cat /etc/fedora-release)"
echo "  glibc: $(ldd --version | head -1)"
echo "  arch : $(uname -m)"

echo "=== installing the runtime a tester would already have"
for p in binutils gawk findutils which \
         SDL2 libpng bzip2-libs freetype fontconfig miniupnpc libzstd \
         dejavu-sans-fonts \
         xorg-x11-server-Xvfb xorg-x11-apps ImageMagick; do
  if dnf -y -q install "$p" >/dev/null 2>&1; then
    echo "  ok      $p"
  else
    echo "  FAILED  $p"
  fi
done

echo "=== tools"
for t in objdump readelf awk xwd Xvfb; do
  c=$(command -v "$t"); printf '  %-10s %s\n' "$t" "${c:-MISSING}"
done
echo "  convert/magick: $(command -v convert || command -v magick || echo MISSING)"

echo "=== the libraries, with the SONAME each one really declares"
for lib in libbz2 libminiupnpc libSDL2-2.0 libpng16 libfreetype libfontconfig libzstd; do
  found=0
  for f in /usr/lib64/${lib}*; do
    [ -e "$f" ] || continue
    found=1
    so=$(objdump -p "$f" 2>/dev/null | awk '/SONAME/{print $2}')
    printf '  %-34s %-28s %s\n' "$(basename "$f")" "${so:+SONAME=$so}" "$(rpm -qf "$f" 2>/dev/null | head -1)"
  done
  [ $found -eq 0 ] && echo "  $lib: NOT INSTALLED"
done
