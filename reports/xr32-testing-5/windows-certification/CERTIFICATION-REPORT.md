# SIMUTRANS-XR32-TESTING5-WINDOWS-01 — Informe de certificación

Fecha: 2026-11-03. Todo es **local**: no hay push, tag, release, commit SVN ni publicación en el foro o Discord. testing-4, los artefactos congelados de TCC, los laboratorios A4 y TCC, Pak192.Comic y el SVN oficial quedan intactos (sección 6).

| | |
|---|---|
| **Veredicto** | **B — CERTIFIED WITH LIMITATIONS** |
| **Estado** | **READY_FOR_APPROVAL** |
| **Publicación** | **NOT_AUTHORISED** |
| Base | testing-4 publicado `92ef45519f42b3d086d116a9cf3bf348fe271303` = laboratorio `a4/cache` `0afdfbd` (SVN r12377); hashes verificados contra la release |
| Candidato | `t5/candidate` **`038b538`** = base + `07e4d54` (TCC, cherry-pick de `03de7ed`, diff idéntico) + `038b538` (A11, cherry-pick de `c66eaaa`) |
| Plataformas | **Windows x64, SDL2 y SDL3, únicamente** |

## Respuestas

**¿Se corrigió A11/H1? Sí.**
- **Antes:** en el mapa fijo de 93 costas inclinadas a 2/1 y 1920×1080, testing-4 tenía 7 huecos de arena, en SDL2 y en SDL3, reproducidos aquí antes de tocar nada.
- **Ahora:** el candidato tiene **0**, también tras cambios de zoom encadenados.
- **Causa demostrada al píxel con un modelo independiente:** a 2/1 el escalador de 16 bits redondea el contorno y el de XR32 replica píxeles. La máscara de playa es de 16 bits y el agua de costa derivada, XR32.
- **Corrección:** las teselas derivadas toman a 2/1 la regla tradicional. Su cobertura coincide con la de 16 bits (**0 px** de diferencia en 195 teselas, 8 zooms, 3 paks, luz 0/2/4) y no cambia ningún color.
- Detalle: `A11-ROOT-CAUSE.md`.

**¿Se preservó TCC? Sí.**
- El parche integrado es **idéntico** al congelado `03de7ed`.
- **Caso real** `station_track_Bahnsteig_2000_5`: **X = 9,90** en SDL2 y SDL3, igual que el laboratorio original (control testing-4: 0,47).
- **Oráculo de zoom del fixture de alfas:** resultado completo **idéntico** al de la consolidación.
- **Noche:** oscurecimiento único (MAE 0,15–0,17); las luces translúcidas se encienden de noche (+165 al alfa 255); el RGB ordinario no se recolorea; alfa de 8 bits.
- **Seis objetos de Pak192.Comic:** métricas idénticas a TCC-01.
- **Interacción entre los dos cambios:** ninguna, porque afectan a conjuntos de imágenes disjuntos.
- Detalle: `TCC-INTEGRATION.md`.

**¿Son válidos SDL2 y SDL3? Sí.**
- puertas A4 8/8 en cada uno;
- oráculo de presentación: las 44 combinaciones escenario × backend del candidato a 0 px (86 ejecuciones con los mutantes de control), regiones sucias incluidas, y 2/1 con teselas derivadas (`zoomx`);
- batería de regresión igual que testing-4, suites 309/309;
- K/M 48/48;
- los volcados de la escena Pak192 son byte-idénticos entre SDL2 y SDL3, salvo contenido que cambia entre corridas.
- Detalle: `WINDOWS-TEST-MATRIX.md`.

**¿Qué resultados gráficos cambiaron?**
1. **Costas inclinadas a 2/1:** el contorno del agua es ahora el de testing-3 (52.026 px de cobertura en las 195 teselas). Desaparecen los 7 huecos, y 99 píxeles de borde bajo la copa de los árboles cambian; los colores no cambian.
2. **Colores de empresa translúcidos y luces translúcidas** (TCC): recoloreados por empresa y encendidos de noche.

Ninguna otra imagen, zoom o escena cambia: las imágenes del pak, los demás zooms, XR32 apagado, el motor de 16 bits y los paks sin XR32 son byte-idénticos a testing-4.

**¿Hay regresiones? No se ha encontrado ninguna.**
- **Rendimiento:** sin diferencia medible en 64 de 66 celdas. Las 2 restantes van en sentidos opuestos, con n = 3 y en un escenario donde ningún cambio ejecuta código distinto: ruido.
- **Memoria:** sin diferencia medible, o 0,2 MB menos.
- **Avisos de compilación:** conjuntos idénticos.
- **ASan (MSVC, SDL2):** 0 informes.
- **Antivirus:** binarios limpios.
- Detalle: `PERFORMANCE.md`.

**¿Está listo para aprobación? Sí: READY_FOR_APPROVAL**, con las limitaciones de la sección 4.

## 1. Fases

| Fase | Resultado |
|---|---|
| A — Reproducción | 7 GAP a 2/1 en SDL2 y SDL3, 0 a 1:1. El oráculo independiente detecta huecos inyectados (9.936). La regla queda demostrada con el modelo (testing-4 con XR32 apagado = modelo, 0 px; con XR32 activo = replicación, 0 px). |
| B — Corrección | `c66eaaa`, un fichero, +132/−1. Alternativas B, C y D y el parche de coordenadas, descartados (`A11-ROOT-CAUSE.md` §4). |
| C — Certificación aislada | 1:1, 2/1, 9/8, 3/4 y 1/2, más 7/4, 3/2 y 5/4 por tesela; costas planas e inclinadas; agua XR32 y tradicional; día y noche; estadios 0/13/31; luz 0/2/4; regiones sucias; zooms encadenados. 0 huecos y ninguna regresión equivalente. |
| D — Integración | `t5/candidate` con dos commits separados y trazables, sin conflictos. Parche TCC sin alterar (control incluido). |
| E — Certificación Windows | véase `WINDOWS-TEST-MATRIX.md` y `PERFORMANCE.md` |

## 2. Integridad

- **Reproducibilidad:** la receta de compilación aplicada a `a4/cache` produce los ejecutables **publicados** de testing-4 byte a byte (`43420d63` SDL3, `42e9089c` SDL2).
- **16 bits:** el binario del candidato es idéntico al de la base (`8feaaf49` / `20cbf7da`).
- **makeobj:** idéntico al certificado por TCC (`8a150381`).
- **Formato XR32:** sin cambios. Los paks antiguos se dibujan igual, y los paks nuevos funcionan en testing-4 (1:1 correcto, zoom con la ponderación antigua).

## 3. Pruebas que no pudieron hacerse como se planteó, y cómo se cubrieron

| | Cobertura |
|---|---|
| Los controles SDL3 con testing-4 de la escena Pak192 | Windows Defender puso en cuarentena en el carril el ejecutable SDL3 publicado de testing-4. Se usan las capturas de la consolidación y de TCC-01 hechas con **ese mismo binario**, los mismos paks y la misma escena. En SDL2 se capturaron aquí y dan los mismos valores. |
| Las referencias de 16 bits de la escena Pak192 | La puerta gdb no puede volcar un ejecutable de 16 bits. Se usan capturas del motor ya existentes; el binario de 16 bits del candidato es byte-idéntico al de la base. |
| Sanitizadores | ASan de MSVC solo con SDL2 (no hay SDL3 en el árbol MSVC disponible). Sin UBSan, LSan ni TSan en Windows, y MinGW no tiene ASan. |

## 4. Limitaciones (por qué B y no A)

1. **Residuo cromático heredado del zoom:** con clave de color de empresa más ordinario translúcido, la diferencia con la media ideal llega a 3,8 (clave translúcida) y a ≈6 (clave opaca). Aceptado en el encargo y **sin empeorar**: el resultado del oráculo es idéntico.
2. **Cobertura de sanitizadores parcial** en Windows (sección 3).
3. **Alcance solo Windows.** El encargo pide no convertir B en A por haber reducido el alcance.
4. **Hallazgo externo al candidato:** Defender marca con su heurística ML (`Trojan:Win32/Bearfoos.A!ml`) el ejecutable SDL3 de **testing-4 ya publicado**. No se ha cambiado ni comunicado nada; es decisión del mantenedor.

## 5. Recomendación

Si el mantenedor lo autoriza:
1. **Publicar `038b538` como testing-5 de Windows** con los artefactos de `candidate/` (renombrados quitando `-candidate-`) y las notas de `RELEASE-NOTES-DRAFT.md`.
2. **Decidir sobre el aviso de Defender** de testing-4 SDL3: informar del falso positivo a Microsoft o retirar o sustituir el binario. Comprobar los binarios de testing-5 con firmas recientes justo antes de publicarlos.
3. Antes de una release multiplataforma, certificar Linux y macOS por separado; este candidato no se ha probado en ellas.

## 6. Lo protegido, comprobado al cierre

Véase `evidence/protected-check.txt`: sumas de la release testing-4, de los artefactos TCC-01 y de la consolidación, estado de los laboratorios A4/TCC, Pak192.Comic y SVN.
