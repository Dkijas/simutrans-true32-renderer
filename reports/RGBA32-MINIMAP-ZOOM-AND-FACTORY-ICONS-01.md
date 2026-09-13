# STLAB-RGBA32-MINIMAP-ZOOM-AND-FACTORY-ICONS-01

    MISSION:        STLAB-RGBA32-MINIMAP-ZOOM-AND-FACTORY-ICONS-01
    MODE:           /simufix
    BASE:           RGBA32-CANDIDATE-REVISED-7 on official SVN trunk r12261
                    = git ecca332dca25025833f5ce6641491f8886a3ef9f
    LANE:           C:\Users\Victor\dev\_stlab-rgba32-minimap-icons-01   (dedicated)
    BUILD ROOT:     WSL Ubuntu 26.04, GCC 15.2.0, CMake, SDL2, /root/mm
    OBJECTIVE:      the two symptoms Yona-TYT reported on revised-7
    ALLOWED WRITES: this lane and a local branch; no publication
    PUBLICATION:    NOT AUTHORISED
    REQUIRED GATES: negative control per defect + a 16-bit reference arm
    STOP CONDITION: READY_FOR_APPROVAL

---

## 1. What was reported, and what it actually is

Yona-TYT, message #25 of forum topic 24081, on the revised-7 Linux package:

> Perfect, I can now start Simutrans without any problems.
> There is a bug when I use zoom out / in in the minimap
> Some icons are misaligned in the factory window

He attached a 7.3 s screen recording (1366x768, 178 frames) and one screenshot.
Both were downloaded and examined frame by frame; nothing below is inferred
from his wording.

His confirmation covers **starting the program**. It is not a statement about
the renderer as a whole and is not treated as one.

| # | symptom | verdict | where |
|---|---|---|---|
| D1 | the minimap paints over the toolbar and the map view when zoomed | **REAL** | `simgraph32_draw_array` |
| D2 | icons misplaced in the factory window | **REAL** | `simgraph32_get_base_image_offset`, `simgraph32_get_image_offset` |

Both are defects of the 32-bit renderer alone. A 16-bit build of the very same
tree has neither. They are **two independent defects with two independent
causes** - they are not one bug seen twice, and the fixes are separate.

---

## 2. D1 - the minimap escapes its window

### 2.1 What the recording shows

`evidence/minimap-zoom.mp4`, extracted to 178 frames with their timestamps
(`out/frame-index.csv`). Measured on the tool icon row of his own window
(y 88..116, 1366 px wide = 38 248 px), each frame against the first:

| | |
|---|---|
| frames in which more than 2 % of the tool icon row is repainted | **53 of 178** |
| worst frame | **55.2 %** (s035, t = 1.435 s) |
| resting value between zoom steps | 2.65 % |

`out/w018.png` and `out/w019.png` are two consecutive frames: at 2:1 the minimap
is inside its window; one zoom step later the map is painted across the toolbar,
the window title bar and the desktop, while the window's own client area is
left empty.

### 2.2 The cause, in the code

`minimap_t::draw()` hands the rendered minimap to the renderer in one call:

```
src/simutrans/gui/minimap.cc:1338
    gfx->draw_array( cur_off.x+pos.x, new_off.y+pos.y,
                     map_data->get_width(), map_data->get_height(),
                     map_data->to_array() );
```

The 16-bit renderer clips that array against the **active clip rectangle**:

```
src/simutrans/display/simgraph16.cc:3136  simgraph16_draw_array
    const scr_coord_val xoff = clip_wh( &xp, &w, CR0.clip_rect.x, CR0.clip_rect.xx );
    const scr_coord_val yoff = clip_wh( &yp, &h, CR0.clip_rect.y, CR0.clip_rect.yy );
```

The 32-bit renderer clips it against **the screen**:

```
src/simutrans/display/simgraph32.cc:2396  simgraph32_draw_array
    if(  yp + y < 0  ||  yp + y >= disp_height  )     continue;
    if(  xp + x >= 0  &&  xp + x < disp_actual_width  )  p[x] = ...
```

So whenever the array is handed over at a position that is not entirely inside
the minimap's window, the 32-bit renderer paints it on the screen anyway.

That happens on the frame after every zoom step once the map is bigger than the
window. `map_frame_t::draw()` first tells the minimap which part of the map it
is showing, and only then re-centres the view by moving the scrollbars:

```
src/simutrans/gui/map_frame.cc:701
    minimap_t::get_instance()->set_xy_offset_size(
        scr_coord(scrolly.get_scroll_x(), scrolly.get_scroll_y()),
        scrolly.get_client().get_size() );
    ...
src/simutrans/gui/map_frame.cc:710
    if(zoomed  ||  ...) {
        scrolly.set_scroll_position( max(0,center.x-(size.w/2)),
                                     max(0,center.y-(size.h/2)) );
```

In that one frame the offset the minimap was given and the position the
scrollpane then draws it at differ by the re-centring jump, so the array is
handed over displaced by exactly that much. **The displacement is in trunk's own
GUI code and is harmless there**, because the 16-bit renderer clips it away: at
worst a band of the minimap is missing for one frame. It is only the missing
clip in the 32-bit renderer that turns it into paint across the screen.

This is also why it flashes rather than sticks: every window redraws on the next
frame and paints over the damage.

---

## 3. D2 - the icons in the factory window

### 3.1 What the screenshot shows

`evidence/factory-window.png`: in the "Los trabajadores viven en:" row the
passenger symbol sits low and crowds the number next to it.

The same row was reproduced here, in both renderers, from the same savegame and
pakset, and measured rather than eyeballed (`tools/iconpos.py`, the coloured
runs on the row; `out/fi16-workers.png` and `out/fi32-workers.png`):

| element on the row | 16-bit renderer | 32-bit candidate | displacement |
|---|---|---|---|
| position button | x 300..305, y 458..467 | x 300..305, y 458..467 | none |
| "32" | x 373..384 | x 377..388 | **+4 x** |
| passenger symbol | x 391..398, y 460..466 | x 395..402, y 464..470 | **+4 x, +4 y** |
| "12" | x 415..426 | x 414..425 | **-1 x** |
| mail symbol | x 432..439, y 461..465 | x 432..439, y 465..469 | **+4 y** |

The number of coloured pixels in each symbol is identical (25 / 7 / 12 / 40), so
the same images are being drawn - only in the wrong place.

### 3.2 The cause, in the code

The window builds that row as a six column table and asks each image how much
room it needs:

```
src/simutrans/gui/fabrik_info.cc:376
    all_cities.new_component<gui_image_t>(
        skinverwaltung_t::passengers->get_image_id(0))->enable_offset_removal(true);

src/simutrans/gui/components/gui_image.cc:23  gui_image_t::get_min_size()
    scr_rect r = gfx->get_base_image_offset(id);
    if (remove_enabled) return scr_size(r.w, r.h);
```

In the 32-bit renderer both offset queries are the **headless renderer's stubs**,
copied from `simgraph0.cc` and never implemented:

```
src/simutrans/display/simgraph32.cc:2066
    static scr_rect simgraph32_get_image_offset(image_id)      { return scr_rect{ 0, 0, 0, 0 }; }
    static scr_rect simgraph32_get_base_image_offset(image_id) { return scr_rect{ 0, 0, 0, 0 }; }
```

The data they should return is in the renderer's own image table and is filled
in when the image is registered (`imd32::base_x/base_y/base_w/base_h`,
`simgraph32.cc:279` and `:1944`). Nothing was missing except the two functions.

The consequence is twofold, and both halves are visible on the row:
`get_min_size()` returns (0,0) so the icon columns have no width, and
`set_size()` computes `remove_offset = (0,0)` instead of `(-r.x,-r.y)` so each
image is drawn displaced by its own offset. The measured +4/+4 and 0/+4 are the
passenger and mail symbols' own offsets.

### 3.3 Measured directly, without a window

A probe build prints what the GUI is told about every image the pakset
registered, right after the pakset loads (`tools/make_probe.py`, one `printf` in
`simmain.cc`, nothing else touched):

| arm | images | base rectangle equal to the 16-bit renderer's | empty {0,0,0,0} |
|---|---|---|---|
| 16-bit reference | 6962 | - | 0 |
| **32-bit candidate** | 6962 | **0 of 6962** | **6962** |

### 3.4 This is not only the factory window

`get_base_image_offset` and `get_image_offset` are asked by fourteen call sites.
Besides `gui_image_t`, which is used by twenty-two windows, they are read by
`gui_button`, `gui_convoiinfo`, `gui_flowtext`, `gui_image_list`,
`vehiclelist_frame`, and by `display_settings.cc`, which sizes GUI elements from
a tool icon.

Two of them are not GUI code at all:

* `src/simutrans/obj/simobj.cc:320` decides whether an object near the edge of
  the map needs the background marked dirty, from `r.x/r.y/r.w/r.h`.
* `src/simutrans/world/simplan.cc:525` measures how far a **tree** rises above
  its tile, from `area.y`, to decide what to clip against the tile above.

Both therefore behave differently in the 32-bit build. **This is recorded as a
lead for the separate tree flicker investigation and nothing more**: no claim is
made here that it is that defect's cause, and it was not tested against it. The
flicker has never been reproduced in this laboratory and stays open on its own
terms.

---

## 4. The two fixes

Both are in `simgraph32.cc` and nothing else in the candidate changes. Each one
restores what the 16-bit renderer has always done; neither invents behaviour.

**FIX-4** - `simgraph32_draw_array` clips against the active clip rectangle:

```c
const scr_coord_val arr_w = w;
const scr_coord_val xoff  = clip_wh32( &xp, &w, CR32_0.clip_rect.x, CR32_0.clip_rect.xx );
const scr_coord_val yoff  = clip_wh32( &yp, &h, CR32_0.clip_rect.y, CR32_0.clip_rect.yy );
if(  w <= 0  ||  h <= 0  ) return;
...
if(  xp == CR32_0.clip_rect.x  ) arr_src += xoff;
if(  yp == CR32_0.clip_rect.y  ) arr_src += (size_t)yoff * arr_w;
```

`clip_wh32` is the 32-bit renderer's own copy of `clip_wh` and behaves
identically; `CR32_0` is its `CR0`. Five callers reach this entry point -
`minimap.cc`, `city_info.cc` twice, `gui_map_preview.h` and `server_frame.cc` -
and all five draw a bitmap inside a window and have always relied on the clip.

**FIX-5** - the two offset queries answer from the renderer's own image table
instead of returning an empty rectangle, exactly as `simgraph16` does:

```c
static scr_rect simgraph32_get_image_offset(image_id image)
{
    if( image < anz_images ) return scr_rect{ images[image].x, images[image].y,
                                              images[image].w, images[image].h };
    else                     return scr_rect{ 0, 0, 0, 0 };
}
// and the same for base_x / base_y / base_w / base_h
```

### A text audit of the whole dispatch table

To check that `draw_array` was the only hole of its kind, every slot of the
renderer dispatch table was compared: does the 16-bit implementation consult a
clip rectangle, and does the 32-bit one (`tools/clipaudit.py`)? Seven slots came
back flagged; six of them delegate the clipping to a helper that does consult it
(`clip_rows32`, `display_img_aux32`, `display_img_blend_wc32`,
`display_fb_internal32`). **`draw_array` was the only one that really did not
clip.**

---

## 5. D2 measured across every arm

The same savegame, the same pakset, the same clicks, the same screen: open the
factory list, open the first factory, photograph the "Workers live in" row and
measure where the coloured runs are (`tools/repro.sh`, `tools/iconpos.py`).

| arm | build | passenger symbol | mail symbol | verdict |
|---|---|---|---|---|
| 16-bit reference | `b16` | x 391..398, y 460..466 | y 461..465 | reference |
| **revised-7, 32-bit** | `b32` | x 395..402, y 464..470 | y 465..469 | **wrong** |
| revised-8, both fixes | `fix-b32` | x 391..398, y 460..466 | y 461..465 | correct |
| control: FIX-5 only | `ctl-no4-b32` | x 391..398, y 460..466 | y 461..465 | correct |
| control: FIX-4 only | `ctl-no5-b32` | x 395..402, y 464..470 | y 465..469 | **wrong** |
| mutant m5: 16-bit given the 32-bit stubs | `mut-m5-b16` | x 395..402, y 464..470 | y 465..469 | **wrong** |
| mutant m4: 16-bit given the 32-bit clipping | `mut-m4-b16` | x 391..398, y 460..466 | y 461..465 | correct |

Three things follow, and each is a separate claim:

1. **The fix works.** With both fixes the placement is identical to the 16-bit
   renderer's, to the pixel.
2. **It is FIX-5 that fixes it, and FIX-4 has nothing to do with it.** The arm
   carrying only FIX-4 still fails, by the same numbers.
3. **The check can fail.** The mutant that puts the 32-bit stubs into the
   otherwise correct 16-bit renderer fails the same check with exactly the same
   coordinates - so what is being measured is these two functions and not
   something adjacent to them.

The probe repeats the same conclusion without a window at all:

| arm | images | base rect equal to the 16-bit renderer's | empty {0,0,0,0} |
|---|---|---|---|
| 16-bit reference | 6962 | - | 0 |
| revised-7, 32-bit | 6962 | 0 of 6962 | 6962 |

---

## 6. D1 measured across every arm

### 6.1 The harness

A screenshot cannot see this defect: it is a flash of one frame per zoom step and
the next frame paints over it. So every arm is **recorded** instead, at 60 fps,
and every recorded frame is looked at.

The world is paused from the command line (`-pause`, confirmed in each run's log
by `Running world, pause=1`), the map window is left at its default size and
place, and the wheel is turned over the middle of the map: ten times three steps
in and three out, then two full sweeps of eleven in and eleven out. Sixty-six
zoom steps, about ninety seconds, five thousand four hundred frames.

The measurement is the **top 90 rows of the screen**, against the first recorded
frame. The map window's own title bar starts at y=94 in every arm - checked, not
assumed, with `tools/wincheck.py` - so those rows are outside the window with
four rows to spare, and with the world paused nothing in them has any reason to
change.

### 6.2 The result

| arm | build | frames repainted above the window | worst frame |
|---|---|---|---|
| **revised-7, 32-bit** | `b32` | **31** of 5386 | **28.10 %** |
| 16-bit reference | `b16` | 0 of 5416 | 0.00 % |
| revised-8, both fixes | `fix-b32` | 0 | 0.00 % |
| control: FIX-5 only, minimap fix left out | `ctl-no4-b32` | **25** | **28.10 %** |
| control: FIX-4 only | `ctl-no5-b32` | 0 | 0.00 % |
| mutant m4: 16-bit given the 32-bit clipping | `mut-m4-b16` | **31** | **28.11 %** |

`out/film-cand32/worst-frame.png` is the same picture Yona filmed: the minimap
painted across the toolbar above its own window, while the window's client area
is left empty.

The three claims, each standing on its own row:

1. **The fix works.** With both fixes, not one frame in 5400 repaints anything
   above the window - the same zero the 16-bit renderer gives.
2. **It is FIX-4 that fixes it.** The arm carrying only FIX-5 still fails, with
   the same worst frame; the arm carrying only FIX-4 passes.
3. **The check can fail.** The mutant that takes the **16-bit** renderer - which
   is otherwise clean, at exactly 0 - and gives its `draw_array` the 32-bit
   clipping fails with 31 frames and a worst frame of 28.11 %: the candidate's
   own numbers. What is being measured is that clip and nothing else.

### 6.3 What the fixes do not change

The main map view was compared between the candidate and the fixed build on the
paused frame after loading the same savegame, everything outside the map window
and the status bar: **0 pixels differ, at any threshold.** The same holds for the
arm carrying only FIX-4.

That is a statement about this scene at this zoom and this map position, not a
general one. It says the fixes did not disturb the view that was looked at; it
does not prove the tree-clipping path is unaffected everywhere.

---

## 7. Provenance

Everything was built from the same sources with the same options, on one host:

```
Ubuntu 26.04 LTS, Linux 6.18.33.2-microsoft-standard-WSL2 x86_64 (WSL)
gcc 15.2.0   cmake 4.2.3   glibc 2.43   SDL2 2.32.10
cmake -DCMAKE_BUILD_TYPE=Release -DSIMUTRANS_BACKEND=sdl2
      -DSIMUTRANS_MULTI_THREAD=ON -DSIMUTRANS_USE_REVISION=12261
      -DSIMUTRANS_TRUE32=<ON for a 32-bit arm, OFF for a 16-bit one>
```

| arm | tree | depth | binary sha256 |
|---|---|---|---|
| candidate revised-7 | `tree` | 32 | `caa6d0652627e7adaf27db0df8e37d51d83a52b98eff7e34202dfe55f2a62fa4` |
| 16-bit reference | `tree` | 16 | `4d698d6c8d7ed2b44d19f6825fc8b9c408040f0e76e2add4bca8cfb7cca15a54` |
| revised-8, both fixes | `fix` | 32 | `8db45405beac69f9527c38dc86c7f2f14db778e1b36cd0c92237b1e2b00544ce` |
| revised-8, 16-bit | `fix` | 16 | `4d698d6c8d7ed2b44d19f6825fc8b9c408040f0e76e2add4bca8cfb7cca15a54` |
| control, FIX-5 only | `ctl-no4` | 32 | `4700927c35a17f1d957d07f5b6e71926d62b003b06ef78b613152b888ce6c694` |
| control, FIX-4 only | `ctl-no5` | 32 | `41ed4891e80cbd004a88ba44c2087a00175926051cd0cbcf6765db4917f664e3` |
| mutant m4, 16-bit | `mut-m4` | 16 | `027aa039358e900fc395b906f2f227cf85be31760a0817ffbe2326349fe67a27` |
| mutant m5, 16-bit | `mut-m5` | 16 | `91c9870eaaa2ec478843f9426aa3e8b7f689f9ac8312f0d40e41bdcb37778d73` |

Note the third and fourth rows: **the 16-bit binary is byte-identical with and
without the fixes.** Both changes are inside `simgraph32.cc`, which a 16-bit
build does not compile, so the reference arm cannot have been moved by them.

Warning sets were compared, not warning counts: 7 warnings, 6 distinct, the same
six strings in the candidate and in the fixed build.

**Test inputs**

```
pakset    pak64, 818 files, 17 MB
          .../_stlab-rgba32-normal-scene-and-platform-matrix-01/run/pak
savegame  tutorial64.sve, 304 734 bytes
          sha256 2fdc17154064386ad76d9a8cc3bfc2ca35183b5d440bcf2af030730e1ea16158
screen    Xvfb 1366x768x24 for the minimap, 1280x800x24 for the factory window
```

**The patch base**

The Windows-side r12261 export in the review lane is stored with CRLF endings,
so the published revised-7 patch does not apply to it and it was not used. The
base was reconstructed instead by reverse-applying the published revised-7 patch
to the revised-7 sources, and then checked by applying it forward again: the
result is identical to the sources it came from, with no rejects in either
direction. `RGBA32-CANDIDATE-REVISED-8.diff` is made against that base, and
applying it to the base reproduces the fixed tree exactly.

| patch | files | lines | sha256 |
|---|---|---|---|
| `FIX-4-simgraph32-draw-array-clip.diff` | 1 | +20 / -6 | `6ce28be3975a2bff4b9495a53ba665531dbf7b5c84b74b186079efd5181f1291` |
| `FIX-5-simgraph32-image-offsets.diff` | 1 | +35 / -9 | `3c7ce45c7d6907c9e8ed65a628c5fa5c6e3c52be22a92498d379459f29fbc890` |
| `RGBA32-CANDIDATE-REVISED-8.diff` | 26 (15 modified, 11 added) | +6823 / -836 | `bdc0a3262b0b8168d148d643ce418256fdf08254128934883662db7794bc64ea` |

Revised-8 differs from revised-7 in exactly one file,
`src/simutrans/display/simgraph32.cc`.

---

## 9. What this does not say

- **Xvfb, not a desktop.** Every run here is on a virtual X server. It exercises
  the X11 path, the event delivery and the drawing; it is not Wayland, not a
  compositor, not a GPU driver, and no claim is made about any of those.
- **One pakset, one savegame, one screen size.** pak64 and `tutorial64.sve` at
  1366x768 and 1280x800. Yona's pakset is not known and was not guessed from the
  look of his screenshot.
- **Linux x86_64 is what was measured.** Both defects are in code every platform
  compiles, so Windows had them too, and the Windows builds here carry the fixes -
  but the measurements above were not repeated on Windows.
- **Yona's "I can now start Simutrans without any problems" is about starting.**
  It is recorded as that and not as a verdict on the renderer. His case is not
  closed until he answers on this build.
- **The tree flicker is untouched.** It has never been reproduced in this
  laboratory. Section 3.4 records a lead, not a diagnosis.
- **The minimap's own frame-of-displacement is trunk behaviour and was not
  changed.** `map_frame_t::draw()` still tells the minimap the old scroll offset
  and then moves the scrollbars; FIX-4 only makes the renderer clip the result,
  as the 16-bit renderer always did. Whether that ordering is worth tidying in
  trunk is a separate question and not one this mission was asked to answer.

---

## 10. Verdict and next action

    D1  minimap paints outside its window   REAL -> FIXED (FIX-4)
    D2  icons misplaced in the GUI          REAL -> FIXED (FIX-5)
    candidate                               RGBA32-CANDIDATE-REVISED-8
    readiness                               READY_FOR_APPROVAL
    publication                             NOT_AUTHORISED

Nothing has been published, nothing has been pushed, `main` is untouched and no
tag was created or moved. The next action is the user's: approve the delivery,
and then the branch, the release and the forum reply can go out.

---

## 11. What was produced

### Patches — `patches/`

| file | sha256 |
|---|---|
| `FIX-4-simgraph32-draw-array-clip.diff` | `6ce28be3975a2bff4b9495a53ba665531dbf7b5c84b74b186079efd5181f1291` |
| `FIX-5-simgraph32-image-offsets.diff` | `3c7ce45c7d6907c9e8ed65a628c5fa5c6e3c52be22a92498d379459f29fbc890` |
| `RGBA32-CANDIDATE-REVISED-8.diff` | `bdc0a3262b0b8168d148d643ce418256fdf08254128934883662db7794bc64ea` |

### Packages — `pkg/`, built but **not published**

| package | bytes | sha256 |
|---|---|---|
| `simutrans-rgba32-revised8-r12261-linux-x86_64-sdl2.tar.gz` | 8 527 572 | `0cac9d0944ccf1f86117e13f02a7a6bd3615a7ef93a7fbd7dc91e4606d5bd680` |
| `simutrans-rgba32-revised8-r12261-win64-sdl2.zip` | 15 212 039 | `3210195ab1d3faa2574651484026140916efabdef9f1a6f6110ee8095ceb10d1` |
| `simutrans-rgba32-revised8-r12261-win64-sdl3.zip` | 17 043 259 | `bc0ca0ce2cf5d091c8fb67be007ec5e9d9f8de2d1d68a61c095a16bfba5272e4` |

Linux: Debian 13, GCC 14.2.0, CMake 3.31.6, SDL2 2.32.4, Release, multithreaded,
with bzip2 and miniupnpc linked statically exactly as revised-7 was. Measured
floor **GLIBC_2.38**, GLIBCXX_3.4.29, CXXABI_1.3.9. The same archive, extracted
and run with **no symlink created anywhere**:

| distribution | glibc | unresolved | text in the pakset window | pakset + savegame | errors |
|---|---|---|---|---|---|
| Debian 13 | 2.41 | 0 | 0.331663 = 16-bit | loads | 0 |
| Fedora 43 | 2.42 | 0 | 0.332569 = 16-bit | loads | 0 |
| Fedora 44 | 2.43 | 0 | 0.350964 = 16-bit | loads | 0 |

and the fontconfig-with-no-fonts fallback gives 0.299406 in both depths on all
three, the same number revised-7 gave.

Windows: MSYS2 MinGW-w64, Release, multithreaded, SDL2 and SDL3, both depths.
SDL, libstdc++ and libwinpthread static, **0 DLLs in either package**. Each
package unpacks to 925 files with 772 font and text files - the count that
proves the Simutrans `.gitignore` did not eat the data directory - two
executables, three patches, `README-TESTERS.txt`, `LICENSE.txt`, and a
`SHA256SUMS.txt` whose five entries all verify. No pakset, no savegames.

All four Windows binaries start headlessly, read the pakset and enter a world:

```
sdl2-32  errors=0  pakset=1  world=1
sdl2-16  errors=0  pakset=1  world=1
sdl3-32  errors=0  pakset=1  world=1
sdl3-16  errors=0  pakset=1  world=1
```

There is no GUI automation for Windows on this machine, so that is a RUN-level
smoke test and not a pixel measurement. The pixel measurements are the Linux
ones above, from the same sources.

The three `.sha256` sidecars are plain ASCII with LF endings and `sha256sum -c`
returns OK for all three - the CRLF trap that broke two earlier deliveries was
checked for explicitly.

### Documents

    STLAB-RGBA32-MINIMAP-ZOOM-AND-FACTORY-ICONS-01.md   this report
    RELEASE-NOTES-8.md                                  release notes, unpublished
    FORUM-MESSAGE-DRAFT.md                              reply for the thread, UNSENT
    pkg/README-TESTERS.txt, pkg/README-TESTERS-WIN.txt  shipped inside the packages

### Evidence

    evidence/minimap-zoom.mp4        the tester's recording, as downloaded
    evidence/factory-window.png      the tester's screenshot, as downloaded
    evidence/seq/                    his 178 frames, renamed in order
    out/frame-index.csv              frame -> time for those
    out/w018.png, out/w019.png       his two consecutive frames, before and after
    out/film-*/session.mp4           the six recordings made here
    out/film-*/worst-frame.png       the worst frame of each
    out/film-*/band.csv              every frame's measurement
    out/repro-*/21-factory-info.png  the factory window in every arm
    out/probe-cand-imgoff-{16,32}.txt   what the GUI is told about 6962 images

### The branch

Prepared **locally only**, in `C:\Users\Victor\dev\simutrans-true32-renderer`,
on `fix/minimap-clip-and-image-offsets`, commit `ddd0419`. It carries
`source/rgba32-revised-8-r12261/` (2164 files, of which 772 font and text and 16
under `src/simutrans/builder/`, all added with `git add -f` because the
Simutrans tree's own `.gitignore` would otherwise drop them), the three patches,
this report and the release notes.

**Nothing was pushed.** `main` is still at `2bb5846`, there are no tags, and no
release was made.

One thing worth recording for the next delivery: CMake writes `revision.h` and
the two files it generates from `.in` templates **into the source directory it
is pointed at**, and the Windows build here was pointed straight at `src-r8`. So
three build artefacts appeared in the source tree and were staged on the first
attempt. They were removed and the tree re-staged; `src-r8` is now byte-identical
to the tree the patch was generated from and verified against. The packages were
never affected - they copy only `src-r8/simutrans/`.
