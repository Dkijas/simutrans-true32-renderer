"""FIX-1: the 32-bit renderer never set an initial clip rectangle.

clips32[] is a file-scope static, so it starts zeroed: clip_rect.xx and .yy are 0
and every drawing operation is clipped away. simgraph16_init() has always ended
with a set_clip_rect over the whole screen; simgraph32_init() did not, so until
something else set a clip - which loading a pakset does, and the pakset-selection
dialogue does not - nothing reached the framebuffer at all.

That is why the window is black with no error, and why resizing it fixes it:
on_window_resized() sets the clip rectangle.

One line, mirroring simgraph16_init() including its use of disp_width.
"""
import io
import os
import shutil
import sys

L = 'C:/Users/Victor/dev/_stlab-rgba32-blackwindow-01'
SRC = 'C:/Users/Victor/dev/_stlab-rgba32-testing-release-01/tree-final'

OLD = """	init_colour_tables();
	dirty_state_alloc();

	return true;
}

static bool simgraph32_is_display_init()"""

NEW = """	init_colour_tables();
	dirty_state_alloc();

	// The clipping state is a zeroed static, so without this every drawing
	// operation is clipped against an empty rectangle and nothing reaches the
	// framebuffer. simgraph16_init() has always ended with this line; leaving
	// it out here made the first window black until something else set a clip.
	simgraph32_set_clip_rect( 0, 0, disp_width, disp_height  CLIP_NUM_DEFAULT, false );

	return true;
}

static bool simgraph32_is_display_init()"""


def build(target, apply_fix=True):
    dst = os.path.join(L, target)
    if os.path.isdir(dst):
        shutil.rmtree(dst)
    shutil.copytree(SRC, dst)
    for junk in ('src/simutrans/revision.h',
                 'src/android/AndroidAppSettings.cfg',
                 'src/linux/com.simutrans.Simutrans.metainfo.xml'):
        p = os.path.join(dst, junk.replace('/', os.sep))
        if os.path.exists(p):
            os.remove(p)
    if apply_fix:
        f = os.path.join(dst, 'src', 'simutrans', 'display', 'simgraph32.cc')
        b = io.open(f, encoding='utf-8', newline='').read()
        if b.count(OLD) != 1:
            raise SystemExit('anchor x%d' % b.count(OLD))
        io.open(f, 'w', encoding='utf-8', newline='').write(b.replace(OLD, NEW, 1))
    print('%s written%s' % (target, ' with FIX-1' if apply_fix else ' (unfixed control)'))


if __name__ == '__main__':
    build(sys.argv[1], sys.argv[2] != 'control' if len(sys.argv) > 2 else True)
