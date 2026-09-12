"""FIX-3: load a font in simgraph32_init(), as simgraph16_init() always has.

simgraph16_init() ends its display setup by loading a font, with three fallbacks
and a fatal notice if none of them works. simgraph32_init() never loaded one at
all, so in a 32-bit build the first font load happens much later, from the theme
code - by which time the working directory is the USER directory, not the game
directory, and the relative fallback "font/cyr.bdf" no longer resolves.

That is why Yona-TYT's log shows

    trying to load 'font/cyr.bdf' ... Cannot load font/cyr.bdf

while the 16-bit binary from the same archive, on the same machine and the same
file, gets as far as "Cannot set pixel size 11", which the loader recovers from
by picking the nearest fixed size. No font at all means panels and buttons draw -
they are boxes - and no text does.

Mirrors simgraph16_init() exactly, including the order (right after
dr_textur_init) and the fatal notice. Adds the one include it needs.
"""
import io
import os
import shutil
import sys

L = 'C:/Users/Victor/dev/_stlab-rgba32-fedora-01'
R5 = 'C:/Users/Victor/dev/_stlab-rgba32-blackwindow-01/cmp/r5'

OLD_INC = '#include "../simconst.h"\n#include "../sys/simsys.h"\n'
NEW_INC = '#include "../simconst.h"\n#include "../pathes.h"      // FONT_PATH_X, for the font fallback\n#include "../sys/simsys.h"\n'

OLD = """	textur = dr_textur_init();
	if(  textur == NULL  ) {
		dr_fatal_notify( "Cannot allocate 32 bit framebuffer!" );
		return false;
	}

	init_colour_tables();"""

NEW = """	textur = dr_textur_init();
	if(  textur == NULL  ) {
		dr_fatal_notify( "Cannot allocate 32 bit framebuffer!" );
		return false;
	}

	// init, load, and check fonts. simgraph16_init() has always done this here,
	// and doing it here is what matters: later on the working directory is the
	// user directory, where the relative fallback below cannot resolve. Without
	// it a 32 bit build can end up with no font at all - panels and buttons draw,
	// because they are boxes, and no text does.
	if (!gfx->load_font(env_t::fontname.c_str(), false)) {
		env_t::fontname = dr_get_system_font();
		if (!gfx->load_font(env_t::fontname.c_str(), false)) {
			env_t::fontname = FONT_PATH_X "cyr.bdf";
			if (!gfx->load_font(env_t::fontname.c_str(), false)) {
				dr_fatal_notify("No fonts found!");
				return false;
			}
		}
	}

	init_colour_tables();"""


def main():
    dst = os.path.join(L, 'src-fix3')
    if os.path.isdir(dst):
        shutil.rmtree(dst)
    shutil.copytree(R5, dst)
    for junk in ('src/simutrans/revision.h',
                 'src/android/AndroidAppSettings.cfg',
                 'src/linux/com.simutrans.Simutrans.metainfo.xml'):
        p = os.path.join(dst, junk.replace('/', os.sep))
        if os.path.exists(p):
            os.remove(p)
    f = os.path.join(dst, 'src', 'simutrans', 'display', 'simgraph32.cc')
    b = io.open(f, encoding='utf-8', newline='').read()
    for old, new in ((OLD_INC, NEW_INC), (OLD, NEW)):
        if b.count(old) != 1:
            raise SystemExit('anchor x%d: %r' % (b.count(old), old[:80]))
        b = b.replace(old, new, 1)
    io.open(f, 'w', encoding='utf-8', newline='').write(b)
    print('src-fix3 written from the revised-5 tree; simgraph32.cc only')


if __name__ == '__main__':
    main()
