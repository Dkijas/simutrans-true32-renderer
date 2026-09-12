#!/bin/bash
# Install, in the throwaway Fedora, exactly the runtime packages a tester would
# reasonably have: SDL2 and the rest of the package's direct dependencies, plus
# the few tools the diagnosis needs. Nothing here is a fix; it is the baseline
# that lets "wrong SONAME" be told apart from "package not installed".
set -u
DIST=$1
echo "=== $DIST"
cat /etc/fedora-release
echo "  glibc: $(ldd --version | head -1 | rev | cut -d' ' -f1 | rev)"
dnf -y -q install binutils gawk which findutils \
    SDL2 libpng bzip2-libs freetype fontconfig miniupnpc libzstd \
    mesa-dri-drivers xorg-x11-server-Xvfb xorg-x11-apps ImageMagick \
    >/dev/null 2>&1
echo "  install rc=$?"
for t in objdump readelf awk xwd convert Xvfb; do
  p=$(command -v "$t"); printf '  %-10s %s\n' "$t" "${p:-MISSING}"
done
echo "=== the libraries Fedora now provides, with their real SONAMEs"
for lib in libbz2 libminiupnpc libSDL2-2.0 libpng16 libfreetype libfontconfig libzstd; do
  for f in /usr/lib64/${lib}*; do
    [ -e "$f" ] || continue
    so=$(objdump -p "$f" 2>/dev/null | awk '/SONAME/{print $2}')
    printf '  %-40s %s\n' "$(basename "$f")" "${so:+SONAME=$so}"
  done
done
echo "=== which package provides each"
for f in /usr/lib64/libbz2.so.1 /usr/lib64/libminiupnpc.so* /usr/lib64/libSDL2-2.0.so.0; do
  [ -e "$f" ] || continue
  printf '  %-36s %s\n' "$(basename "$f")" "$(rpm -qf "$f" 2>/dev/null)"
done
