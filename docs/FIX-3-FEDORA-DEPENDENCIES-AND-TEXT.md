# Fedora: the missing library and the missing text

Reported by **Yona-TYT** on the forum. Two separate problems, both reproduced on
a real Fedora, both solved, neither of them the tree flicker.

Reference package: `simutrans-rgba32-revised5-r12261-linux-x86_64-sdl2.tar.gz`,
sha256 `75da93d8…` — **downloaded from the release and hash-verified before any
of this**, with one extraction kept untouched as the control.

## Environment

The report does not state the Fedora version. It is **not** Fedora 43: measured
here, Fedora 43 carries glibc 2.42 and the published binary needs `GLIBC_2.43`,
so it would not have started at all. **Fedora 44** (glibc 2.43) runs it, so that
is what was used — a throwaway WSL2 `FedoraLinux-44`, x86_64, with the runtime a
desktop user would have (`SDL2 libpng bzip2-libs freetype fontconfig miniupnpc
libzstd dejavu-sans-fonts`). Ubuntu 26.04 was kept as the reference, never as a
substitute.

From his log: SDL2 with the **Wayland** driver, 1366×768, `-debug 3` and no
`-singleuser`, base directory `/home/yonatan/Descargas/simutrans-rgba32-revised5-…`.

## 1. `libbz2.so.1.0` — SONAME, not a missing package

Measured on Fedora 44:

```
requested by us : libbz2.so.1.0
on Fedora       : libbz2.so.1        SONAME=libbz2.so.1
                  libbz2.so.1.0.8    SONAME=libbz2.so.1
```

Debian and Ubuntu's bzip2 declares **`libbz2.so.1.0`**; Fedora's declares
**`libbz2.so.1`**. The file Yona has is the right library — it simply does not
answer to the name our binary asks for. His system is not missing anything.

It is also the **only** unresolved dependency: after reproducing his symlink
inside the throwaway box purely as a diagnostic, `ldd` reported **0 unresolved**
for both binaries. `libminiupnpc.so.21`, which looked like the next candidate, is
present on Fedora 44 with exactly that SONAME.

**Fix — packaging, no product code:** bzip2 is now linked **statically**
(`-DBZIP2_LIBRARY_RELEASE=…/libbz2.a`). The dependency disappears rather than
being satisfied by faking a SONAME on the user's system. bzip2's licence is
BSD-style and permits binary redistribution.

The symlink was created only inside the disposable Fedora, and **removed before
the final validation**.

## 2. The window with panel and buttons and no text

Not the initial-clip defect — revised-5 already contains that fix, and the
drawing is partial, not absent.

His log has the whole story in two lines:

```
Message: font_t::load_from_freetype: trying to load 'font/cyr.bdf' in size 11
Warning: font_t::load_from_freetype: Cannot load font/cyr.bdf
```

`font/cyr.bdf` is what `dr_get_system_font()` returns **when fontconfig finds no
match**, and it is a **relative** path. Reproduced exactly by giving fontconfig an
empty configuration, and the comparison between the two binaries in the same
archive is what identifies the defect:

| same machine, same file, fontconfig empty | result |
|---|---|
| `simutrans-rgba32` (32-bit) | `Cannot load font/cyr.bdf` — `FT_New_Face` failed, **no font at all** |
| `simutrans-16bit` (16-bit) | `Cannot set pixel size 11` — the face opened, the loader picked the nearest fixed size, **text works** |

**Cause.** `simgraph32_init()` never loaded a font. `simgraph16_init()` always
has, immediately after `dr_textur_init()`, with three fallbacks and
`dr_fatal_notify("No fonts found!")` if all of them fail. In a 32-bit build the
first font load therefore happens much later, from the theme code — by which time
the working directory is the **user** directory, where the relative
`font/cyr.bdf` does not resolve. With no font at all, the panel and the buttons
still draw, because they are boxes; the text does not. And nothing says so,
because the 32-bit path has no equivalent of the fatal notice.

This is the same family of defect as the black window: `simgraph32_init()` is
missing a step that `simgraph16_init()` has always had.

**Fix — FIX-3, `simgraph32.cc` only:** load the font where the 16-bit renderer
loads it, mirroring its three-step fallback and its error, plus the one include
(`pathes.h`) that `FONT_PATH_X` needs.

### Before and after, on Fedora 44

Ink inside the dialogue, and the screenshots themselves:

| fontconfig | binary | before | after |
|---|---|---|---|
| empty | 32-bit | **0.103423** — panel and buttons, no text | **0.299406** |
| empty | 16-bit | 0.299406 | 0.299406 (unchanged) |
| normal | 32-bit | 0.330709 | **0.350964** |
| normal | 16-bit | 0.350964 | 0.350964 (unchanged) |

After the fix the 32-bit build matches the 16-bit build **to the digit** in both
scenarios. `out/BEFORE/32-empty-win.png` is Yona's screenshot — the panel, two
fields and two buttons with no text; `out/AFTER/32-empty-win.png` is the same
dialogue with "Select one or more graphics to install (Ctrl+click):", the pakset
list and the Install / Cancel labels.

The 16-bit rows cannot move: the 16-bit build does not compile `simgraph32.cc`,
and indeed its binary comes out byte-identical to revised-5's.

## 3. A third thing, not reported but measured

The published Linux binaries require **`GLIBC_2.43`**, for float maths symbols
(`sqrtf`, `acosf`, `asinf`, `atan2f`, `log10f`, …) — a consequence of building on
Ubuntu 26.04. Fedora 44 is fine; **Fedora 43 and any older distribution cannot
start them at all**. That is stated in the package README rather than papered
over. Building on an older host (Debian 13 here carries glibc 2.41) would widen
it, and is left as a separate decision.

## Identity

- **candidate `RGBA32-CANDIDATE-REVISED-7`** = revised-6 + FIX-3, sha256
  `a488d13ec99a4d2409e4f36b028c862afb36303096862c2a3282ff14e95844cb`,
  26 files, +6767/−820.
  - r12261 + revised-7 == the tree the binaries were built from: **2164/2164,
    nothing differing**.
  - revised-7 against revised-6: **one file, `simgraph32.cc`**.
  - It carries revised-5's black-window fix and revised-6's SDL3 selection. It
    does **not** carry any earlier candidate's hash or verification.
- **FIX-3** alone: `b5bcad6f1c25263b4598938191184d882f17e1cbe2fc4fe2b5594e2f025156e6`.
- The **static bzip2** is a build setting, not a code change; it needs no
  candidate of its own.

## Package, not published

`simutrans-rgba32-revised7-r12261-linux-x86_64-sdl2.tar.gz`
sha256 `d024f57d2adc228a292c7b064e6c325f357f01987f4f3298fef8a1134651f85d`
· `simutrans-rgba32` `e5ba7c22…` · `simutrans-16bit` `71d71b56…`

### Final validation, from the extracted archive, symlink removed

On a plain Fedora 44:

| | unresolved libraries | starts |
|---|---|---|
| revised-5 (the reported build) | **1 — `libbz2.so.1.0`** | **no**, `rc=127` |
| revised-7, 32-bit | **0** | yes |
| revised-7, 16-bit | **0** | yes |

and from the package: text visible, the system font resolved, a pakset read, a
**copy** of a savegame loaded (`Running world`), resize working, 0 errors in all
four runs. The Ubuntu reference does not regress: 0 unresolved, no bz2 in
`NEEDED`, world loads, 0 errors, the same ink as revised-5 to four decimals.

Xvfb is a real X server but a **virtual** one. It reproduces the startup, the
font resolution and the drawing; it is not a Wayland session, and no claim is
made about Wayland or about any compositor. Yona's session was Wayland via SDL2.

## Not addressed

The tree flicker at the map edge. Untouched by this work and still unreproduced.
