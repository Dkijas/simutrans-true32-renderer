# Problemas conocidos y limitaciones — candidato XR32 testing-5 (Windows)

Candidato: `t5/candidate` `038b538` = testing-4 (`92ef4551` / `0afdfbd`) + TCC (`03de7ed`) + A11 (`c66eaaa`). SVN r12377.

## Resuelto respecto a testing-4

| | | Evidencia |
|---|---|---|
| **A11** | Píxeles sueltos de arena en el agua junto a costas inclinadas a 2/1: 7 en testing-4, **0** en el candidato (SDL2 y SDL3, también tras cambios de zoom encadenados). | `A11-ROOT-CAUSE.md` |
| **Colores de empresa translúcidos** (TCC) | Vuelven a recolorearse con la empresa, de día y de noche, con alfa de 8 bits. Las luces translúcidas vuelven a encenderse de noche. | `TCC-INTEGRATION.md`, `WINDOWS-TEST-MATRIX.md` |

## Sigue abierto (limitaciones aceptadas en el encargo, sin empeorar)

| | |
|---|---|
| **Residuo cromático del zoom (TCC)** | Cuando una clave de color de empresa se mezcla con píxeles ordinarios **translúcidos** en la misma muestra, el resultado se aparta de la media premultiplicada ideal: ≈2 de media y 3,8 como máximo con clave translúcida; hasta ≈6 con clave opaca, igual que en testing-4. Medido otra vez sobre el candidato: las mismas cifras que en la consolidación, sin empeorar. |
| **A12** | Las demás imágenes de terreno que genera el juego (transiciones de clima, nieve en pendientes) y el contorno de playa siguen en 16 bits. Fuera del alcance. |
| **H1 fuera de las costas** | La regla de 2/1 de XR32 (replicar píxeles) sigue aplicándose a las imágenes XR32 normales: mar abierto, costas planas, sprites. Allí no produce huecos porque no se recortan contra un contorno de 16 bits escalado aparte. Solo las costas inclinadas toman la regla tradicional. |
| **Doble oscurecimiento del motor de 16 bits** | Sin cambios, por decisión del mantenedor. XR32 oscurece una sola vez. |

## Hallazgos de esta certificación

1. **Windows Defender marca el ejecutable SDL3 publicado de testing-4.**
   - **Qué:** `simutrans-rgba32.exe` del zip `…-testing-4-win64-sdl3.zip` (`43420d63…`) se detecta como `Trojan:Win32/Bearfoos.A!ml` (heurística de aprendizaje automático, firmas 1.463.99.0).
   - **Efecto:** la protección en tiempo real puso en cuarentena 3 copias en este carril mientras se usaban; un escaneo bajo demanda lo reproduce.
   - **Probablemente un falso positivo:** el binario se reproduce byte a byte desde el código y el veredicto es `!ml`.
   - **Los binarios del candidato salen limpios:** testing-5 SDL2 y SDL3, de 32 y de 16 bits, y makeobj (`evidence/defender-scan.txt`). Un veredicto ML puede cambiar con otras firmas.
   - **No se ha cambiado nada en Defender** (ni exclusiones ni restauraciones), ni se ha informado a Microsoft: eso es decisión del mantenedor.
   - **Consecuencia para las pruebas:** en SDL3, los controles con testing-4 usan las capturas de la consolidación, hechas con ese mismo binario, los mismos paks y la misma escena. En SDL2 se capturaron aquí.
2. **Los escenarios de laboratorio necesitan `-log -debug 2`.** Sin esas opciones, el escenario cierra el juego justo después de «script engine started.» (3 de 3). testing-4 publicado hace lo mismo (2 de 2), así que no es una regresión del candidato. No afecta al juego normal: es la combinación de `-scenario` con esta partida guardada y estos ajustes (`logs/scenario-launch-probe.txt`).
3. **Diferencias que no son deterministas** (presentes en testing-4 contra sí mismo, no causadas por el candidato):
   - a 1/2 hay 182–228 px de contenido que cambia entre corridas;
   - el reloj y las coordenadas de la barra de estado;
   - el cursor de herramienta, que SDL2 dibuja donde está el ratón real.

## Limitaciones de la certificación (lo que no se ha probado)

| | |
|---|---|
| Solo Windows x64 | No se ha probado Linux, WSLg, macOS, Android ni ARM; el encargo lo prohíbe expresamente. Las pruebas de la consolidación en otras plataformas no se extienden a este candidato. |
| ASan solo con SDL2 | AddressSanitizer de MSVC (`cl /fsanitize=address`) con el backend SDL2, el único del árbol de dependencias MSVC disponible (sin SDL3). Los ejecutables de producto son de MinGW-w64, que no tiene ASan en Windows, y no hay clang64. Sin UBSan, LSan ni TSan en Windows. |
| Una pantalla | 3440×1440; sin pruebas multimonitor. |
| 16 bits sin volcado por gdb | La puerta gdb lee el estado de `simgraph32`. Las referencias de 16 bits son capturas del motor de la consolidación y de TCC-01. El binario de 16 bits del candidato es **byte-idéntico** al de la base. |
| Zoom de SDL2 en la escena Pak192 | Ahora sí está cubierto: la puerta gdb cambia el zoom con la función del propio juego, sin teclas ni foco. Es una mejora sobre la consolidación. |
