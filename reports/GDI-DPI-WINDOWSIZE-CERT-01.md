# GDI DPI: the `WindowSize` unit contract, certified fix (r12254)

Laboratory mission STLAB-SIMUTRANS-WINDOWS-GDI-DPI-UNIT-RECERTIFICATION-03, 2026-09-05, after two
earlier attempts whose visual certification had been contaminated by a separate defect. Certified
verdict: **A - DPI UNIT DEFECT AND FORUM FAILURE CAUSALLY CERTIFIED**, confidence HIGH.

    WINDOWS_GDI_DPI_UNIT_DEBT = CERTIFIED_FIXED at candidate level
    FORUM_23805_ROOT_CAUSE    = PROVEN
    FORUM_23805               = OPEN
    READY_FOR_SVN_INTEGRATION = YES   (technical readiness only)
    SVN_INTEGRATION           = NOT_AUTHORISED   (maintainer approval required)
    TRUE32_CANONICAL_PATCH    = v2   (unchanged by this patch)

This is a Windows backend fix, not a renderer change. It is published as a separate auxiliary patch
next to the canonical TRUE32 candidate, exactly like the resize-event delivery fix.

## Dependency chain

    r12254
      -> patches/gdi-resize-event-delivery-r12254.diff   (certified GDI resize-event delivery fix)
        -> patches/gdi-dpi-windowsize-r12254.diff        (this patch)

The event-delivery fix is a **prerequisite of the configuration in which this patch was causally
certified**. Before it, resize events could be lost or replayed, and every DPI experiment inherited
that nondeterminism - which is precisely why the earlier DPI certification attempts were rejected.
Nothing here claims the DPI patch is certified against uncorrected event delivery.

## Problem

Windows, GDI backend, screen scaling other than 100 % (forum topic 23805): the window is drawn
correctly until it is resized, after which the content occupies about two thirds of each dimension
and the rest of the client area stays black or stale. Linux is unaffected. With event delivery
corrected, the failure is deterministic: maximize at 150 % failed in 6 of 6 runs, with 909 of 1369
rows painted, and still failing ten seconds later.

## Root cause: one variable, two units

`WindowSize` (a `RECT` in `simsys_w.cc`) has two producers and one consumer.

- `dr_os_open` writes it as **physical client pixels** (logical size scaled up by the screen scale).
- `WM_PAINT` reads it as physical, twice: as the `StretchDIBits` destination rectangle on the window
  device context, and to recompute the DIB height by dividing by the scale.
- `dr_textur_resize` writes the **logical** framebuffer size into it, unscaled.

At 100 % the two units are numerically equal and nothing shows. At 150 % the second producer stores
two thirds of the physical size, so the next `WM_PAINT` paints into a two-thirds rectangle and
rewrites the DIB height to two thirds of the framebuffer. Every later partial blit clips its rows to
that height, so the bottom third is never painted again until the next resize. That is the reported
signature, and the trace shows the height being rewritten 131 times in one run.

The intended unit is therefore physical client pixels, proven from the producers and the consumer
rather than from the variable's name.

## The fix

`patches/gdi-dpi-windowsize-r12254.diff` - one file, one hunk, +8 / -2 (six added lines are the
comment that states the unit). In `dr_textur_resize`, `WindowSize` is stored scaled:

    WindowSize.right  = (w * x_scale) / 32;
    WindowSize.bottom = (h * y_scale) / 32;

Nothing else changes: no event-delivery behaviour, no mouse coordinates, no framebuffer pitch or
format, no renderer code, no timers, sleeps or repaint workarounds.

## Evidence

Contract oracle: lab-only instrumentation records, at every geometry transition, the physical client
rectangle, the scale, `WindowSize`, the DIB header, and classifies the `WindowSize` unit.

| Configuration | `dr_textur_resize` observations | `WM_PAINT` height rewrites that shrink |
|---|---|---|
| 100 % without the fix | 93 / 93 PHYSICAL | 0 |
| 150 % without the fix | 136 / 136 **LOGICAL** | 131 |
| 150 % with the fix | 136 / 136 PHYSICAL | 44, all a one-row floor rounding |
| 100 % with the fix | 93 / 93 PHYSICAL | 0 |

Product-visible oracle (real screen pixels, foreground verified, status bar across the bottom-right
of the client area): without the fix, maximize fails 6 / 6 at 150 % with the two-thirds signature,
and resize-larger, drags, the resize stream and restore fail the same way; with the fix, no capture
failed in three on-screen runs. At 100 % both configurations pass every operation.

Negative control: the same event-fixed base without the DPI hunk is the old-unit mutant, and it
reproduces both the deterministic contract mismatch and the visible corruption.

Other gates: the official automated suite passes 302 / 302 on both configurations with identical
script output; an SDL2 control at 150 % passes every operation (SDL2 has no `WindowSize`, so the
defect is GDI-specific); clean GDI builds with a warning set identical to pure trunk.

Details: `evidence/gdi-dpi-windowsize/`.

## Composition with TRUE32

Applied in the certified order on a disposable tree from clean r12254 - TRUE32 v2, then the
event-delivery fix, then this patch - the three patches compose without conflict; the composed
GDI32 build links the 32-bit renderer only and passes a maximize / resize-stream / restore smoke at
150 %. See `evidence/gdi-dpi-windowsize/composition-gate.txt`.

The canonical TRUE32 patch remains v2. No consolidated candidate is created by this publication, and
neither auxiliary patch is folded into v2.

## Residual, stated plainly

The height is stored as `floor(h * scale / 32)` and read back as `floor(x * 32 / scale)`, which
returns one row less for an odd logical height. It costs a single framebuffer row at the bottom and
no visible failure in any run. A separate, bounded transient exists in trunk and is untouched here:
when the scale changes at runtime, `WindowSize` still holds the value computed with the previous
scale until the resize that the change itself triggers.

## Scope

This patch fixes the unit contract that the forum failure depends on. Forum topic 23805 stays open:
the fix is not in official SVN, integration requires explicit maintainer authorisation, and no forum
post was made about it.
