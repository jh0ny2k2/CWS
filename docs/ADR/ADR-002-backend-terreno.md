# ADR-002 — Backend del terreno deformable

- **Estado:** Propuesto (se valida en H2)
- **Fecha:** 2026-11-06
- **Decisores:** Jhony Chaves

## Contexto

La excavación y el movimiento de tierras son el núcleo del juego y su mayor riesgo técnico. Hay que elegir cómo representar el terreno antes de invertir en arte o en la ciudad. Requisitos: deformación persistente, colisión fiable, volumen conservado entre excavar/transportar/depositar y rendimiento estable en una zona de 150×150 m con dos vehículos y físicas activas.

## Decisión propuesta

Adoptar un **heightfield por chunks** como base:

- Rejilla regular por chunks con resolución variable; altura almacenada como `uint16` por celda.
- Material por capas (tierra, grava, arena, asfalto) con Runtime Virtual Texture y blending por altura/pendiente/humedad.
- Excavación por brush volumétrico con presupuesto de operaciones por frame y actualización de colisión por lotes.
- Persistencia: solo deltas de altura + id de material + humedad por chunk, comprimidos. Nunca se guardan mallas completas.
- Replicación: el servidor aplica deltas; el cliente reconstruye visualmente.

Antes de comprometerse se comparará contra **SDF/voxel con malla dinámica** y contra plugins/`Landmass` en una escena de benchmark sin arte pesado (tareas F3.01–F3.05).

## Criterios de aceptación del prototipo

- 60 FPS en RTX 3060 con ajustes medios.
- Deformación correcta tras guardar y cargar.
- Sin artefactos de colisión ni huecos visibles.
- Coste de CPU acotado y sin picos de frame al actualizar colisión.

## Consecuencias

- Si el heightfield no supera el benchmark, se documentará aquí la migración a SDF/voxel antes de construir sistemas dependientes.
- El guardado y la replicación dependen de esta decisión: se implementan después del prototipo, no antes.
