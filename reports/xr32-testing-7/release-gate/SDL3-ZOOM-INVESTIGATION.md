> *Lab report, published as written at its date (machine paths replaced by placeholders). Its "candidate", "not published" and "not authorised" wording refers to that mission; the published build is described in the release notes.*

# SDL3: zooming out by keyboard

**Verdict: not an engine defect, not a regression of testing-7.** The previous mission's failure was a **focus problem of the simulation**. In SDL3, characters reach the game only while its window has keyboard focus. With focus, zoom in and zoom out by keyboard make exact round trips on SDL3 and SDL2, on testing-6 and testing-7 alike.

**A physical keyboard was not tested.** There is no access to one here, and injected events are not hardware. A physical keyboard always types into the focused window, which is the case shown to work.

Evidence: `evidence/keys2.txt` (classified runs), `out/keys2-*` (captures, `run.json` with the focus state at every key). Tools: `tools/t7_keys2.ps1`, `tools/keys2_classify.py`, `tools/keys2_all.sh`.

## Method

A real window of each unmodified executable, no debugger, default video driver, the corner density scene, paused by its own key. A capture of the window after every key.

- **Directions:** "out" is 3 × zoom out (`<`) then 3 × zoom in (`>`); "in" is the reverse.
- **Identifying the zoom level:** each capture is compared with the engine's own framebuffer of the same save at all 11 zoom levels, made under the gate. The level is the reference with the fewest differing pixels.
- **Control:** the 11 references are all different from each other.
- **Residuals:** the small differences left at the matching level (1,000–8,000 px) are the pause banner and the mouse cursor; they are constant per level.
- **Focus:** recorded at every key.

| Mode | What it sends | Physical keyboard? |
| --- | --- | --- |
| `post-char` | `WM_CHAR` posted to the game's window: the previous mission's method | no |
| `post-char-fg` | the same, after bringing the window to the foreground | no |
| `si-vk` | `SendInput` of the real virtual key, Shift included, window in the foreground: OS-level injected input | no, but the closest available |
| `si-page` | `SendInput` Page Down / Page Up, which the game maps to zoom through key events, not text | no |

## Results

| Mode | Backend | Executables | Focus at the keys | Result |
| --- | --- | --- | --- | --- |
| `post-char` | SDL2 | testing-6, testing-7 | no | **exact round trips**, in and out |
| `post-char` | SDL3 | testing-6, testing-7 | **no** | **keys ignored**, the pause key too: the world keeps running into the night |
| `post-char` | SDL3 | testing-6 (one run) | yes (by chance) | exact round trip |
| `post-char-fg` | SDL3 and SDL2 | testing-6, testing-7 | yes | **exact round trips**, in and out: 1/1 → 3/4 → i9 → 1/2 → i9 → 3/4 → 1/1, and 1/1 → 9/8 → … → 1/1 |
| `si-vk` | SDL3 | testing-6, testing-7 | yes | **exact round trips**, in and out |
| `si-page`, without hardware scan code | SDL3 | testing-6, testing-7 | yes | no zoom: SDL3 reads an **unknown key**, and the game opens its Keyboard Help, as it does for an unassigned key. An injection artefact: a physical key always carries its scan code. Kept in `out/_keys2-sipage-run4-scancode0/` |
| `si-page`, **with** the hardware scan code | SDL3 | testing-6, testing-7 | yes | **exact round trips**, in and out (Page Up / Page Down) |

**Testing-6 and testing-7 behave identically in every mode.**

## Why

`simsys_s3.cc` takes ordinary characters, `<` and `>` among them, from `SDL_EVENT_TEXT_INPUT`. SDL3 starts text input per window. The TCC mission's probes (`probe5`, `probe6`) found that SDL3 makes text input only for the window that has keyboard focus. A posted `WM_CHAR` to a window without focus is therefore dropped. SDL2 delivers it anyway.

The previous mission's runs did not control focus: whichever window had it decided the outcome.

## The categories asked for

| Category | Answer |
| --- | --- |
| Engine defect | No: with focus the zoom is exact both ways |
| Simulation defect | **Yes**: posting characters to an unfocused window does not model a keyboard |
| Focus problem | **Yes**, it is the mechanism |
| SDL event problem | SDL3's design: text input belongs to the focused window. Not a fault of the game |
| Regression of testing-7 | No: identical on testing-6 |

## Runs that do not count

| Folder | Why |
| --- | --- |
| `out/_invalid-keys2-run1/` | PowerShell treats `$Dir` and `$dir` as the same variable, so the direction was overwritten by the run folder; and the window was not raised before capture |
| `out/_invalid-keys2-run2/` | another application in the foreground and in use: the game window was hidden or minimised |
| `out/_invalid-keys2-sipage-run3/` | the pause key was sent as Page Up |
| `out/_keys2-sipage-run4-scancode0/` | page keys injected without a scan code: a finding about injection, recorded above |

## Not tested

- A physical keyboard.
- Other keyboard layouts (this machine: es-ES).
- An IME.
- The mouse wheel, another zoom path.
- `si-unicode` and `post-full` modes: skipped for time.
