# XR32 testing-5 — candidato para Windows (local, no publicado)

Misión SIMUTRANS-XR32-TESTING5-WINDOWS-01. **Publicación: NOT_AUTHORISED.** No se ha hecho push, tag, release, commit SVN ni publicación en el foro o Discord. testing-4, los artefactos de TCC, los laboratorios A4 y TCC, Pak192.Comic y el SVN oficial quedan intactos.

| | |
|---|---|
| Base | testing-4 publicado `92ef4551` = laboratorio `a4/cache` `0afdfbd` (SVN r12377) |
| Candidato | `t5/candidate` **`038b538`** = base + TCC `07e4d54` (cherry-pick de `03de7ed`, sin cambios) + A11 `038b538` (cherry-pick de `c66eaaa`) |
| Plataformas | **solo Windows x64**, SDL2 y SDL3 |

## Documentos

| Fichero | Contenido |
|---|---|
| `CERTIFICATION-REPORT.md` | informe ejecutivo, veredicto y recomendación |
| `A11-ROOT-CAUSE.md` | causa al píxel, corrección y alternativas descartadas |
| `TCC-INTEGRATION.md` | ramas, identidad del parche TCC e interacción entre los dos cambios |
| `WINDOWS-TEST-MATRIX.md` | cada prueba, SDL2 y SDL3, con su control |
| `PERFORMANCE.md` | tiempos de fotograma P50/P95/P99, coste de zoom, memoria pico y contadores de la caché A4 |
| `KNOWN-ISSUES.md` | lo resuelto, lo que sigue abierto, hallazgos y límites de la certificación |
| `RELEASE-NOTES-DRAFT.md` | borrador en inglés (no publicado) |
| `SHA256SUMS`, `freeze-verification.txt` | sumas de todo el paquete, verificadas, con control negativo |

## Contenido

- **`candidate/`**:
  - ejecutables: `simutrans-rgba32-testing-5-candidate-win64-sdl3.exe` y `-sdl2.exe`, los de 16 bits y `makeobj-xr32-testing-5-candidate-win64.exe`;
  - `xr32-testing-5-candidate-source.tar.gz` (fuente reproducible);
  - `BUILD-MANIFEST.txt` (receta, herramientas y hashes).

  Los nombres llevan `-candidate-` a propósito, para no confundirlos con una versión publicada. Equivalen a los del encargo: `simutrans-rgba32-testing-5-win64-sdl2/3.exe`.
- **`patch/`**:
  - `XR32-TESTING5-CANDIDATE.diff` (testing-4 → candidato);
  - `01-TCC-translucent-company-colour.diff` y `02-A11-coast-2x1-outline.diff`;
  - `PATCH-IDENTITY.txt` (pruebas de identidad con control).
- **`evidence/`**: evidencia en texto, resultados de los oráculos y capturas antes/después de A11. Las capturas usan el pak mutante (pak128.german con un mar sintético de un color); no hay arte de Pak192.Comic, que es CC BY-NC-SA y se queda en el carril.

## Reproducir

- **Compilación:** `tools/build_win.sh <ref> <nombre> <sdl2|sdl3> <32|16|makeobj>` en el carril, con MSYS2 MinGW-w64 GCC 16.1. La misma receta aplicada a `a4/cache` da los ejecutables **publicados** de testing-4 byte a byte.
- **Oráculos:** `tools/a11_oracle.py` (huecos en el mundo), `tools/a11_model.py` (modelo independiente de la regla de 2/1), `tools/a11_identity.py`, `tools/gates_t5.py`, `tools/zoom_oracle2.py`, `tools/night_oracle.py`, `tools/critical_case.py`.
- **Captura sin foco:** `tools/scene_gate.sh` + `tools/launch_wait.ps1` + la puerta gdb `tools/battery/gdbgate.py` en modo adjunto.
