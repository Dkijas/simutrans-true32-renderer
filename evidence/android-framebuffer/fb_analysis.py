"""Analyse a raw framebuffer dump written by the lab capture in dr_flush().

Header: 8 x uint32 little-endian: magic 'SLAB', COLOUR_DEPTH, screen w, screen h,
fb_pitch (pixels), fb_height, sizeof(PIXVAL), present counter. Then
fb_pitch * fb_height words.

RGB565 round-trip = the game's own semantics: encode r>>3, g>>2, b>>3; expand
with (v * 0xFF) / 0x1F and (v * 0xFF) / 0x3F, exactly as simgraph16's colour
expansion does. A pixel is "RGB565-representable" when the round trip
returns the original RGB888.

  fb_analysis.py <dump.fb> [png-out]
"""
import struct
import sys
import hashlib


def expand565(word):
    r5, g6, b5 = (word >> 11) & 0x1F, (word >> 5) & 0x3F, word & 0x1F
    return (r5 * 0xFF // 0x1F, g6 * 0xFF // 0x3F, b5 * 0xFF // 0x1F)


def roundtrip(r, g, b):
    return expand565(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))


path = sys.argv[1]
data = open(path, 'rb').read()
magic, depth, w, h, pitch, fbh, psz, presents = struct.unpack('<8I', data[:32])
assert magic == 0x42414c53, 'bad magic'
body = data[32:]
n = pitch * fbh
assert len(body) == n * psz, ('size mismatch', len(body), n * psz)
print('  file %s  sha256 %s' % (path.split('/')[-1].split('\\')[-1], hashlib.sha256(data).hexdigest()[:16]))
print('  COLOUR_DEPTH=%d  screen %dx%d  pitch %d  fb_height %d  sizeof(PIXVAL)=%d  captured at present #%d' % (depth, w, h, pitch, fbh, psz, presents))
words = struct.unpack('<%d%s' % (n, 'I' if psz == 4 else 'H'), body)
rep = 0; tot = 0; examples = []; found_vec = 0; hist = {}
img = None
if len(sys.argv) > 2:
    from PIL import Image
    img = Image.new('RGB', (w, h))
    px = img.load()
for y in range(h):
    base = y * pitch
    for x in range(w):
        c = words[base + x]
        if psz == 4:
            r, g, b = (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF
        else:
            r, g, b = expand565(c)
        if img is not None:
            px[x, y] = (r, g, b)
        tot += 1
        rt = roundtrip(r, g, b)
        if rt == (r, g, b):
            rep += 1
        else:
            if len(examples) < 6 and (r, g, b) not in hist:
                examples.append(((r, g, b), rt))
            hist[(r, g, b)] = hist.get((r, g, b), 0) + 1
        if (r, g, b) == (18, 52, 86):
            found_vec += 1
out = tot - rep
print('  pixels analysed %d  RGB565-representable %d (%.2f%%)  NOT representable %d (%.2f%%)  distinct non-representable colours %d' % (
    tot, rep, 100.0 * rep / tot, out, 100.0 * out / tot, len(hist)))
print('  decisive vector RGB(18,52,86) = 0xFF123456: %d pixels; its RGB565 round-trip is %s' % (found_vec, '0xFF%02X%02X%02X' % roundtrip(18, 52, 86)))
for (rgb, rt) in examples:
    print('    example: framebuffer 0xFF%02X%02X%02X -> RGB565 round-trip 0xFF%02X%02X%02X' % (rgb + rt))
if img is not None:
    img.save(sys.argv[2])
    print('  derived PNG written:', sys.argv[2])
print('RESULT: %s' % ('CONTROL-OK (0 outside the RGB565 grid)' if depth == 16 and out == 0 else
                      'TRUE32-PROVEN (substantial population outside the RGB565 grid)' if depth == 32 and out > tot // 20 else
                      'FAIL (unexpected population for this depth)'))
