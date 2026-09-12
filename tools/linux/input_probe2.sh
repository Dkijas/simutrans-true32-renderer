#!/bin/bash
set -u
echo "=== window managers available"
for m in twm openbox matchbox-window-manager metacity mutter i3 icewm fluxbox jwm blackbox marco xfwm4; do
  p=$(command -v "$m"); printf '  %-26s %s\n' "$m" "${p:-MISSING}"
done
