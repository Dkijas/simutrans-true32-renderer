# A11 / H1 — causa raíz y corrección

Misión SIMUTRANS-XR32-TESTING5-WINDOWS-01. Base: testing-4 publicado (`92ef4551` = laboratorio `a4/cache` `0afdfbd`, SVN r12377).

## 1. El defecto

A zoom 2/1, junto a las costas inclinadas, aparecen píxeles sueltos de arena dentro del agua. Ocurre con un pak cuyo mar tiene datos de 32 bits y `SIMUTRANS_XR32=1`.

Medición publicada en `KNOWN-ISSUES-r12377-testing-4.md`, apartado A11: mapa fijo `coastworld.sve` con 93 costas inclinadas, 1920×1080, día forzado y pak mutante con el mar de un solo color. Resultado: **7 huecos de 1 px**; testing-3 no tiene ninguno.

### Reproducción en este carril, antes de tocar nada

- **Herramientas:**
  - `tools/t5_world.sh`: captura del mundo con la puerta gdb de la batería, sin modificar el ejecutable;
  - `tools/a11_oracle.py`: huecos de 1–8 px rodeados de agua, clasificados píxel a píxel contra testing-3 en la misma escena.
- **Clases del oráculo:**
  - `T3HOLE`: testing-3 tiene el mismo hueco;
  - `SAME`: mismo color que testing-3;
  - `BLEND`: mezcla de la máscara de playa;
  - `GAP`: la tierra asoma donde debería haber agua.

| Ejecutable (publicado, hash verificado) | 1:1 | 2/1 |
|---|---|---|
| testing-4 SDL3 `43420d63` | 0 GAP | **7 GAP** (arena `b0a040`, `807030`… donde testing-3 tiene agua) |
| testing-4 SDL2 `42e9089c` | 0 GAP | **7 GAP** (los mismos) |

- **Control:** quitar agua a propósito (1 de cada 97 píxeles de clave) produce 9.936 GAP, así que el detector puede fallar.
- **1:1:** los 2 píxeles candidatos son mezclas de la máscara de playa, no huecos; coincide con lo que A4C clasificó.

Evidencia: `evidence/a11-repro-sdl2.txt`, `evidence/a11-fix-s3.txt` (filas de testing-4), `evidence/a11-world-zoomchain-s3.txt`.

## 2. La causa, regla por píxel

Una costa inclinada se dibuja como `draw_alpha(tesela_de_agua, máscara_de_playa, ALPHA_RED)` (`grund.cc`): el color viene de la tesela de agua y el peso, de la máscara.

Desde A4 la tesela de agua es una imagen **XR32 derivada** del mar. La máscara sigue siendo una imagen de **16 bits**. A 2/1 cada una se amplía con un escalador distinto:

| | Escalador | Regla a 2/1 |
|---|---|---|
| Máscara (16 bits) | `zoom_core_rezoom()`, caso `zoom_den == 1` (`zoom_core.h`) | **No replica píxeles.** Copia la primera y la última fila y la primera y la última columna. En el interior aplica `zoomin_pixel(p, ab, rl, dia)`:<br>- un píxel transparente cuyos tres vecinos hacia el siguiente píxel de origen son opacos **se rellena** con la media de dos de ellos;<br>- uno opaco con los tres transparentes **se elimina**;<br>- y la regla «preserve one pixel» conserva uno si el superpíxel desaparecería. |
| Tesela de agua (XR32) | `stlab_xr32_zoom_build()` | Con `den == 1` cada píxel de destino tiene una sola muestra: **replicación pura**. |

La geometría A1 de XR32 copia la rejilla del escalador tradicional para `den > 1`. Las demás ampliaciones (7/4, 3/2, 11/8, 4/3, 5/4, 9/8) tienen `den > 1` y coinciden. Solo 2/1 tiene el caso especial `den == 1`.

Donde la máscara dice «agua» y la tesela replicada no tiene píxel (una esquina cóncava que el escalador tradicional habría rellenado), se ve la tierra de debajo: ese es el GAP.

**Comprobación de la regla** (`tools/a11_model.py`). Es un modelo independiente, escrito desde la especificación de `zoom_core.h` y no desde el código del candidato. Toma la cobertura de cada tesela a 1:1 y predice su cobertura a 2/1, que se compara con la que dibuja el motor (`coastgate`: 195 teselas de costa × niveles de luz):

| Corrida | Diferencia con el modelo tradicional | Diferencia con replicación pura |
|---|---|---|
| testing-4, XR32 apagado (camino de 16 bits) | **0 px** | 52.026 px |
| testing-4, XR32 encendido | **52.026 px** (la cifra de A4C) | **0 px** |
| control: modelo de la tesela siguiente | 313.050 px (puede fallar) | |

La regla queda establecida al píxel: testing-4 replica donde el camino tradicional redondea.

**No es la caché A4.** La caché por estadio de A4C produce dibujos byte-idénticos a A4 sin caché (11.247 de 11.247). El defecto está en la geometría del escalador, no en la composición ni en la caché.

## 3. La corrección

Commit `c66eaaa` (`a11/fix`), y en el candidato `038b538`. Un solo fichero: `src/simutrans/display/stlab_xr32_zoom.cci`, +132/−1.

- **Función nueva `stlab_xr32_zoom_build_2x_legacy()`.** Construye la caché a 2/1 de una tesela de costa **derivada** con la regla tradicional, en el orden tradicional y sobre sus planos XR32 de día y de noche:
  - un píxel que la regla tradicional copia toma ese píxel de origen, que es exactamente lo que daba la replicación;
  - un píxel que rellena toma la media de los mismos dos vecinos, igual que `SumSubpixel`;
  - un píxel que elimina queda como hueco.
- **`stlab_xr32_zoom_ensure()`** llama a esa función solo si la imagen es una tesela derivada y la escala es 2/1 (`num == 2 && den == 1`). Todo lo demás sigue en `stlab_xr32_zoom_build()`.
- **Lectura fuera del búfer del código tradicional.** La regla «preserve one pixel» del original lee la fila anterior por índice lineal, y su primera lectura cae antes del búfer. Esa lectura nunca decide, porque el píxel de encima es una copia de ese mismo píxel opaco. Está documentado en el código y el modelo la trata igual.

**Lo que no cambia:**
- el formato XR32;
- makeobj (el binario sale idéntico, `8a150381`);
- el motor de 16 bits (binarios idénticos);
- la caché A4;
- cualquier imagen que no sea una tesela derivada;
- cualquier escala distinta de 2/1.

**Coste:** 2 bytes de estado más 8 bytes de índices por píxel de destino, solo durante la construcción y liberados al terminar. No se añade ninguna caché.

## 4. Alternativas descartadas

| | Idea | Por qué no |
|---|---|---|
| B | Aplicar la regla tradicional de 2/1 a **todas** las imágenes XR32 (corregir H1 en general) | Cambiaría resultados XR32 ya certificados a 2/1: barras A1/A2, catenaria y sprites de todos los paks. El brief pide no alterar el escalado general ni las escenas no afectadas. |
| C | Ampliar la máscara de playa con la regla XR32 | La máscara es una imagen del pak que comparten las costas planas; cambiaría bordes de 16 bits que hoy son correctos, y además el cambio caería en la geometría tradicional. |
| D | Rellenar los huecos después de dibujar (vecino más próximo donde la máscara pesa y la tesela no tiene píxel) | Heurístico, difícil de demostrar exacto, y un parche de pintura sobre el resultado: justo lo que el brief prohíbe («sobrepintados»). |
| — | Ocultar las 7 coordenadas | Prohibido por el brief, y no corrige la causa: otro mapa tendría otros huecos. |

Se eligió **A**: el mismo enfoque que proponía el brief H1 preparado tras testing-4 (`<testing-4-lab>/deliver/H1-MISSION-BRIEF.md`). Afecta solo a las teselas de A4, deja H1 como está certificado en lo demás y tiene un oráculo exacto: la cobertura tradicional de la misma tesela.

## 5. Resultado

**Cobertura** (`evidence/a11-coverage-*.txt`, `evidence/a11-identity-*.txt`, `evidence/a11-model-*.txt`):
- a 2/1, la cobertura del candidato coincide con el modelo tradicional en **0 px**, con los paks mutante, `makie` y `daynight` y con niveles de luz 0, 2 y 4;
- las 195 teselas de costa tienen la cobertura de XR32 apagado en **los 8 zooms medidos**: 1:1, 2/1, 7/4, 3/2, 5/4, 9/8, 3/4 y 1/2.

**Identidad frente a testing-4:**
- todos los zooms salvo 2/1 son **byte-idénticos**;
- a 2/1 **no cambia el color de ningún píxel que ambos dibujen**; solo cambian los 52.026 px de cobertura.

**Mundo** (`evidence/a11-fix-s2.txt`, `a11-fix-s3.txt`, `a11-world-zoomchain-s3.txt`): **0 GAP** a 1:1, 9/8, 3/4, 1/2 y 2/1, en SDL2 y SDL3, también tras cambios de zoom encadenados (2/1 → 1:1 → 2/1). Los píxeles de clave aumentan en exactamente 7. testing-4 sigue dando 7 GAP en las mismas capturas.

**Diferencias de imagen restantes a 2/1, frente a testing-4:** 106 px en el mundo (`evidence/a11-world-z2-diff-classes.txt`).
- testing-4 contra sí mismo da 0 px a 2/1 (`a11-world-runtorun-t4.txt`), así que vienen de A11.
- 7 son los huecos, que pasan a ser agua.
- 99 están en costas inclinadas tapadas por bosque: el borde del agua asoma entre la copa de los árboles y ahí cambia su cobertura, ahora la de testing-3.
- Son el efecto esperado de devolver la geometría tradicional, no una regresión.

**Diferencias entre corridas que no son de A11:** a 1/2 hay 212–228 px de un objeto que cambia entre corridas. Aparecen igual entre dos corridas del mismo testing-4.
