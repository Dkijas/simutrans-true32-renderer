> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# Known limitations

| # | Limitation | Consequence | What would close it |
| --- | --- | --- | --- |
| L1 | **makie's keyed working file is not available.** The published file has no keys. That the capture of 5 Nov came from keys rests on makie's own statement (`6b6b6b`), on the blue dots, and on the lab reproduction | The fix is proven on lab reconstructions, not on makie's exact resource | makie running the candidate on a file with keys |
| L2 | **Nobody but me has seen the candidate.** makie's "everything is working" is about makie's file on testing-6 | No artist review of keyed corners | A review after a decision to publish |
| L3 | Keyed coast water stays 16-bit on those slopes | Land 8-bit, water 16-bit on a keyed pak; no real pak today | `A4-SPECIAL-COLOR-FALLBACK.md` |
| L4 | Declared semantics (v2 runs) on a terrain source keep the fallback | Undefined meaning, left as testing-6; lab pak only | A maintainer's decision |
| L5 | Translucent light map crashes at load, also in clean r12377; translucent texture refuses | Not touched, by order | `LIGHTTEXTURE-ALPHA-CRASH.md` |
| L6 | **Real keys on SDL3, zooming out: 4 of 4 runs do not restore the picture, on testing-6 too.** Unexplained. The first posted key seems to have no effect | Not caused by the candidate. Posted-key input may be the cause. The same round trip through the game's zoom call is exact, 40 of 40. **A player's zoom-out with a physical keyboard on SDL3 was not tested** | Someone at the keyboard; or instrumenting the keys |
| L7 | Cost with keys: zoom step +2.5 to +2.8 ms, light change +0.7 to +1.6 ms at 100 % keyed corners | Same as a keyless pak costs; only a keyed pak sees it as a change | A12-P1, proposed, not implemented |
| L8 | `zoomx` P99 −25 ms on the keyed pak, not understood | Not claimed as an improvement | Tracing those frames |
| L9 | n = 7 and 5; long sessions n = 2, 4,000 frames; peak working set hides small movements; no heap or ASan on the candidate | "Controlled" means within this | A session of hours under ASan |
| L10 | One real pak with XR32 terrain, its lab variants, a traditional pak, and pak192.comic. **The TCC regression covers 4 XR32 images**, plus byte identity on 50,823 others; TCC's own gates were not re-run | No claim of universal compatibility | More paks |
| L11 | Transitions checked at 1/1 only against the model, two climate pairs, 19 slopes; other zooms by byte identity and by NK against NK1 | No independent model of the zoom filter | An oracle of the filter |
| L12 | One display, 3440×1440, one GPU. Real windows at 1920×1080. **Multi-monitor not validated.** Capture of the display reads 13,11,12 as black (Windows' own, measured outside the game) | | Other machines |
| L13 | **Linux not certified.** Compile, link and hash check only; it did not run with graphics | `LINUX-RECIPE` section of `candidate/README-CANDIDATE.md` | A Linux mission |
| L14 | Windows Search keeps running with bursts of disk and memory on this machine | Uncontrolled noise in timings | |
| L15 | Inherited from testing-6: 32-level weights, snow near white on some steep slopes, A12's event cost, and r12377's own known issues | `RELEASE-NOTES` of testing-6 | |

## Not a limit, but worth saying

The candidate fixes nothing visible on PAK-OLD or PAK-NEW, by design: it is byte-identical there. Its value is that the next `LightTexture` or `ClimateTexture` may land on a grey key without anyone noticing, and that artists need not avoid those colours (D3). It is **robustness, not a visible improvement for today's pak**.
