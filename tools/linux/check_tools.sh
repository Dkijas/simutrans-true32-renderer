#!/bin/bash
# What is available for driving and capturing a real X session here.
for t in Xvfb xvfb-run xwd xwud convert magick import xdotool wmctrl x11vnc python3 ffmpeg; do
  p=$(command -v "$t")
  printf '  %-10s %s\n' "$t" "${p:-MISSING}"
done
echo "--- python imaging"
python3 -c "import PIL, PIL.Image; print('  PIL', PIL.__version__)" 2>/dev/null || echo "  PIL MISSING"
python3 -c "import numpy; print('  numpy', numpy.__version__)" 2>/dev/null || echo "  numpy MISSING"
echo "--- sudo without a tty"
sudo -n true 2>/dev/null && echo "  passwordless sudo: yes" || echo "  passwordless sudo: NO"
