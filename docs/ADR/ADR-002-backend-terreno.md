# ADR-002 — Backend del terreno deformable

- **Estado:** Aceptado (validado en H2)
- **Fecha:** 2026-11-06 (actualizado 2026-11-07)
- **Decisores:** Jhony Chaves

## Contexto

La excavación y el movimiento de tierras son el núcleo del juego y su mayor riesgo técnico. Hay que elegir cómo representar el terreno antes de invertir en arte o en la ciudad. Requisitos: deformación persistente, colisión fiable, volumen conservado entre excavar/transportar/depositar y rendimiento estable en una zona de 150×150 m con dos vehículos y físicas activas.

## Decisión tomada

Se adopta definitivamente el **heightfield deformable por chunks** (`ACWTerrainField`):

- Rejilla regular por chunks de 8×8 celdas con `ProceduralMeshComponent`.
- Alturas almacenadas como `float` en cm y material por celda (`ECWSoilType`: Tierra, Grava, Arena, Arcilla) con densidades físicas reales.
- Excavación y depósito conservando el volumen matemático exacto.
- Actualización de colisión por lotes (máximo 4 chunks sucios por tick) para eliminar picos de frame.
- Serialización de deltas comprimidos con cabecera mágica (`0x43575445`), versión y checksum CRC32.
- Se descarta SDF/voxel como backend general por consumo de memoria (8–16× mayor) y complejidad excesiva para excavaciones de superficie y zanjas 2.5D. Se reserva la evaluación de SDF exclusivamente para futuros túneles subterráneos si se requiriesen en fases avanzadas.

## Resultados del benchmark (H2)

- **Rendimiento:** 1.000 operaciones de excavación completadas en 0.8 ms (~0.001 ms por operación).
- **Memoria:** ~95 KB para 32×32 celdas (~381 KB para 64×64 celdas / 150 m).
- **Persistencia:** Guardado en 2.1 ms, recarga en 34.2 ms. Detección segura y rechazo de archivos corruptos o truncados sin caídas.
- **Conservación de volumen:** Error menor a 0.001 m³ tras 1.000 operaciones alternadas.
- **Batería de tests funcionales (T-001..T-006):** Todos en estado PASS.

## Consecuencias

- El backend de terreno queda fijado para las fases de jugabilidad (H3: Primera misión) y mundo abierto.
- La excavadora y el camión volquete interactúan con el terreno a través de `UCWTerrainSubsystem`.
