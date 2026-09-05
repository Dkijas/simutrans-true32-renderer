# GDI resize-event delivery: certified fix (r12254)

Laboratory missions STLAB-SIMUTRANS-GDI-RESIZE-EVENT-DELIVERY-01 (discovery, with its follow-up
STLAB-SIMUTRANS-GDI-RESIZE-EVENT-SLOT-01) and STLAB-SIMUTRANS-GDI-RESIZE-EVENT-DELIVERY-SVN-CERT-01
(certification), 2026-09-05. Certified verdict: **A - RESIZE DELIVERY FIX CERTIFIED FOR SVN**,
confidence HIGH. Status: `READY_FOR_SVN_INTEGRATION = YES`, `SVN_INTEGRATION = NOT_AUTHORISED`
(maintainer approval required). Not in official SVN.

This is a Windows backend / shared event-queue fix found while testing the TRUE32 backends. It is
**not** part of the TRUE32 renderer candidate (canonical patch v2 unchanged) and it does **not** fix
the DPI-scaling defect of forum topic 23805 (see "What it does not fix").

## Problem

On Windows with the GDI backend the renderer can keep an old framebuffer size after the window is
resized: after a maximize the image stays confined to the old 800x600 area and the rest of the
client area is black, persistently (still so 40 s later). Reproduced deterministically at 100 %
scaling, so DPI is not involved.

## Root cause

Two independent transitions lose the newest client size, and both are needed to explain the
reproducer:

- **Path B, normal play (deterministic, 3 / 3):** the backend keeps ONE pending event in the global
  `sys_event` slot and every `WindowProc` branch assigns to it. One message retrieval
  (`GetMessageW`) dispatches the *sent* messages (`WM_WINDOWPOSCHANGED` -> `WM_SIZE`) before returning
  the *posted* one (`WM_MOUSEMOVE`, which Windows posts right after the client area grows under the
  pointer). Three writes into one slot between two polls: the resize written by `WM_SIZE` is
  overwritten by the mouse move before anything reads it. Proven with a slot-transition trace:
  `OVERWRITE by WM_MOUSEMOVE | before: SIM_SYSTEM 3440x1369`.
- **Path A, during loading (phase race):** `loadingscreen_t::set_progress` consumes the resize,
  applies it and stores it; the destructor flushes it back to the global queue (r6846, 2013, so the
  main loop also learns the size); the next loading screen dequeues, applies and stores it again.
  With event identity the startup size (id=1, 800x600) was applied four times, the last time AFTER
  the real maximize size (id=2), leaving the renderer small. While the global queue is non-empty the
  backend is not polled at all.

## Contract

Resize is **CURRENT_STATE**, not history. Invariant: the newest client size must reach the polling
consumer; no older size may be applied after a newer one; intermediate sizes may be dropped only
when superseded by a newer one. No consumer needs the sequence of intermediate sizes.

## The two-part correction

`patches/gdi-resize-event-delivery-r12254.diff` - 2 files, 4 hunks, base SVN r12254:

- `src/simutrans/sys/simsys_w.cc` (3 hunks): `WM_SIZE` no longer writes the single slot; it records
  the newest client size in a pending state (`resize_pending`, width, height). `GetEvents()` hands
  that size to the slot as a `SYSTEM_RESIZE` when the slot is empty, before pumping further
  messages. A later message in the same retrieval cannot destroy it; a later `WM_SIZE` simply
  replaces the pending size. Size arithmetic and minimum-size guards unchanged.
- `src/simutrans/simevent.cc` (1 hunk): `queue_event()` of a `SYSTEM_RESIZE` discards the
  `SYSTEM_RESIZE` events already queued, so a replayed older size can never be applied after a
  newer one. Nothing else in the queue is touched.

No sleeps, timers, retries, redraw workarounds, maximize special cases, DPI or renderer code.

## Evidence (certification lane, fresh pure-trunk checkout at r12254)

Screen oracle: real screen pixels of the client area (no PrintWindow), the game's bottom status bar
must be lit across the bottom-right of the client area. Identity: lab-only, compile-time guarded
trace giving every backend-fetched SYSTEM event a monotonic id, logging loading-screen
consume/flush, queue dequeue/coalesce, renderer application and every transition of the backend
slot; plus a lab ordering invariant at the three consumers (an applied id lower than the last
applied id is a violation).

| Gate | Result |
|---|---|
| GDI maximize after loading, 100 %, pak64 | pure trunk FAIL 3 / 3; candidate PASS 3 / 3 (production) + 3 / 3 (trace) |
| GDI maximize during loading, pak64 | candidate PASS 3 / 3 + 3 / 3 |
| GDI maximize during loading, pak128 | candidate PASS 3 / 3 + 3 / 3 |
| Resize storm (scripted border drag) | 155 `WM_SIZE`, final native 1212x818, final applied 1212x818 (id 155), no older size afterwards |
| Restore after maximize | PASS (fresh event id 3, 800x600, applied last) |
| Ordering invariant | 369 applications in 11 trace runs, 0 backward applications |
| Mutant A: pending-slot mechanism removed | historical path B reproduced 2 / 2 (resize overwritten, never applied) |
| Mutant B: queue coalescing removed | historical path A reproduced (`VIOLATION: id=1 applied AFTER id=2`) |
| SDL2 (shared `simevent.cc`) | HEAD PASS 3 / 3, candidate PASS 4 / 4 (startup, resize storm, maximize/restore, pak128 loading) |
| SDL3 | HEAD PASS 3 / 3, candidate PASS 4 / 4 |
| Official automated suite (`-scenario automated-tests`) | BASE 302 / 302, CANDIDATE 302 / 302, identical script output |
| Clean builds GDI / SDL2 / SDL3 | 0 errors, 361 objects each, warning sets identical to pure trunk |

The during-loading phase is a race in the harness (the replay depends on where the maximize falls
among the loading-screen instances); on pure trunk it failed in most runs, with the candidate in
none. Identity tracing, not run counts, proves that path. Details and per-run tables:
`evidence/gdi-resize-event-delivery/`.

## Provenance

- base: SVN trunk r12254
- files: `src/simutrans/sys/simsys_w.cc`, `src/simutrans/simevent.cc`; no adds, deletes or property
  changes; `svn:eol-style native` preserved, no line-ending churn
- published patch: `patches/gdi-resize-event-delivery-r12254.diff`, LF, `a/` `b/` prefixes,
  SHA256 `13f283a63235bfb913a231b945f02ea93a71b5f336da135088944bd8dbeeb76b`; applying it to pure
  r12254 reproduces the certified files byte for byte
- certified `svn diff` of the same change (localised svn headers): SHA256
  `b07d33cbaf87e95782d80526d1923f0134299ed7091318a12c8f71bffed58fa0`; identical hunk content

## Relation to TRUE32

TRUE32 inherits the Windows backend's event delivery, so a TRUE32 GDI build shows the same defect;
the bug is not TRUE32-specific and the fix is independent of the renderer architecture. TRUE32 v2
also modifies `simsys_w.cc` (system colours, DIB format), so the two patches touch one common file.
Composition check on disposable copies of r12254: CLEAN_COMMUTATIVE - the two application orders (v2 then GDI fix, GDI fix then v2) produce byte-identical trees (one order needs a 12-line offset in simsys_w.cc, no fuzz, no rejects); a GDI32 build of the composed tree (COLOUR_DEPTH=32) compiles clean (361 objects, simgraph32 only) and passes one maximize/restore smoke (3440x1369 painted in full, restore to 800x600) and one resize-storm final-state check (final 1212x818 painted in full). Documented for compatibility only; no consolidated candidate is created.

The canonical TRUE32 patch remains v2. No consolidated candidate is created by this publication.

## What it does not fix

The separate DPI defect (`WindowSize` holding physical pixels when filled by `dr_os_open` and logical
pixels when filled by `dr_textur_resize`, read as physical by `WM_PAINT`) remains OPEN. Forum topic
23805 is therefore not resolved by this patch; it is now re-certifiable deterministically because
event delivery no longer injects nondeterminism.

## SVN status

Not integrated. Integration requires explicit maintainer authorisation (prissi). Certified against
r12254; trunk HEAD was still r12254 at publication.
