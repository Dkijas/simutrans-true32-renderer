#!/bin/bash
# What the two Linux binaries actually are, read from the binaries.
set -u
W=$HOME/stlab/rgba32-linux
for a in build32 build16; do
  b="$W/$a/simutrans/simutrans"
  echo "=== $a"
  file -b "$b" | sed 's/^/  /'
  echo "  sha256 $(sha256sum "$b" | cut -d' ' -f1)"
  echo "  size   $(stat -c %s "$b") bytes"
  echo "  renderer symbol: g_simgraph32=$(nm -C -D --defined-only "$b" 2>/dev/null | grep -c g_simgraph32)$(nm -C "$b" 2>/dev/null | grep -c g_simgraph32 | sed 's/^/ (static: /;s/$/)/')  g_simgraph16=$(nm -C "$b" 2>/dev/null | grep -c g_simgraph16)"
  echo "  shared libraries:"
  ldd "$b" | sed 's/^/    /'
  echo "  missing:"
  ldd "$b" | grep -i "not found" | sed 's/^/    /' || echo "    none"
done
