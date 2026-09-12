"""Diagnostic tree: instrument the SDL2 backend only.

One file, four questions, all of them about the first presentation:

  - is dr_flush() reached at all before the window is resized?
  - what rectangle does flush_framebuffer() ask dr_textur() to upload?
  - does SDL_UpdateTexture actually succeed?
  - and, decisively, is the framebuffer NON-BLACK in memory at that moment?

The last one separates "drawn but not presented" from "not drawn yet", which is
the fork the whole diagnosis turns on. Nothing in the renderer is touched.
"""
import io
import os
import shutil
import sys

L = 'C:/Users/Victor/dev/_stlab-rgba32-blackwindow-01'
SRC = 'C:/Users/Victor/dev/_stlab-rgba32-testing-release-01/tree-final'


def edit(path, pairs):
    b = io.open(path, encoding='utf-8', newline='').read()
    for old, new in pairs:
        n = b.count(old)
        if n != 1:
            raise SystemExit('ANCHOR x%d in %s:\n%r' % (n, os.path.basename(path), old[:120]))
        b = b.replace(old, new, 1)
    io.open(path, 'w', encoding='utf-8', newline='').write(b)


PROBE = r'''
/* ---- STLAB BLACK WINDOW PROBE (diagnostic build only) ------------------- */
static long stlab_flush_calls = 0;
static long stlab_textur_calls = 0;
static long stlab_textur_fail  = 0;

/// how many of the first N pixels of the framebuffer are not black
static long stlab_nonblack(void)
{
	if(  screen == NULL  ||  screen->pixels == NULL  ) {
		return -1;
	}
	long n = 0;
	const PIXVAL *p = (const PIXVAL *)screen->pixels;
	const long total = (long)screen->h * (long)(screen->pitch / (int)sizeof(PIXVAL));
	for(  long i = 0;  i < total;  i++  ) {
		if(  (p[i] & 0x00FFFFFFu) != 0  ) { n++; }
	}
	return n;
}

static void stlab_report(const char *where)
{
	dbg->message( "stlab_probe", "%s flush=%ld textur=%ld fail=%ld | textur=%p actual_w=%d h=%d | surf w=%d h=%d pitch=%d | nonblack=%ld",
	              where, stlab_flush_calls, stlab_textur_calls, stlab_textur_fail,
	              (void*)( screen ? screen->pixels : NULL ),
	              (int)gfx->get_screen_size().w, (int)gfx->get_screen_size().h,
	              screen ? screen->w : -1, screen ? screen->h : -1, screen ? screen->pitch : -1,
	              stlab_nonblack() );
}
/* ------------------------------------------------------------------------ */
'''


def main():
    dst = os.path.join(L, 'src-diag')
    if os.path.isdir(dst):
        shutil.rmtree(dst)
    shutil.copytree(SRC, dst)
    for junk in ('src/simutrans/revision.h',
                 'src/android/AndroidAppSettings.cfg',
                 'src/linux/com.simutrans.Simutrans.metainfo.xml'):
        p = os.path.join(dst, junk.replace('/', os.sep))
        if os.path.exists(p):
            os.remove(p)

    f = os.path.join(dst, 'src', 'simutrans', 'sys', 'simsys_s2.cc')
    edit(f, [
        # the probe goes right before dr_prepare_flush, where `screen` and gfx exist
        ('void dr_prepare_flush()\n{\n\treturn;\n}\n',
         PROBE + '\n\nvoid dr_prepare_flush()\n{\n\treturn;\n}\n'),

        ('void dr_flush()\n{\n\tgfx->flush_framebuffer();\n',
         'void dr_flush()\n{\n'
         '\tstlab_flush_calls++;\n'
         '\tif(  stlab_flush_calls <= 8  ||  stlab_flush_calls % 200 == 0  ) { stlab_report( "pre-flush" ); }\n'
         '\tgfx->flush_framebuffer();\n'
         '\tif(  stlab_flush_calls <= 8  ) { stlab_report( "post-flush" ); }\n'),

        ('\t\tSDL_UpdateTexture( screen_tx, &r, (uint8 *)screen->pixels + yp * screen->pitch + xp * sizeof(PIXVAL), screen->pitch );',
         '\t\tconst int stlab_rc = SDL_UpdateTexture( screen_tx, &r, (uint8 *)screen->pixels + yp * screen->pitch + xp * sizeof(PIXVAL), screen->pitch );\n'
         '\t\tstlab_textur_calls++;\n'
         '\t\tif(  stlab_rc != 0  ) { stlab_textur_fail++; }\n'
         '\t\tif(  stlab_textur_calls <= 8  ||  stlab_rc != 0  ) {\n'
         '\t\t\tdbg->message( "stlab_probe", "dr_textur #%ld rect=%d,%d %dx%d rc=%d %s",\n'
         '\t\t\t              stlab_textur_calls, r.x, r.y, r.w, r.h, stlab_rc, stlab_rc ? SDL_GetError() : "" );\n'
         '\t\t}'),
    ])
    print('src-diag written from tree-final, simsys_s2.cc instrumented')


if __name__ == '__main__':
    main()
