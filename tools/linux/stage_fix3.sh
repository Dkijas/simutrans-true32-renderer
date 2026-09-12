#!/bin/bash
set -u
O=/mnt/c/Users/Victor/dev/_stlab-rgba32-fedora-01/fix3bin
mkdir -p "$O"
cp "$HOME/stlab/fedorafix/b32/simutrans/simutrans" "$O/simutrans-rgba32"
cp "$HOME/stlab/fedorafix/b16/simutrans/simutrans" "$O/simutrans-16bit"
chmod 755 "$O"/*
ls -l "$O"
sha256sum "$O"/*
