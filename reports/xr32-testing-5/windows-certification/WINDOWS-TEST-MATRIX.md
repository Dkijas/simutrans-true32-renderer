# Matriz de pruebas — Windows x64, SDL2 y SDL3

Candidato `t5/candidate` `038b538`. Ejecutables de producto: SDL3 `f7f7ce82`, SDL2 `e82a2ce8`; de 16 bits, SDL3 `8feaaf49` y SDL2 `20cbf7da`. Laboratorio (`lab/t5` `b762fef`, `-DSTLAB_PERF`): SDL3 `f9e42af8`, SDL2 `53809471`.

Toda prueba cuyo PASS importa tiene un control que falla.

**Leyenda:**
- **mismo bin.:** el binario de 16 bits del candidato es byte-idéntico al de la base, así que su comportamiento es el mismo.
- **consolid.:** dato tomado de la captura de la consolidación hecha con el mismo binario, los mismos paks y la misma escena. Se usa porque Defender puso en cuarentena el binario SDL3 de testing-4 en este carril.

## 1. Integridad

| Prueba | SDL3 | SDL2 | Control / nota | Evidencia |
|---|---|---|---|---|
| Compilación reproducible: `a4/cache` vuelve a dar el testing-4 publicado | `43420d63` == publicado | `42e9089c` == publicado | receta de `tools/build_win.sh` | `logs/builds-*.txt` |
| Identidad del binario de 16 bits (candidato frente a base) | `8feaaf49` == `8feaaf49` | `20cbf7da` == `20cbf7da` | | `logs/builds-t5.txt` |
| makeobj | `8a150381` == makeobj TCC certificado | | | `logs/builds-t5.txt` |
| Conjunto de avisos de compilación | igual que la base | igual que la base | se comparan conjuntos, no recuentos | `evidence/warning-sets.txt` |
| Parche TCC integrado == congelado | IDÉNTICO | | un carácter cambiado se detecta | `TCC-INTEGRATION.md` |
| Formato XR32 / nodos | sin cambios: ni lector, ni escritor, ni makeobj (binario idéntico) | | | `patch/` |

## 2. A11 (costas a 2/1)

| Prueba | SDL3 | SDL2 | Control | Evidencia |
|---|---|---|---|---|
| Huecos GAP a 2/1, mundo de 93 costas | t4 **7** → candidato **0** | t4 **7** → candidato **0** | quitar agua → 9.936 GAP | `a11-fix-s3.txt`, `a11-fix-s2.txt` |
| GAP a 1:1, 9/8, 3/4 y 1/2, y tras cambios de zoom encadenados | 0 | 0 (1:1, 2/1) | ídem | `a11-world-zoomchain-s3.txt` |
| Cobertura por tesela frente al modelo tradicional, 2/1 (mutante, makie, daynight; luz 0/2/4) | 0 px | (camino independiente del backend) | modelo de la tesela vecina: 313.050 px | `a11-model-*.txt` |
| Cobertura frente a XR32 apagado, 8 zooms | 0 px | | desplazada una tesela: 1.119.556 px | `a11-coverage-a11-after.txt` |
| Color fuera de 2/1 (frente a testing-4) | byte-idéntico | | desplazada | `a11-identity-*.txt` |
| Color a 2/1 donde ambos dibujan | sin cambios | | | ídem |

## 3. Colores de empresa (TCC tras la integración)

| Prueba | SDL3 | SDL2 | Control | Evidencia |
|---|---|---|---|---|
| Caso real `station_track_Bahnsteig_2000_5`, X = P1 − P0 | **9,90** | **9,90** | testing-4: 0,47 (SDL3 consolid., SDL2 aquí) | `logs/tcc-analyze-*.txt` |
| Oráculo de zoom, fixture v2 (alfas 0–255; 1:1, 9/8, 2/1, 3/4, 1/2) | M_new, **0 px > 2** | M_new, **0 px > 2** | ponderación antigua: 1.792 / 870 / 288 px; sin semántica: MAE ≈ 6 | `results/zoom-oracle-*.json` |
| Alfa de 8 bits (1:1 y 2/1, sin mezcla) | MAE 0,075 | MAE 0,075 | | ídem |
| RGB ordinario sin recolorear (fila de control) | X = 0 | X = 0 | | ídem |
| Seis objetos Pak192.Comic: colocaciones y exactitud frente al PNG | idénticas a TCC-01 (11/10/4/5/4/6; 99,56…98,88 %) | idénticas | | `logs/tcc-analyze-*.txt` |
| SDL3 frente a SDL2, mismos volcados | 0 px fuera de la barra de estado (salvo contenido no determinista a 1/2 y la casilla del cursor) | | SDL2 contra sí mismo: 182 px a 1/2 | `evidence/tcc-s3-vs-s2.txt` |

## 4. DAY/NIGHT

| Prueba | SDL3 | SDL2 | Control | Evidencia |
|---|---|---|---|---|
| Oscurecimiento único de XR32 (fixture v2, niveles 0/1/2/4) | MAE 0,155 / 0,170 / 0,152 / 0,164 | idéntico | modelo «sin recolorear»: 25,5 / 19,5 / 15,2 / 8,9 | `results/night-oracle-*.json` |
| El motor de 16 bits conserva su doble oscurecimiento | encaja mejor con el modelo doble (mismo bin.) | | | ídem |
| Luces translúcidas de noche (nivel 4, fixture v1): luz menos su vecina ordinaria, por alfa | encendidas: +165 (255) / +161 (247) / +130 (200) / +82 (128); 16 bits: +170 / +172 / +137 / +87 | idéntico | testing-4: ≈0 entre los alfas 254 y 76 (apagadas) | `logs/night-lights.txt` |
| Costas DAY+NIGHT (puertas A4: makie, daynight 0–4) | PASS | PASS | mutante `stale` FALLA | `gates-t5-*.txt` |

## 5. Terreno, costas y caché A4

| Prueba | SDL3 | SDL2 | Control | Evidencia |
|---|---|---|---|---|
| Puertas A4/A4C, 8 configuraciones, frente a la caché A4C (testing-4) | **8/8 PASS** | **8/8 PASS** | `stale` FALLA (2.862 / 1.270) | `gates-t5-s3.txt`, `gates-t5-s2.txt` |
| — imágenes del pak (mar abierto, costas planas, textura) | 1.086/1.086 idénticas | ídem | | |
| — teselas derivadas fuera de 2/1 | todas idénticas | ídem | | |
| — teselas derivadas a 2/1 | solo cambia la cobertura (0 colores cambiados) | ídem | | |
| — XR32 apagado y pak sin XR32 | idénticos | idénticos | | |
| Oráculo de presentación (ventana = renderizador, fotograma a fotograma): costas ×6, primeros fotogramas, agua día/noche, **regiones sucias ×12** | **0 px** | **0 px** | `droprect` > 0 en cada fase | `logs/oracle-matrix.txt` |
| Oráculo de presentación con **zoom a 2/1 y teselas derivadas** (`zoomx`, coastworld + makie, fase TWZ) | **0 px** / 240 fotogramas | **0 px** / 240 fotogramas | `droprect`: 3,9 M y 2,9 M px | ídem |
| **Total del oráculo:** 86 ejecuciones (44 combinaciones del candidato), 0 rechazadas | | | | |

## 6. Seguridad y estabilidad

| Prueba | Resultado | Control | Evidencia |
|---|---|---|---|
| Batería de regresión de Windows: A1/A2/A3/A5/A6/N1/N5, puertas A/B, logcheck | igual que testing-4 / consolidación (11 líneas distintas, todas explicadas: control de suite ahora OK y contador `rezoomed` que varía) | una palabra cambiada se detecta | `battery-win-compare.txt` |
| Suites automáticas (309 tests) | **6/6 PASS** (rgba32 y 16 bits del candidato, 16 bits oficial; SDL2 y SDL3) | test inyectado: **FALLA `[1/310]`** | `logs/battery-win.txt` |
| AddressSanitizer de MSVC, SDL2, laboratorio t5 y base, 8 escenarios con el oráculo activo | **16/16, 0 informes**, 0 px en el oráculo | `asan_control.cpp`: desbordamiento detectado | `evidence/asan-win.txt` |
| Antivirus (Defender, bajo demanda) | binarios del candidato limpios | testing-4 SDL3 publicado: detectado | `evidence/defender-scan.txt` |

## 7. Rendimiento y memoria

Véase `PERFORMANCE.md`.
