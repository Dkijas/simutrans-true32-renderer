#!/bin/bash
# Which versioned glibc/libstdc++ symbols does the published binary demand?
set -u
W=$HOME/stlab/fedora-an
N=simutrans-rgba32-revised5-r12261-linux-x86_64-sdl2
rm -rf "$W"; mkdir -p "$W"
tar -xzf /mnt/c/Users/Victor/dev/_stlab-rgba32-fedora-01/pkg/$N.tar.gz -C "$W"
D=$W/$N
for b in simutrans-rgba32 simutrans-16bit; do
  echo "=== $b"
  echo "  highest GLIBC_ required : $(objdump -T "$D/$b" | grep -o 'GLIBC_[0-9.]*' | sort -V -u | tail -1)"
  echo "  all GLIBC_ versions     : $(objdump -T "$D/$b" | grep -o 'GLIBC_[0-9.]*' | sort -V -u | tr '\n' ' ')"
  echo "  GLIBCXX / CXXABI        : $(objdump -T "$D/$b" | grep -oE 'GLIBCXX_[0-9.]*|CXXABI_[0-9.]*' | sort -V -u | tr '\n' ' ')"
  echo "  which symbols need the highest:"
  hi=$(objdump -T "$D/$b" | grep -o 'GLIBC_[0-9.]*' | sort -V -u | tail -1)
  objdump -T "$D/$b" | grep "$hi" | sed 's/^/    /' | head -8
done
