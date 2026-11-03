# Rendimiento y memoria — testing-5 frente a testing-4 (Windows x64)

## Método

**Brazos** (mismo código de laboratorio y mismos contadores):
- **t4:** `w-base` = `lab/cache` `7fe351e`, el laboratorio de A4C del testing-4 publicado. SDL3 `209f0fbd`, SDL2 `459daf44`.
- **t5:** `w-t5` = `lab/t5` `b762fef` (`lab/cache` + TCC + A11). SDL3 `f9e42af8`, SDL2 `53809471`.

**Medición:**
- Las ejecuciones se intercalan y rotan, sin oráculo, con la máquina **libre**: se comprobó al arrancar que había 0 procesos del carril.
- Cada ejecución da `busy = frame_ms − sleep_ms` por fotograma, y de ahí su P50, P95 y P99.
- Se rechaza una ejecución si el hash del ejecutable no coincide, si rc ≠ 0, si el oráculo está activo, si el escenario no es el pedido o si tiene menos de 600 filas.

**Lectura** (la misma regla que A4C):
- Se calcula la mediana por brazo y la diferencia emparejada por repetición.
- La dispersión de referencia es máximo − mínimo de las ejecuciones de testing-4.
- Solo se afirma una diferencia si **todas** las diferencias emparejadas tienen el mismo signo **y** la mediana supera esa dispersión; si no, «sin diferencia medible». Con n < 3 no se afirma nada.

Análisis: `tools/perf_t5.py` → `evidence/perf-t5-summary.txt` y `evidence/perf-t5-rows.csv` (todas las cifras).

**Ejecuciones: 188 aceptadas, 0 rechazadas.**

| Fase | Mundo | Resolución | n | Escenarios |
|---|---|---|---|---|
| PB | coastworld + agua makie (2.080 teselas de costa derivadas) | 1920×1080 | 5 | coast, coastzoom, coastnight, coastscroll |
| PH | ídem | 2560×1440 | 3 | ídem |
| PC | oepnv1950 + cat | 1920×1080 | 5 | zoom, daynight |
| **PZ** | coastworld + makie | 1920×1080 | 5 | **zoomx**: llega a **2/1**, el único zoom donde actúa A11 |

`coastzoom` solo alterna 1:1 ↔ 9/8, así que **no** ejercita A11. El coste del código nuevo lo mide **PZ**: 16 pasos hacia dentro hasta 2/1 y 16 hacia fuera, en cada paso de zoom se reconstruye la caché de las teselas derivadas.

## Tiempo de fotograma (ms de trabajo por fotograma, medianas)

| | SDL3 t4 | SDL3 t5 | SDL2 t4 | SDL2 t5 | Lectura |
|---|---|---|---|---|---|
| PB coast P50 / P99 | 3,54 / 6,26 | 3,55 / 6,14 | 3,89 / 5,94 | 3,90 / 5,92 | sin diferencia medible |
| PB coastzoom P50 / P99 | 4,13 / 40,95 | 4,11 / 40,89 | 4,24 / 40,32 | 4,25 / 40,60 | sin diferencia medible |
| PB coastnight P50 / P99 | 4,01 / 9,47 | 4,04 / 9,39 | 4,10 / 8,69 | 4,13 / 8,71 | sin diferencia medible |
| PB coastscroll P50 / P99 | 4,96 / 6,66 | 4,89 / 6,95 | 4,55 / 6,58 | 4,55 / 6,49 | sin diferencia medible |
| PC zoom P50 / P99 | 2,19 / 20,46 | 2,16 / 20,42 | 2,59 / 20,91 | 2,65 / 21,12 | sin diferencia medible |
| PC daynight P50 / P99 | 3,01 / 6,00 | 3,04 / 5,90 | 2,29 / 5,31 | 2,27 / 5,10 | sin diferencia medible |
| **PZ zoomx (2/1) P50 / P95 / P99** | 6,17 / 75,7 / 194,7 | 6,13 / 74,9 / 198,7 | 6,14 / 75,3 / 197,7 | 6,15 / 75,0 / 198,9 | **sin diferencia medible** (P99 t4: dispersión 79 ms en SDL3 y 6 ms en SDL2) |

**Celdas con diferencia según la regla: 2 de 66, en sentidos opuestos.**
- PH coastnight P99, **SDL2**: t5 **1,18 ms más rápido**.
- PH coastnight P99, **SDL3**: t5 **0,42 ms más lento**.

Las dos son de 1440p con n = 3, y caen en un escenario (cambios de nivel de luz, sin zoom a 2/1 ni colores de empresa) donde ningún cambio ejecuta código distinto. Con 66 comparaciones y signos opuestos es lo que cabe esperar del ruido. **No se afirma ninguna diferencia de rendimiento.**

## Coste de cambiar de zoom

- `PZ` (pasa por 2/1 y reconstruye con el constructor nuevo): P99 sin diferencia medible.
- `coastzoom` y `PC zoom`: sin diferencia medible.

El constructor A11 reserva por píxel de destino 1 byte de estado más 8 bytes de índices, solo mientras construye. Es algo menos que lo que reserva `stlab_xr32_zoom_build()` para la misma tesela (seis acumuladores de 32 bits por píxel).

## Caché A4 (contadores por fotograma, SDL3, 600 fotogramas, una ejecución por escenario)

| Escenario | texturas de estadio construidas t4 → t5 | composiciones t4 → t5 |
|---|---|---|
| coast | 13 → 13 | 433 → 433 |
| coastnight | 13 → 13 | 1.258 → 1.269 |
| coastscroll | 13 → 13 | 394 → 393 |
| coastzoom | 14 → 14 | 1.202 → 1.180 |

La caché de texturas por estadio de A4C se comporta igual. El número de composiciones depende del ritmo de cada ejecución (n = 1, ±2 %); de ellas no se saca ninguna conclusión de tiempos (`evidence/derive-counters-summary.txt`).

## Memoria pico y apagado limpio (K/M)

Ejecutables de producto: t5 (`e82a2ce8` SDL2, `f7f7ce82` SDL3) frente a testing-4 **publicado** (`42e9089c`, `43420d63`). El juego avanza unos 2 meses y se cierra solo (`-until`). n = 3, intercalado, con dos cargas: `cat` y `coast` (2.080 teselas derivadas).

- **48/48 PASS**: código de salida 0, ninguna línea FATAL ni `Error:`, y los mismos nodos XR32 omitidos (0 / 154 / 192).
- **Memoria pico de trabajo:** sin diferencia medible en 7 de 8 celdas. La octava (`coast`, SDL2, XR32 activo) da t5 0,2 MB **más bajo** (522,6 → 521,9 MB, un 0,04 %), sin importancia práctica.

Evidencia: `evidence/shutdown-mem-t5.csv`, `evidence/shutdown-mem-summary.txt`.

## DAY/NIGHT

`coastnight` (cambios de nivel de luz con agua de costa) y `PC daynight`: sin diferencia medible. Las regiones sucias de los cambios día/noche están verificadas a 0 px por el oráculo de presentación (`WINDOWS-TEST-MATRIX.md`, sección 5).

## Ruido

En esta máquina el P99 de los escenarios de zoom varía entre ejecuciones de testing-4 hasta 79 ms en SDL3 (`zoomx`) y 13 ms en SDL2 (`PC zoom`, P95). Por eso las cifras de P99 de zoom solo pueden afirmar diferencias grandes, y no hay ninguna.
