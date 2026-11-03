# SIMUTRANS-XR32-TRANSLUCENT-COMPANY-COLOUR-01 — Informe

Fecha: 2026-11-02. Todo es **local**: no hay push, tag, release, SVN ni publicación, y no se han tocado testing-4, A11/H1 ni los originales de Pak192.Comic.

## Veredicto

**B — CERTIFIED WITH LIMITATIONS.**

| | |
|---|---|
| Preparación | **READY** como candidato local, con las limitaciones de la sección 10 |
| Publicación | **NOT_AUTHORISED** |

La corrección es funcionalmente correcta:
- los colores especiales translúcidos recuperan su semántica dinámica (empresa de día y de noche, y luces de noche);
- el alfa conserva sus 8 bits;
- la pintura RGBA ordinaria no se convierte en color de empresa;
- **no hay cambio de formato**.

Todos los paks y todas las imágenes existentes se dibujan **byte a byte igual** que en testing-4.

No es A por tres motivos:
1. **De noche** el candidato no imita al motor tradicional, porque este **oscurece dos veces** los colores de empresa translúcidos. Es un defecto previo suyo (sección 9).
2. **La nueva ponderación del zoom** está demostrada neutra para todo el material existente, pero sobre material nuevo solo tiene evidencia analítica y visual.
3. **Costas A4, regiones sucias y memoria** se cubren con argumentos de diff disjunto e identidad de fotogramas, no con sus propios arneses.

## 1. Base y candidato

| | |
|---|---|
| Base | testing-4 publicado: commit público `92ef4551` = lab `0afdfbd`. El tarball de fuentes publicado coincide con `git archive 0afdfbd` (2.231 ficheros, 0 diferencias, con control). Los binarios publicados verifican contra su `SHA256SUMS` |
| Candidato | lab `03de7edd6d0c0588c5b0953c286552c14ca47cad` (rama local `tcc/fix`), +22/−7 en 2 ficheros. El diff (`patch/XR32-TRANSLUCENT-COMPANY-COLOUR-01.diff`, LF) se aplica limpio sobre una exportación fresca de la base y da `tcc/fix` byte a byte |
| Compilación | fresca, MSYS2 MinGW-w64, Release, MT, SDL3, revisión 12377, `SOURCE_DATE_EPOCH` fijo. **La base recompilada reproduce byte a byte el `simutrans-rgba32` publicado (`43420d63`)**, así que la cadena es exacta. El makeobj base recompilado (`fce1d439`) produce **paks idénticos** a los del makeobj publicado (17/17) |
| Ejecutables candidatos | `candidate/simutrans-rgba32-tcc01-win64-sdl3.exe` `1b4f22ed…`; `candidate/makeobj-tcc01.exe` `8a150381…`. El 16 bits base y el candidato son **byte-idénticos** (`8feaaf49`) |
| Avisos de compilación | los conjuntos son idénticos entre base y candidato (makeobj 2, juego 26); ninguno en los ficheros tocados |

## 2. Reproducción, antes de cambiar el código

- **Datos:** el makeobj de testing-4 da 0 de 140.076 claves translúcidas con registro en `station_track_Bahnsteig_2000_5` (`results/base-runs-2000_5.json`). Sus paks son idénticos a los del laboratorio certificado.
- **Juego, con testing-4:** mismo sprite, mismas posiciones y 11.440 píxeles de vidrio. La diferencia entre empresas es 8,99 (16 bits), 8,75 (XR32 sin datos) y **0,47** (XR32 con datos): las cifras certificadas, con la misma métrica y en las mismas condiciones (`screenshots/repro-*`).

## 3. Causa raíz: el píxel, del PNG al framebuffer

| Etapa | Código | Qué ocurre con una clave exacta con alfa 8–254 |
|---|---|---|
| 1. Clasificación legacy | `image_writer.cc` `pixrgb_to_pixval` | se conserva como especial: `0x8020 + i*31 + alfa` (8–247) o `0x8000 + i` (248–254) |
| 2. Extracción XR32 | `stlab_semantic.cc` etapa 1 | **solo emitía registro si `alpha == 255`**. Un píxel con `7 < a < 255` "takes part in nothing" (heredado del oráculo Python) → **sin registro** |
| 3. Serialización | `stlab_xr32_writer.cc` | codifica los registros que recibe, sin mirar el alfa; el plano RGBA se guarda exacto |
| 4. Lectura | `stlab_xr32.cc` parser | sin supuestos de opacidad |
| 5. Recoloreado (1:1) | `stlab_xr32_resolve` | pinta el RGB del plano; un registro fija el color **y conserva el alfa del plano** |
| 6. Alfa y fondo | hook y blitters | compone el ARGB con alfa directo |
| 5b. Zoom nativo | `stlab_xr32_zoom.cci` | **segundo supuesto de opacidad**: cada muestra clave pesa su cobertura sin alfa, normalizada por el número de muestras, y los píxeles normales pesan su alfa |

**Diferencia entre v1 y v2.** En v2 (máscaras explícitas, `semantic_masks=1`) la semántica la declaran las máscaras: una clave exacta sin máscara es pintura ordinaria por diseño, y el solver ya acepta píxeles base translúcidos. En v1 la semántica se deduce de la clave exacta, igual que en el formato tradicional. El fallo era solo de v1. Lo mismo vale para objetos con `night_images=1` y sin máscaras (prototipo de prissi), que no se tocan.

## 4. Diseño y alternativas

| Opción | Decisión |
|---|---|
| **A. Extractor:** registro de cobertura 255 para toda clave exacta que no sea hueco (alfa 8–254 incluido); la reconstrucción de bordes no cambia | **elegida**. Paridad exacta con `pixrgb_to_pixval` y sin cambio de formato. Los motores sin el arreglo leen los registros: el resolver 1:1 ya conservaba el alfa |
| **A2. Zoom:** ω = alfa para las muestras clave translúcidas y 255 para el resto; claves normalizadas por Σω | **elegida**, porque sin ella el borde vidrio/estructura sobrepondera la clave. Para claves opacas Σω = 255·muestras y el redondeo entero es idéntico; la prueba está en el comentario del código y la verificación empírica en la sección 8 |
| B. Respaldo en el runtime que busque claves en el plano | rechazada: más invasiva, contradice "semantics are never inferred from RGB" de v2 y no arregla los paks en motores viejos |
| C. Cambio de formato (nodo nuevo, alfa por registro) | innecesaria: el formato ya separa color (registro) y alfa (plano) |
| Usar las translúcidas como semillas de la reconstrucción de bordes | rechazada: cambiaría registros de píxeles opacos vecinos en paks existentes |

## 5. Evidencia de serialización

`results/serialization-evidence.json`. Base frente a candidato, sobre 17 paks:

| Pak | Cambios |
|---|---|
| 15 de 17 | **ninguno, byte a byte**. Incluye todos los que tienen translúcidos ordinarios (Hochbahn_5, Halle_2000, 1850 hall) |
| `station_track_Bahnsteig_2000_5` | 16 nodos XR32 cambian (los 16 `FrontImage`); 0 nodos no-XR32; **0 planos RGBA**. Claves translúcidas con registro: 0 → **140.076/140.076** (clase jugador 1). Claves opacas: 39.041/39.041 en ambos. Registros de borde: 404 en ambos |
| fixture | claves translúcidas con registro: 0 → **6.912/6.912** (jugador 1, jugador 2 y luz, 2.304 cada una). Claves con alfa ≤7: 0. **Controles de RGB vecino a la clave: 0 de 7.776**. Ordinarios: 0 |

El pak base hace de control negativo del gate (0/140.076).

## 6. Pruebas en el juego

Escena Squirrel reproducible: 39 estaciones de Pak192.Comic, el caso crítico para dos empresas y el fixture para dos empresas. Captura del propio motor a 3200×1300, con foco verificado en cada tecla. Son 14 ejecuciones, todas con salida 0.

Los esquemas detectados son el jugador 0 = 0 (clave) / 3 (clave de color 2) y el jugador 1 = 1 (gris) / 4 (naranja): **dos configuraciones de color distintas por canal**.

### Caso crítico, antes y después

| | testing-4 | candidato |
|---|--:|--:|
| Diferencia entre empresas, XR32 con datos (esperado al recolorear: 9,28) | 0,47 | **9,90** |
| Misma casilla: MAE H-regresión / H-correcto | 1,16 / 9,40 | 9,31 / **1,07** |
| Color estimado de la capa de vidrio de la empresa gris | (63, 100, 134), clave azul | **(114, 113, 115)**; esquema (111, 111, 111) |
| ΔE00 de los especiales (XR32 con frente a sin datos), media / p99 | 2,97 / 12,8 | 2,16 / 7,5 |

### Fixture, de día (`results/fixture-day.json`)

- **Claves translúcidas:**
  - testing-4 no las recolorea (control);
  - el candidato sí, y su **alfa efectivo sigue al PNG**: 248→248,8, 247→247,0, 200→199,6/200,2, 128→127,5/128,6, 76→76,0/75,7, 32→31,9/32,2;
  - el motor tradicional cuantiza a 31 niveles y vuelve opacos los alfas 248–254.
- **Claves opacas:** se recolorean en todos los modos.
- **Alfa ≤7:** hueco en todos.
- **Controles de RGB vecino:** no se recolorean nunca. Un parche con alfa 8 muestra el mismo fondo en los seis modos.
- **Fila de luz:** no varía por empresa.

### Noche (`daynight_level=4`, `results/night-lights.json`)

Luminancia de la luz menos su vecina ordinaria:

| Modo | a=254 | a=200 | a=128 | a=76 |
|---|--:|--:|--:|--:|
| 16 bits | +170 | +137 | +87 | +50 |
| testing-4 XR32 | **0** | **0** | **−0,6** | **0,1** |
| candidato XR32 | +165 | +130 | +82 | +49 |

**El defecto también apagaba las luces translúcidas de noche**, y el candidato lo corrige. El color de empresa translúcido de noche se recolorea en el candidato; sobre la diferencia con el tradicional, ver la sección 9.

### Los seis objetos de Pak192.Comic

Exactitud frente al PNG con XR32 activo, sin datos y en 16 bits: **idéntica a la certificada** en los seis, en testing-4 y en el candidato (por ejemplo 99,56 / 39,94 / 39,94 en 2000_5 y 99,48 / 76,49 / 56,77 en Hochbahn_5). Los cinco objetos sin claves translúcidas son byte a byte iguales a testing-4.

## 7. Alcance del cambio en pantalla

1:1, XR32 con datos: el candidato cambia 35.764 píxeles respecto a testing-4, **todos dentro de las casillas propias de `2000_5` y del fixture; 0 fuera**. Control con la referencia equivocada: 267.006 fuera.

Con el jugador 0 (esquema clave) no hay cambio visible, como es lógico: su color es la clave.

## 8. Matriz de regresiones (`results/mx-frames.json`)

| Regresión | Resultado |
|---|---|
| **16 bits** | binario byte-idéntico; fotogramas FX = T4 a 1:1 y 9/8 |
| **XR32 sin datos** | fotogramas FX = T4 a 1:1, 9/8 y 3/4 |
| **Compatibilidad de PAK: paks viejos en el motor nuevo** (BF frente a T4, XR32 con datos) | **idénticos a 1:1, 9/8 y 3/4** |
| **Compatibilidad de PAK: paks nuevos en el motor viejo** (PF) | idénticos al candidato a 1:1; el motor viejo los recolorea bien. Con zoom aplica su ponderación antigua (diferencias de hasta 30 en los bordes del vidrio, `results/crops/zoom-*.png`) |
| Motores sin XR32 (16 bits y XR32 apagado) | los nodos IMG no cambian (0 nodos no-XR32 distintos), así que se dibujan igual |
| **DAY/NIGHT** | luces translúcidas corregidas; color de empresa translúcido de noche, sección 9 |
| **Zoom** | idéntico para todo el material sin claves translúcidas; el cambio solo actúa sobre cajas con claves translúcidas |
| **Regiones sucias** | ese código no se ha tocado (diff de 2 ficheros); las capturas consecutivas solo difieren en las superposiciones de interfaz (unos 15.800 píxeles, iguales en los ocho árboles) |
| **Costas A4** | ese código no se ha tocado (`ground_desc.cc`, hook de costa); el material de costa no tiene claves translúcidas, así que el zoom es idéntico por la prueba anterior. **No se repitió con el arnés de A4** |
| **Memoria y estabilidad** | 14/14 ejecuciones con salida 0. Hay un búfer temporal nuevo (`uint32` por píxel de destino de una imagen escalada), liberado en el único camino de salida (sin retornos anticipados entre la reserva y la liberación). **No se pasó ASan** |
| **Ruido medido** | 0 píxeles a 1:1. 22–333 píxeles en fotogramas tardíos de zoom, que **también aparecen entre binarios de 16 bits idénticos**: es algo que se mueve, no la corrección |

## 9. Hallazgo colateral: el motor tradicional oscurece dos veces el color de empresa translúcido de noche

`simgraph16.cc`:
- `recode_img_src_target` toma el color de empresa de `rgbmap_day_night`, ya oscurecido;
- lo guarda como 3-4-3;
- al dibujar, el 3-4-3 pasa otra vez por `rgbmap_day_night`, cuya parte translúcida también está oscurecida (`calc_base_pal_from_night_shift`, "again the same but for transparent colors").

Medido en el fixture con alfa 247:

| Motor | Jugador 0 | Mismo color opaco |
|---|---|---|
| 16 bits | (8, 8, 16) | (32, 44, 90) |
| Candidato XR32 | (36, 47, 87) | (36, 47, 89) |

El candidato oscurece **una** vez, igual que los píxeles opacos. **No** se ha imitado el defecto en XR32 ni se ha tocado el motor tradicional. **NEEDS_MAINTAINER_DECISION:** decidir si es un defecto de 16 bits que merece su propia misión. Es lo que sugiere el código.

## 10. Limitaciones

1. La noche difiere del tradicional en los colores de empresa translúcidos (sección 9).
2. La nueva ponderación del zoom no tiene oráculo numérico de caja: hay identidad demostrada para el material existente y evidencia visual para el nuevo.
3. Costas A4, regiones sucias y ASan no se repitieron con sus arneses.
4. Solo Windows SDL3, con una pantalla; ni SDL2 ni Linux compilados.
5. Colores de empresa: los guardados en la partida (esquemas 0/1 y 3/4), porque un script no puede cambiarlos.
6. La ruta v2 (máscaras) no se toca: su zoom sigue normalizando por muestras, aunque v2 admite contribuyentes sobre base translúcida. Queda fuera del alcance.

## 11. Candidato congelado

| | |
|---|---|
| Commit | `03de7edd6d0c0588c5b0953c286552c14ca47cad` en `src/` del carril (rama `tcc/fix`; **no** empujado) |
| Diff | `patch/XR32-TRANSLUCENT-COMPANY-COLOUR-01.diff` y `patch/0001-commit-03de7ed.patch` |
| Ejecutables | `candidate/` (hashes en `SHA256SUMS`) |
| Paks | `paks-local/` (base y candidato). Derivados de Pak192.Comic: CC BY-NC-SA 3.0, **solo uso local** |
| Fixture | `fixture/` (arte propio del laboratorio) |
| Evidencia | `screenshots/`, `results/`, `logs/`; herramientas en `tools/` y `scene/` |
