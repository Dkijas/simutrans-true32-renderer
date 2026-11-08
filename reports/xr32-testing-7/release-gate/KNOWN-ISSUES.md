> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# Known issues and limitations

## Of testing-7

| # | Issue | Consequence | Status |
| --- | --- | --- | --- |
| K1 | **makie's older keyed `LightTexture` is not available.** The cause rests on makie's own diagnosis (#93: special colours in `LightTexture`, `6b6b6b`), the blue dots of the capture, and the lab reproduction, all of which agree. makie later wrote (#96): "Processing special colors in LightTexture is simply superfluous. There is no sensible application for this", which agrees with what the fix does | Converging evidence, not a byte-for-byte reproduction of that file | open |
| K2 | **Nobody but the lab has seen the candidate.** makie's statements are about makie's own files on testing-6 | No artist review on a keyed pak | open |
| K3 | **Coast water** keeps the 16-bit picture on slopes whose `LightTexture` row has a key | Land 8-bit, water 16-bit there. No real pak today | separate fix, not certified, not integrated (`evidence/issues/A4-SPECIAL-COLOR-FALLBACK.md`) |
| K4 | **Extra composition cost** where the fix acts: zoom step +2.0 to +3.0 ms; light change up to ~+3 ms in its slowest frames | Equal to what a keyless pak already costs | by design; A12-P1 pending |
| K5 | **Memory with constant zooming** grows slowly in sessions of 13–16 minutes, on the keyless pak too | See `MEMORY-STRESS.md` for the long-session verdict | see there |
| K6 | Declared semantics (v2 masks) on a terrain source keep the fallback | Undefined meaning; lab only | for a maintainer |
| K7 | **A translucent pixel in `LightTexture` crashes at load**, also in clean r12377 | Not changed | `evidence/issues/LIGHTTEXTURE-ALPHA-CRASH-CARD.md` |
| K8 | **Linux: experimental, not certified.** Built, packaged, started and quit cleanly under WSL 2; no picture certified | | `LINUX-STATUS.md` |
| K9 | Windows: one display, one GPU, Windows 11, MinGW only. Multi-monitor and a physical keyboard not tested | | |
| K10 | 32 blend levels in transitions | as in testing-6 | accepted |
| K11 | XR32 is experimental; the release is a pre-release | | |
| K12 | All known issues of testing-6 and of official r12377 | | inherited |

## Found during this gate, not defects of the candidate

| Finding | What it is |
| --- | --- |
| SDL3 ignores characters posted to an unfocused window | SDL3's text input belongs to the focused window. With focus the zoom by keyboard is exact (`SDL3-ZOOM-INVESTIGATION.md`, `evidence/issues/SDL3-KEYBOARD-FOCUS-NOTE.md`) |
| 8 of 600 frames mismatched once in the dirty-region check (`windows` scenario, SDL3) while another program was in use | 0 of 720 when repeated on an idle machine, testing-6 and candidate. Cause not established |
| `CopyFromScreen` reads 13,11,12 as black | Windows' capture, shown outside the game |
| A window held by gdb becomes a DWM ghost window | measuring tool; the capture now waits for it |

## Gate items not run

| Item | Why |
| --- | --- |
| A physical keyboard | no access |
| `si-unicode` and `post-full` key modes; `si-vk` and `si-page` on SDL2 | skipped for time; SDL2 already works with posted characters |
| ASan, heap tracing | MinGW build; peak, private bytes and sessions only |
| Native Linux desktop, other distributions | not available |
