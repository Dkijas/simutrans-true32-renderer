# Integración de TCC y A11 sobre testing-4

## 1. Ramas (repositorio de laboratorio del carril, `src/`)

| Rama | Commit | Contenido |
|---|---|---|
| `a4/cache` | `0afdfbd` | testing-4 certificado = árbol publicado en `92ef4551` (`source/xr32-r12377/`) |
| `tcc/fix` | `03de7ed` | TCC congelado, traído sin cambios del laboratorio original `<tcc-lab>/src` |
| `a11/fix` | `c66eaaa` | corrección A11 sobre `a4/cache` |
| **`t5/candidate`** | **`038b538`** | `0afdfbd` → **`07e4d54`** (TCC, `cherry-pick -x` de `03de7ed`) → **`038b538`** (A11, `cherry-pick -x` de `c66eaaa`) |
| `lab/t5` | `b762fef` | `lab/cache` `7fe351e` (ganchos de medición de A4C) + los mismos dos commits |

Cada cambio es un commit propio, con la línea `(cherry picked from commit …)` que apunta a su original. No hay ningún commit de mezcla, ni retoques ni soluciones improvisadas.

## 2. El parche TCC no se ha alterado

Evidencia: `deliver/patch/PATCH-IDENTITY.txt`.

- `git diff a4/cache t5/candidate~1` es **idéntico** a `git diff a4/cache tcc/fix`, quitadas las líneas `index` (hashes de blob).
- A su vez, ese diff es idéntico al parche que entregó TCC-01 (`XR32-TRANSLUCENT-COMPANY-COLOUR-01.diff`) y al que congeló la consolidación.
- **Control:** un carácter cambiado en una copia hace fallar la comparación.

El cherry-pick aplicó **sin conflicto**. Los dos ficheros de TCC son:
- `stlab_semantic.cc`: el extractor de makeobj;
- `stlab_xr32_zoom.cci`: la ponderación por alfa dentro de `stlab_xr32_zoom_build`.

## 3. El commit A11 es el mismo en las dos ramas

Las 133 líneas añadidas o quitadas de `t5/candidate~1..t5/candidate` son **idénticas** a las de `a4/cache..a11/fix`. Solo cambia la numeración de las cabeceras `@@`, +10 líneas, porque TCC añade 10 líneas antes en el mismo fichero. **Control:** un carácter cambiado hace fallar la comparación.

## 4. ¿Interactúan las dos correcciones?

| | TCC | A11 |
|---|---|---|
| Fichero de zoom | cambia la ponderación **dentro** de `stlab_xr32_zoom_build()`, solo para muestras con clave translúcida de color especial | añade una función **aparte**, `stlab_xr32_zoom_build_2x_legacy()`, y la llama en `stlab_xr32_zoom_ensure()` |
| Imágenes que toca | imágenes con registros semánticos (colores de empresa, luces) | solo teselas de costa derivadas (A4), que **nunca** tienen registros semánticos: `stlab_xr32_derive_water_tile` rechaza un mar con `run_count != 0`, y la función nueva exige `run_count == 0` |
| Escalas | todas las que mezclan muestras | solo 2/1 |
| makeobj | sí (extractor) | no |

Los conjuntos de imágenes son **disjuntos**: una tesela derivada no pasa por la ponderación de TCC, y una imagen con colores de empresa no pasa por la regla A11. No hace falta modificar ninguno de los dos cambios.

**Comprobado en el binario integrado:**
- Puertas A4 de t5: lo que no es una tesela derivada a 2/1 es byte-idéntico a testing-4 en los 8 juegos de puertas (`evidence/gates-t5-s3.txt`, `gates-t5-s2.txt`).
- Captura adjunta de t5 a 1:1 de la escena del fixture TCC: **byte-idéntica** a la captura del motor TCC de la consolidación, mismo fixture y mismo pak (`evidence/attach-method-validation.txt`).
- Oráculo de zoom, caso crítico y métricas de objetos sobre t5: véase `WINDOWS-TEST-MATRIX.md`.

## 5. makeobj

makeobj de `t5/candidate` = `8a150381…`, **byte-idéntico** al makeobj TCC certificado (`mo-fix` de la consolidación y `makeobj-tcc01.exe` de TCC-01). A11 no toca makeobj. makeobj de `a4/cache` = `fce1d439…`.
