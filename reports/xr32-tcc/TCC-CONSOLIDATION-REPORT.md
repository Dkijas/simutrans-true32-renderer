# SIMUTRANS-XR32-TCC-CONSOLIDATION-01 — Informe de consolidación

Fecha: 2026-11-02/03. Todo es **local**: no hay push, tag, release ni publicación; testing-4, A11/H1, SVN y los originales de Pak192.Comic quedan intactos. El candidato `03de7ed` **no se ha modificado**.

**Lo protegido, comprobado al cierre (3-nov, 00:45):**

| Elemento | Estado |
|---|---|
| testing-4 publicado | su `SHA256SUMS.txt` verifica completo |
| Pakset Pak192.Comic instalado | ningún fichero posterior al 5-ene-2025 |
| Clon del arte de Pak192.Comic | limpio (0 líneas de `git status`) en `a268b637`, el commit registrado |
| Carriles A4C, TCC-01 y testing-4 | 0 ficheros escritos desde el inicio de esta misión |
| Carril de validación de Pak192 | 1 fichero: el `.git/index` del clon, que reescribió mi propio `git status` de solo lectura (git refresca la caché de estado del índice); el contenido no cambió |
| SVN | no se ha tocado |

## 1. Identidad

| | |
|---|---|
| Base | testing-4 publicado `92ef4551` = lab `0afdfbd` (`a4/cache`) |
| Candidato congelado | `03de7edd6d0c0588c5b0953c286552c14ca47cad` (`tcc/fix`), 2 ficheros, +22/−7, sin cambio de formato |
| Compilación de laboratorio | `lab/tcc` `d89352b` = `lab/cache` (ganchos de A4C) + cherry-pick de 03de7ed; diff idéntico al del candidato (verificado) |
| Compilaciones | frescas, `SOURCE_DATE_EPOCH` fijo, recetas de A4C (`logs/builds-win.txt`, `logs/builds-linux.txt`) |

### Ejecutables (SHA-256, primeros 16)

| | Windows SDL3 | Windows SDL2 | Linux SDL2 | Linux SDL3 |
|---|---|---|---|---|
| producto 32 bits (candidato) | `1b4f22ed` | `00fd69a5` | `9e0dfa1b` | `68e0d47e` |
| **16 bits (candidato)** | **`8feaaf49` = testing-4/A4C** | **`20cbf7da` = A4C** | **`3e7cd80e` = testing-3/4** | **`9822aee1` = testing-3/4** |
| laboratorio (`lab/tcc`) | `5b8135be` | `d31df0aa` | `3825d9e7` | `b5cc2b8e` |
| ASan producto / laboratorio | — | — | `63c979be` / `c52c69ab` | `1c030130` / `95e13a4d` |
| makeobj | `8a150381` (= el entregado en TCC-01) | | | |

Los Windows SDL3 32 y 16 bits reproducen byte a byte las compilaciones del carril TCC-01. **Conjuntos de avisos idénticos a los de A4C**: Windows 26/26 en 4 compilaciones; Linux 18/18 (producto, 16 bits, laboratorio) y 20/20 (ASan) en 8 compilaciones (`evidence/warning-sets-linux.txt`).

## 2. Prueba crítica 1 — Zoom: oráculo numérico

**Oráculo** (`tools/zoom_oracle2.py`).

- **Independencia:** es independiente del código del motor, salvo en la geometría de muestreo, que es la del remuestreador legacy documentada en el propio código (columna i ← trunc((i·den + k − rem)/num), k < den).
- **Observable:** no depende del fondo. Es la misma imagen para dos empresas en el mismo fotograma: X = P1 − P0 = A·(c1 − c0)/255.
- **Modelos comparados**, para cada píxel de destino:
  - **M_new**, el candidato: ω = alfa en una clave translúcida, 255 en el resto;
  - **M_old**, testing-4: una muestra, un voto;
  - **M_ideal**, la media premultiplicada.

**Fixture v2** (`work/fixture2`, arte propio del laboratorio):
- claves del jugador 1 y del jugador 2 y un control ordinario, con alfa 0, 1, 32, 76, 128, 200, 247, 254 y 255, sobre un zócalo opaco (borde de vidrio contra estructura);
- tableros de 1 px: clave translúcida/ordinario opaco, clave translúcida/ordinario translúcido y clave opaca/ordinario translúcido;
- dos empresas: esquemas 0 y 1 (color 1) y 3 y 4 (color 2).

**Zooms:** 1:1, 9/8, 2/1, 3/4 y 1/2, en Windows SDL3 y capturas del propio motor (`evidence/zs3-*`).

| Frames | 1:1 | 9/8 | 2/1 | 3/4 | 1/2 |
|---|---|---|---|---|---|
| **Candidato** frente a M_new (MAE / px > 2) | 0,075 / **0** | 0,124 / **0** | 0,075 / **0** | 0,134 / **0** | 0,120 / **0** |
| Control: motor de testing-4 con paks nuevos (ponderación antigua) frente a M_new | 0,075 / 0 | 0,716 / **1.792** | 0,075 / 0 | 0,811 / **870** | 0,703 / **288** |
| Ese mismo control frente a M_old | | 0,122 / 0 | | 0,127 / 0 | 0,112 / 0 |
| Control: testing-4 con paks base (sin semántica) | 6,23 / 2.592 | 5,65 / 4.603 | 6,23 / 10.368 | 5,63 / 2.154 | 5,67 / 864 |

A 1:1 y 2/1 no hay mezcla (una muestra por eje), así que los tres modelos coinciden; esos zooms prueban que el color y el **alfa de 8 bits** pasan sin pérdida. **El oráculo detecta la ponderación antigua** en todos los zooms con mezcla.

**Por filas** (9/8, 3/4 y 1/2):
- **Vidrio contra estructura opaca** (fila 3): el candidato casa con M_new **y con el ideal** (0,13 / 0,19). La ponderación antigua da un error de 1,8–1,9.
- **Clave sola** (filas 0 y 1): casa con los dos (0,15 / 0,18).
- **Control ordinario** (fila 2): X = 0, nunca se recolorea.
- **Clave translúcida + ordinario translúcido** (fila 4): casa con M_new (0,18), pero **se aparta del ideal** (≈2,0, máximo ≈3,8).
- **Clave opaca + ordinario translúcido** (fila 5): **idéntico a testing-4**, y se aparta del ideal hasta ≈6.

Las filas 4 y 5 son una **aproximación heredada**: al normalizar las claves, el motor cuenta como 255 a los píxeles ordinarios translúcidos. En la fila 5, que es el caso que ya existe en los paks actuales, el candidato la conserva a propósito para que el material existente se dibuje byte a byte igual. Llegar al ideal cambiaría el zoom de testing-4 en cajas con translúcidos ordinarios, y eso es decisión del mantenedor (sección 9).

## 3. Prueba crítica 2 — DAY/NIGHT

### 3.1 ¿Por qué el tradicional oscurece dos veces?

Historial del trunk (`git log -S` en el espejo):

| Commit | Qué hizo |
|---|---|
| `2e9cb5d05` (prissi, r7788, 20-4-2016, «ADD: Transparent images») | el color de empresa translúcido se resolvía **al dibujar** desde `transparent_map_*[i]` = el color especial de la empresa con día/noche: **un oscurecimiento**. La tabla nocturna 3-4-3 solo era para translúcidos ordinarios |
| `c552c5115` (prissi, r7790, mismo día, «More thread safe way of switching transparent player colors») | para no conmutar tablas por jugador durante el dibujo (multihilo), el color se **incrusta** en `recode_img_src_target` desde `rgbmap_day_night`, ya oscurecido, y se guarda como 3-4-3. Al dibujar, el 3-4-3 pasa por la tabla nocturna de translúcidos, también oscurecida (`calc_base_pal_from_night_shift`, «again the same but for transparent colors»). **Segundo oscurecimiento** |
| `0e31ed019` (ceeac, r11003, 2023, foro 22569) | arregla el empaquetado de bits 565 → 3-4-3; conserva la fuente oscurecida |

**Conclusión:** el doble oscurecimiento **no es intencional**. Es una **consecuencia del orden de composición** introducida por una refactorización de seguridad entre hilos, es decir un defecto heredado. No hay justificación semántica para reproducirlo (decisión 6 del mantenedor). Los multiplicadores nocturnos son idénticos en `simgraph16` y en el hook XR32: RG = 0,75ⁿ y B = 0,83ⁿ, por (luz + 8)/8.

### 3.2 XR32 aplica la noche una sola vez

`tools/night_oracle.py` sobre las filas sólidas del fixture v2, con dos empresas a 1:1 y niveles 0, 1, 2 y 4 (`results/night-oracle.json`):

| Motor | n=0 | n=1 | n=2 | n=4 |
|---|--:|--:|--:|--:|
| **candidato frente al modelo «XR32, un oscurecimiento»** (sin parámetros libres), MAE | **0,155** | **0,170** | **0,152** | **0,164** |
| tradicional 16 bits: hipótesis «doble» / «simple» (alfa calibrado de día) | 3,75 / 3,75 | 6,05 / 6,44 | 4,80 / 5,02 | 3,54 / 5,14 |

El tradicional encaja mejor con la hipótesis doble en los tres niveles de noche. Sin embargo, el modelo del camino legacy (framebuffer 565, alfa de 31 niveles, mezcla de 5 bits) es tosco, con un residuo de 3,5–6 incluso de día, así que **la prueba del doble oscurecimiento es la del código y la del historial**, no este ajuste.

### 3.3 Luces especiales

Luminancia de la luz menos la del vecino ordinario, en el fixture v1 (`results/night-lights-levels.json`):

| | a=255 | a=200 | a=128 | a=76 |
|---|--:|--:|--:|--:|
| 16 bits n=1 / 2 / 4 | +65 / +111 / +170 | +56 / +92 / +137 | +35 / +58 / +87 | +21 / +34 / +50 |
| **candidato** n=1 / 2 / 4 | +60 / +106 / +165 | +47 / +83 / +130 | +29 / +52 / +82 | +18 / +32 / +49 |
| testing-4 n=2 / 4 | +106 / +165 | **0 / 0** | **−1 / −0,6** | **0 / 0** |

Las luces translúcidas se iluminan de noche en el candidato, en todos los niveles y en proporción al alfa. testing-4 las apagaba.

## 4. Prueba crítica 3 — Regresiones

| Arnés (de testing-4/A4C) | Resultado | Control | Evidencia |
|---|---|---|---|
| **A4 y caché de costas** (coastgate, gdb, 8 configuraciones: mutante con XR32 activo y apagado, makie 5 zooms × 3 niveles × 3 etapas, DAY+NIGHT 2 × 5 × 4, pak128.german sin XR32, imágenes del mar) | **11.247/11.247 dibujos byte-idénticos** a la caché A4C (testing-4) | el mutante `stale` difiere en 2.862 y 1.270 dibujos; los dibujos vecinos difieren | `evidence/gates-tcc.txt` |
| **Oráculo de presentación Windows** (ventana = renderizador, cada fotograma): costas ×6, primeros fotogramas, agua DAY+NIGHT, **regiones sucias** ×12 (`static`, `vehicles`, `scroll`, `zoom`, `windows`, `daynight`, `scatter`, `fullredraw`, `resize`, `playercolor`, `zoomx`, `edges`) | **0 px en las 41 combinaciones**, SDL2 y SDL3 (82 ejecuciones, 0 rechazadas) | el mutante `droprect` es > 0 en cada fase | `evidence/BENCHMARK-MATRIX-win.csv`, `out/TW*` |
| **Oráculo de presentación Linux** (WSLg, **no Linux nativo**): SDL3 `dummy`; SDL2/SDL3 `x11` y `wayland`; costas ×6, primeros fotogramas, DAY+NIGHT, regiones sucias ×12 (x11) | **0 px** en todas las celdas salvo 3, todas de SDL3 + X11: `coastfull`, `coastresize`, `resize`. Esas 3 solo fallan en los fotogramas de cambio de tamaño (3 / 8 / 8), que **testing-3 y la caché A4C fallan idénticamente en la misma sesión** (fase suplementaria TLS), y dan 0 px en todos los demás fotogramas. Es la misma excepción que A4C ya documentó: el oráculo no ve esos fotogramas en X11/WSLg. 130 ejecuciones aceptadas, 0 rechazadas | el mutante `droprect` falla en todas las fases; en los escenarios de cambio de tamaño falla en 62–221 fotogramas fuera de los de cambio | `evidence/BENCHMARK-MATRIX-linux.csv`, `logs/oracle-matrix.txt`, `evidence/switch-frames.txt`, `evidence/switch-frames-main-phase.txt` |
| **Sanitizadores** ASan/UBSan/LSan, Linux SDL2/SDL3, 20 ejecuciones (producto en `worldcat`/`worldcoast`; laboratorio en 6 costas + DAY+NIGHT + makie de noche, oráculo activo) | **20/20 PASS**: ASan 0, UBSan sin ubicaciones nuevas (4–8 = A4C), LSan sin orígenes nuevos (18–19 = A4C) | una línea UBSan sintética sale como NUEVA | `evidence/asan-sets-tcc.txt`, `logs/asan-tcc.txt` |
| **Batería Windows** (A1/A2/A3/A5/A6/N1/N5, gates A/B, suite), SDL2 + SDL3 | 240 líneas, **las mismas comprobaciones y los mismos veredictos que A4C** una vez normalizados hashes, fechas y rutas. Las únicas líneas que difieren son dos:<br>1. **Suites:** en A4C murieron por una ruta rota y se repitieron aparte; aquí se arregló la ruta (`work/final-src/tests` = `git archive tcc/fix tests`) y las 6 suites dan **PASS 309/309**, igual que los veredictos de A4C.<br>2. **Contador de imágenes re-escaladas del gate I** (`catworld`, SDL2: 210/49/559 frente a 209/46/545): es carga de trabajo, ya variaba entre corridas anteriores del mismo arnés (203–210, 543–560, 46/49) y la combinación exacta 210/558/49 ya salió antes del candidato. La aserción («dibujado donde el tradicional oculta» = 0) da `ok` en ambos | el control de la suite (un test que falla adrede) **FALLA `[1/310]`**, igual que en A4C; la comparación de logs detecta una palabra cambiada | `evidence/battery-win-compare.txt`, `logs/battery-win.txt`, `logs/suite-control-rerun.txt`, `out/suite-*/verdict.txt` |
| **Batería Linux** (WSLg), SDL2 + SDL3: paquete, A1/A2/A5/A6, logcheck, suites | 68 líneas, **0 líneas distintas de A4C** tras normalizar; suites **PASS 309/309** en rgba32 y 16 bits de ambos backends (exes `9e0dfa1b`/`68e0d47e`, 16 bits `3e7cd80e`/`9822aee1`); tests idénticos a `tcc/fix` | controles de suite **FALLAN `[1/310]`** en SDL2 y SDL3; la comparación detecta una palabra cambiada | `evidence/battery-linux-compare.txt`, `logs/battery-linux.txt` |
| **K/M** (apagado limpio + memoria pico), Windows SDL2/SDL3, XR32 activo/apagado, `cat` y `coast`, n = 3 intercalado con testing-4 publicado | **48/48 PASS** (salida 0, 0 líneas FATAL/ERROR, nodos XR32 omitidos iguales: 0/154/192). Memoria pico: candidato − testing-4 entre **+0,0 y +0,5 MB** en las 8 celdas, con una dispersión entre repeticiones de hasta 6,6 MB → **sin diferencia medible** | | `evidence/shutdown-mem-tcc.csv`, `evidence/shutdown-mem-summary.txt` |
| **Identidad 16 bits** | **byte-idénticos** en las 4 plataformas (sección 1) | | |
| **Compatibilidad de paksets** | sección 6 | | |

## 5. Prueba crítica 4 — Plataformas

| Plataforma | Producto | Laboratorio | Visual (escena Pak192 + fixture) |
|---|---|---|---|
| Windows SDL3 | compilado; coastgate, batería, K/M | oráculo 0 px | zoom y noche (sección 2/3); seis objetos en TCC-01 con el mismo exe `1b4f22ed` |
| Windows SDL2 | compilado; batería, K/M | oráculo 0 px | **escena completa y fixture a 1:1, sin teclado ni foco** (PrintWindow sobre la propia ventana, ver abajo): con la ventana de aviso cerrada, **byte-idéntica a Windows SDL3** fuera de la barra de estado y del cursor de herramienta; vidrio crítico X = 9,9 (testing-4 SDL2: 0,47); métricas de los seis objetos **idénticas** a SDL3 |
| Linux SDL2 (**WSLg, no Linux nativo**) | compilado; ASan; batería | oráculo x11 + wayland: 0 px en todas las celdas (el driver `dummy` de SDL2 es un carril VACÍO conocido: no se ejecuta) | **sin captura visual automática**: el gancho de volcado solo existe en SDL3 |
| Linux SDL3 (**WSLg, no Linux nativo**) | compilado; ASan; batería | oráculo dummy + x11 + wayland: 0 px salvo los fotogramas de cambio de tamaño de X11, idénticos a testing-3 | **escena completa: fotograma byte-idéntico al de Windows SDL3** (0 de 3.993.600 px en el área de mundo, quitando la ventana de aviso); vidrio crítico X = 9,9, igual que en Windows |

## 6. Compatibilidad

| Caso | Resultado |
|---|---|
| PAK anteriores con el motor corregido | idénticos a testing-4 a 1:1, 9/8 y 3/4 (TCC-01, BF frente a T4); coastgate 11.247/11.247 |
| PAK corregidos con testing-4 | 1:1 recoloreado correcto; con zoom, ponderación antigua (oráculo: casa con M_old) |
| Formatos sin datos XR32 | pak128.german plano: 1.560 dibujos idénticos; XR32 apagado: idéntico; 16 bits: binario idéntico |
| Colores de empresa opacos | sin cambios (registros 39.041/39.041 en ambos; claves opacas del fixture recoloreadas igual) |
| Colores de empresa translúcidos | recoloreados (fixture y `station_track_Bahnsteig_2000_5`); alfa de 8 bits |
| Formato de nodos XR32 | **sin cambios** (mismo nodo v1; 0 nodos no-XR32 cambiados; el escritor solo añade registros) |

## 7. Los seis objetos de Pak192.Comic

Windows SDL3 (TCC-01, mismo exe `1b4f22ed` y mismos paks): exactitud frente al PNG **idéntica a la certificada** en los seis (99,56 / 99,48 / 99,98 / 97,88 / 98,07 / 98,88 % con XR32 activo). Linux SDL3: fotograma byte-idéntico al de Windows. Windows SDL2: con la ventana de aviso cerrada, las métricas de los seis objetos son **idénticas campo a campo** a las de SDL3 con XR32 activo y apagado (colocaciones 11/10/4/5/4/6, exactitud 99,56 / 99,48 / 99,98 / 97,88 / 98,07 / 98,88 %); solo difieren los campos del brazo de 16 bits, porque la captura de ventana y la captura del motor expanden RGB565 a 888 de forma distinta (±1, nunca 2; a precisión 565 son idénticos). `results/sdl2-object-metrics-closed-FX.json`.

### 7.1 Cómo se capturó Windows SDL2 sin teclado ni foco

SDL2 solo recibe la tecla de captura del motor si su ventana tiene el foco, y el usuario no estaba. En lugar de quitarle el foco:

1. el escenario construye la escena y escribe «STLAB DONE»; se esperan 12 s;
2. un clic (move+down+up seguidos, `PostMessage` a la ventana del propio PID) cierra la ventana de objetos ausentes que abre la partida guardada; no se mueve el cursor real;
3. `PrintWindow(PW_RENDERFULLCONTENT)` copia el área cliente de esa ventana.

El método se validó primero en SDL3 contra la captura del propio motor: sin el clic, las dos imágenes solo difieren dentro de la ventana de aviso.

`tools/pw_compare.py` (`evidence/pw-sdl2-compare.txt`, `pw-sdl2-closed-compare.txt`) exige que la imagen sea idéntica salvo dos cosas:
- la barra de estado: el reloj del juego avanza, y aparecen las coordenadas de la casilla bajo el ratón;
- un único rectángulo del tamaño de la ventana de aviso. Con la ventana abierta, ese rectángulo es la ventana (431 × 386); con la ventana cerrada, es el cursor de herramienta que SDL dibuja donde está el ratón real, una casilla de 192 × 98 sobre hierba sin objetos.

Con la ventana abierta tapaba parte de `Bahnhof_Hochbahn_5` y de `station_track_Bahnsteig_2000_5`, y por eso se repitió con el clic.

Los controles:
- un píxel cambiado fuera del rectángulo hace fallar la comparación;
- un paso RGB565 hace fallar la comparación de 16 bits.

| Comparación (SDL2 PrintWindow frente a…) | Resultado |
|---|---|
| fixture 1:1, SDL3 PrintWindow (mismo método) | idéntico salvo la ventana de aviso, que SDL3 coloca 24 px más abajo |
| fixture 1:1 candidato y testing-4, captura del motor SDL3 | idéntico salvo la ventana |
| escena completa: 16 bits (a 565), XR32 apagado, XR32 activo y testing-4, ventana cerrada | **9.616 px, todos en la casilla del cursor**; resto idéntico |

El zoom de SDL2 no se capturó (necesita teclas). Lo cubre el oráculo de presentación SDL2: `zoom`, `zoomx` y `coastzoom` dan 0 px en Windows y en Linux x11/wayland.

## 8. Limitaciones

1. **Linux es WSLg, no Linux nativo.** Debian 13 en WSL 2: `x11` y `wayland` son el compositor de WSLg. No se probaron drivers ni compositores Linux nativos (Mesa sobre hardware, GNOME/KDE, X.Org real).
2. **Una sola pantalla** (3440×1440). No se valida el comportamiento multimonitor.
3. **macOS y Android no se probaron**, porque el encargo pedía cuatro plataformas y aquí no hay ese hardware. Los cambios están en el renderizador común y en makeobj, no en código de plataforma; aun así, eso no es una prueba.
4. **Fotogramas de cambio de tamaño en SDL3 + X11.** El oráculo no puede verificarlos (3 en `coastfull`; 8 en `coastresize` y `resize`), y testing-3 y la caché A4C los fallan igual. Fuera de ellos, 0 px.
5. **Linux SDL2 no tiene captura visual de la escena Pak192**, porque el gancho de volcado solo existe en SDL3. Lo cubren el oráculo x11/wayland (0 px), los sanitizadores y la batería.
6. **Windows SDL2 no tiene captura de los niveles de zoom**, porque necesitan teclas y por tanto foco. Lo cubre el oráculo SDL2: `zoom`, `zoomx` y `coastzoom` dan 0 px.
7. **Residuo del zoom: decisión del mantenedor.** Cuando una clave de color de empresa se mezcla con píxeles ordinarios TRANSLÚCIDOS en la misma muestra, el resultado se aparta de la media premultiplicada ideal:
   - clave translúcida con ordinario translúcido: ≈2 de media, 3,8 como máximo;
   - clave opaca con ordinario translúcido: hasta ≈6, **igual que testing-4**, es decir, heredado.

   El candidato no cambia la geometría A1 ni el formato. Corregirlo exigiría ponderar también el canal ordinario, lo que cae fuera del parche mínimo.
8. **El oscurecimiento doble del motor de 16 bits está documentado, no corregido.** La decisión 5 prohíbe tocar ese motor. Por eso, de noche, XR32 (oscurecimiento simple, decisión 6) y 16 bits difieren en los colores de empresa translúcidos **por diseño**.
9. **El modelo «doble» del motor tradicional es aproximado.** Encaja mejor que el simple, pero la prueba de su origen es el código y el historial (c552c5115, r7790), no el ajuste numérico.
10. **Lo que excluye la comparación PrintWindow:** la barra de estado (reloj, coordenadas) y la casilla del cursor de herramienta. Es lo único que excluye.
11. **Licencia de Pak192.Comic** (CC BY-NC-SA 3.0). Las capturas y los paks compilados con su arte se quedan en el carril; el paquete solo lleva el fixture propio.
12. **No se afirma nada sobre rendimiento.** Solo se midió la memoria pico K/M, con n = 3 y comparando con testing-4 (sección 4).

## 9. Veredicto

**B — CERTIFIED WITH LIMITATIONS.**

| | |
|---|---|
| Candidato certificado | `03de7edd6d0c0588c5b0953c286552c14ca47cad` sobre testing-4 (`0afdfbd`), **sin modificar** (diff idéntico al entregado en TCC-01: `deliver/patch/PATCH-IDENTITY.txt`) |
| Preparación | **READY_FOR_APPROVAL** |
| Publicación | **NOT_AUTHORISED**: este encargo no la autoriza |

Todas las pruebas que pidió el encargo pasan, y cada una ha demostrado antes que podía fallar:

- **Zoom.** El candidato casa con su modelo (0 px > 2) en todos los zooms. El oráculo detecta la ponderación antigua y la pérdida de semántica. El alfa de 8 bits pasa intacto a 1:1 y 2/1.
- **Noche.** XR32 oscurece **una sola vez**, en todos los niveles. El doble oscurecimiento del motor tradicional es un efecto de r7790 (2016, seguridad de hilos), no una intención de diseño. Las luces translúcidas vuelven a encenderse.
- **Regresiones.**
  - A4 y la caché de costas: 11.247 dibujos idénticos.
  - Oráculos de presentación: 212 ejecuciones aceptadas, 0 px fuera de los fotogramas de cambio de tamaño no verificables, que testing-3 falla igual.
  - Sanitizadores: 20/20.
  - Baterías Windows y Linux: las mismas que en A4C; suites 309/309.
  - K/M: 48/48, sin diferencia de memoria medible.
  - 16 bits: byte-idéntico.
  - Conjuntos de avisos de compilación: idénticos.
- **Plataformas.** Windows SDL2 y SDL3 y Linux SDL2 y SDL3 (WSLg) compilan y pasan. La escena Pak192 sale byte-idéntica entre Windows SDL3, Windows SDL2 y Linux SDL3.
- **Compatibilidad.** Los paks antiguos se dibujan igual; los paks nuevos funcionan en testing-4; no cambia el formato de nodos.

**Por qué B y no A:**
1. **El residuo heredado del zoom**, cuando una clave de color de empresa se mezcla con píxeles ordinarios translúcidos (sección 2, filas 4 y 5). Queda pendiente de una decisión del mantenedor.
2. **El alcance de plataformas:** WSLg y no Linux nativo; ni macOS ni Android; y en Linux SDL2 no hay captura visual directa (sección 8).

Ninguna de las dos es una regresión respecto a testing-4.

**Recomendación de publicación**, si el mantenedor la autoriza:

1. **Publicar `03de7ed` como corrección del motor XR32 y de makeobj**, sin cambio de formato. Los paks existentes no cambian ni un byte en pantalla.
2. Recompilar con el makeobj corregido los paksets que usan **colores de empresa translúcidos** (en Pak192.Comic, `station_track_Bahnsteig_2000_5`). Hasta entonces se ven como en testing-4. Un pak recompilado se ve bien en testing-4 a 1:1, y con zoom usa la ponderación antigua, sin romper nada.
3. **Decisiones que quedan para el mantenedor**, ninguna de ellas bloqueante:
   - si se aproxima la media premultiplicada ideal en las filas 4 y 5;
   - si se corrige alguna vez el doble oscurecimiento de 16 bits; la decisión 5 lo prohíbe en este encargo.
4. Antes de una release, hace falta una prueba en Linux nativo y en macOS. Aquí no se pueden hacer.
