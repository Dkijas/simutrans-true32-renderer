#!/bin/bash
# What Fedora actually says about the published package.
# Run as root inside the throwaway FedoraLinux-43 WSL distribution.
set -u
WIN=/mnt/c/Users/Victor/dev/_stlab-rgba32-fedora-01
NAME=simutrans-rgba32-revised5-r12261-linux-x86_64-sdl2
W=/root/stlab

echo "=== the environment"
cat /etc/fedora-release 2>/dev/null
uname -srm
echo "  glibc: $(ldd --version | head -1)"

echo "=== tools needed for the diagnosis"
for t in file ldd objdump readelf rpm dnf; do
  p=$(command -v "$t"); printf '  %-10s %s\n' "$t" "${p:-MISSING}"
done

rm -rf "$W"; mkdir -p "$W"
tar -xzf "$WIN/pkg/$NAME.tar.gz" -C "$W"
D=$W/$NAME
echo "=== extracted $(find "$D" -type f | wc -l) files"

echo "=== what the binaries ask the system for (DT_NEEDED)"
for b in simutrans-rgba32 simutrans-16bit; do
  echo "  --- $b"
  if command -v objdump >/dev/null; then
    objdump -p "$D/$b" | awk '/NEEDED/{print "     " $2}'
  else
    readelf -d "$D/$b" 2>/dev/null | awk '/NEEDED/{gsub(/[][]/,"",$5); print "     " $5}'
  fi
done

echo "=== what Fedora can and cannot resolve"
for b in simutrans-rgba32 simutrans-16bit; do
  echo "  --- $b"
  ldd "$D/$b" 2>&1 | grep -i "not found" | sed 's/^/     MISSING /' || true
  n=$(ldd "$D/$b" 2>&1 | grep -ci "not found")
  echo "     missing count: $n"
done

echo "=== does it run at all?"
( cd "$D" && ./simutrans-rgba32 -singleuser -nosound -nomidi 2>&1 | head -3 | sed 's/^/     /' )

echo "=== what Fedora ships for the libraries in question"
for lib in libbz2 libminiupnpc libSDL2-2.0 libpng16 libfreetype libfontconfig libzstd; do
  echo "  --- $lib"
  ls -l /usr/lib64/${lib}* 2>/dev/null | sed 's/^/     /' || echo "     none installed"
done
