# ARCHIVE-CONTENTS-AUDIT

Every member of every file in `staging/` (the exact files proposed for publication), read from the archives
themselves (`tools/audit_archives.py`). What is searched for is in the script header: absolute or parent paths,
VCS / build / debug / log / save leftovers by name, this machine's user and lab paths, the user's e-mail address,
private keys and token shapes in the CONTENT of every member (binaries included), and tar modes and owners.

**Control:** a throw-away archive with one planted absolute member name and one planted user path in a text member:
3 findings -> CAUGHT.

| File | Members | Bytes (uncompressed) | Executables | Findings |
| --- | ---: | ---: | --- | ---: |
| `XR32-NIGHT-PROTOTYPE-FULL.r12377-testing-2.diff` | 1 | 502400 | - | 0 |
| `makeobj-xr32-night-prototype-r12377-testing-2-linux-x86_64.tar.gz` | 5 | 491821 | `makeobj` | 0 |
| `makeobj-xr32-night-prototype-r12377-testing-2-windows-x86_64.zip` | 4 | 1612473 | `makeobj.exe` | 0 |
| `simutrans-xr32-night-prototype-r12377-testing-2-linux-x86_64-sdl2.tar.gz` | 989 | 27038760 | `simutrans-16bit`, `simutrans-rgba32` | 0 |
| `simutrans-xr32-night-prototype-r12377-testing-2-linux-x86_64-sdl3.tar.gz` | 997 | 35799900 | `simutrans-16bit`, `simutrans-rgba32` | 0 |
| `simutrans-xr32-night-prototype-r12377-testing-2-win64-sdl2.zip` | 952 | 45817670 | `simutrans-16bit.exe`, `simutrans-rgba32.exe` | 0 |
| `simutrans-xr32-night-prototype-r12377-testing-2-win64-sdl3.zip` | 952 | 50761742 | `simutrans-16bit.exe`, `simutrans-rgba32.exe` | 0 |
| `xr32-night-prototype-r12377-testing-2-testkit.zip` | 166 | 1484175 | - | 0 |

**Findings: 0.**

Notes (looked at by hand, not findings):
- The executables contain `D:/M/msys64/mingw64/include/...` strings: paths of the MSYS2 project's own build
  machine, carried in the prebuilt static libraries. They are not paths of this machine.
- `victor` inside the executables is Brotli's built-in dictionary (`victory`, `victoria`); `_stlab` in makeobj
  is a function name (`stlab_sem_vocab`).
