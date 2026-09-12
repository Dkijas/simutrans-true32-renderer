# FIX-1 — black window at startup, 32-bit renderer

Reported by makie on the forum, 2026-09-11: *"I started the program and, oh
shock, the window was completely black. No error message, nothing. Somehow I got
the idea to resize the window and there it was, the pak set selection."*

**Status: reproduced, cause found, fixed, fix verified, negative control holds.**

## Reproduction

From the **published** archive
(`de1b768e018f85224a174806e9eac5317b0e5069388fd85e032d476092e14ecb`, hash
verified before use), each case in its own extracted copy so `-singleuser` keeps
the settings separate. Ubuntu 26.04, SDL2 2.32.10, a private Xvfb display,
1280×800×24, capture with `xwd`. The number is the fraction of screen pixels
that are not pure black.

| case | before resize | after resize |
|---|---|---|
| **32-bit, no pakset installed** | **0.000000 — every pixel pure black** | 0.119981 |
| 16-bit, no pakset installed | 0.126965 | 0.126965 |
| 32-bit, pakset installed | 0.384465 | — |
| 16-bit, pakset installed | 0.384320 | — |

So: 32-bit only, and only on the path that shows the **pakset selection**, which
is why it was missed before — every earlier smoke test started with a pakset
present and a savegame to load.

## Which half of the pipeline

The report could mean two very different things: the interface is drawn but never
presented, or it is never drawn. An instrumented build of the SDL2 backend
answers it directly — `dr_flush` is reached every frame, the upload covers the
whole window, and `SDL_UpdateTexture` succeeds every time:

```
stlab_probe: pre-flush  flush=1 textur=0 fail=0 | textur=0x754f2c2f9040 actual_w=704 h=560 | surf 704x560 pitch=2816 | nonblack=0
stlab_probe: dr_textur #1 rect=0,0 704x560 rc=0
stlab_probe: post-flush flush=1 textur=1 fail=0 | ... | nonblack=0
...  (eight frames, all identical, nonblack=0)
```

`nonblack=0` is the framebuffer **in memory**. The presentation path is innocent:
nothing was ever drawn. For contrast, the same build with a pakset present
reports `nonblack=8577` from the second flush onwards.

## Cause

`simgraph32_init()` never set an initial clipping rectangle.

`clips32[MAX_THREADS]` is a file-scope static, so it starts zeroed:
`clip_rect.xx` and `clip_rect.yy` are both 0. Every drawing routine clips against
that empty rectangle, so nothing reaches the framebuffer at all — no background
fill, no dialogue frame, no text, and no error, because nothing failed.

`simgraph16_init()` has always ended with

```c
simgraph16_set_clip_rect(0, 0, disp_width, disp_height  CLIP_NUM_DEFAULT, false);
```

and the 32-bit init did not. Loading a pakset sets a clip rectangle on the way
through, which is why the defect only shows when the pakset selection is the
first thing drawn. Resizing the window calls `on_window_resized()`, which sets
the clip rectangle — that is exactly why makie's resize fixed it.

## The fix

One file, one statement, mirroring `simgraph16_init()` including its use of
`disp_width`:

```c
 	init_colour_tables();
 	dirty_state_alloc();
 
+	// The clipping state is a zeroed static, so without this every drawing
+	// operation is clipped against an empty rectangle and nothing reaches the
+	// framebuffer. simgraph16_init() has always ended with this line; leaving
+	// it out here made the first window black until something else set a clip.
+	simgraph32_set_clip_rect( 0, 0, disp_width, disp_height  CLIP_NUM_DEFAULT, false );
+
 	return true;
 }
```

`patches/FIX-1-simgraph32-initial-clip-rect.diff`, sha256
`5edeca53b801448affd86b2a3557debcda10dc452b5c4c0f8eac9da7202c1b40`.

## After the fix

Same archive, same conditions, binaries rebuilt from the same tree with and
without the fix:

| case | unfixed, before resize | with FIX-1, before resize |
|---|---|---|
| **32-bit, no pakset** | **0.000000** | **0.119981** |
| 32-bit, pakset | 0.384465 | 0.356214 |
| 16-bit, no pakset | 0.126965 | 0.126965 |
| 16-bit, pakset | 0.384320 | 0.384499 |

The pakset selection is now on screen immediately, without touching the window.
Both 16-bit rows are unchanged, as they must be: the fix is inside
`simgraph32.cc` and the 16-bit build does not compile that file.

**Negative control:** the unfixed binary, built from the same tree in the same
run, still shows the pure-black window. Removing the fix brings the defect back;
that is the control, not a separate experiment.

## Limits of this evidence

Measured on Ubuntu 26.04 under WSL2, on a **virtual** X server (Xvfb, SDL2 x11
driver). That is a real X11 path, but it is not a desktop session, and this
machine has no window manager, so window-manager-driven resize and focus
behaviour is not covered. What is covered is the part that matters here: the
symptom reproduces and disappears with the one-line change, and the instrumented
build shows why.
