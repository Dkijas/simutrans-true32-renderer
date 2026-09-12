"""Generate RGBA32-CANDIDATE-REVISED-5.diff = r12261 -> the tree with FIX-1.

Same format as revised-4: a plain unified diff with a/ and b/ labels and
/dev/null for added files, LF throughout. Revised-5 is revised-4 plus one
change; it does NOT inherit revised-4's hash, and it does not inherit the parts
of its certification that FIX-1 touches.
"""
import io
import os
import subprocess
import sys

L = 'C:/Users/Victor/dev/_stlab-rgba32-fedora-01'
BASE = 'C:/Users/Victor/dev/_stlab-rgba32-testing-release-01/tree-lf'
NEW = L + '/src-r7'
OUT = L + '/patches/RGBA32-CANDIDATE-REVISED-7.diff'

EMPTY = os.path.join(os.environ.get('TEMP', '.'), 'stlab-empty-file')
io.open(EMPTY, 'w').close()

GENERATED = {'src/simutrans/revision.h',
             'src/android/AndroidAppSettings.cfg',
             'src/linux/com.simutrans.Simutrans.metainfo.xml'}


def walk(root):
    out = set()
    for dp, dirs, fs in os.walk(root):
        dirs[:] = [d for d in dirs if d != '.git']
        for f in fs:
            rel = os.path.relpath(os.path.join(dp, f), root).replace(os.sep, '/')
            if rel not in GENERATED:
                out.add(rel)
    return out


def read(p):
    return io.open(p, 'rb').read().replace(b'\r\n', b'\n')


def main():
    a, b = walk(BASE), walk(NEW)
    chunks = []
    for rel in sorted(a | b):
        pa = os.path.join(BASE, rel.replace('/', os.sep))
        pb = os.path.join(NEW, rel.replace('/', os.sep))
        ina, inb = rel in a, rel in b
        if ina and inb and read(pa) == read(pb):
            continue
        if ina and not inb:
            raise SystemExit('file removed, not expected: ' + rel)
        la = '/dev/null' if not ina else 'a/' + rel
        src = EMPTY if not ina else pa
        r = subprocess.run(['diff', '-u', '--label', la, '--label', 'b/' + rel, src, pb],
                           capture_output=True, text=True)
        if r.returncode not in (0, 1):
            raise SystemExit('diff failed on ' + rel)
        if r.stdout:
            chunks.append(r.stdout.replace('\r\n', '\n'))
    io.open(OUT, 'w', encoding='utf-8', newline='').write(''.join(chunks))
    n = len(chunks)
    text = io.open(OUT, encoding='utf-8', newline='').read()
    add = sum(1 for l in text.split('\n') if l.startswith('+') and not l.startswith('+++'))
    rem = sum(1 for l in text.split('\n') if l.startswith('-') and not l.startswith('---'))
    print('RGBA32-CANDIDATE-REVISED-7.diff: %d files, +%d/-%d, %d bytes'
          % (n, add, rem, len(text.encode('utf-8'))))


if __name__ == '__main__':
    main()
