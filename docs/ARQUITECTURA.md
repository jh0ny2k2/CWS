# Arquitectura técnica — Construction World

Este documento fija las decisiones de ingeniería antes de escribir los sistemas grandes. Se irá actualizando; cualquier desviación debe justificarse aquí.

## 1. Principios

1. **Jugable antes que espectacular.** Cada sistema debe poder probarse de forma aislada. Nada de maquetas visuales que aparenten funciones inexistentes.
2. **C++ para la lógica, Blueprints para el contenido.** Sistemas críticos (vehículos, terreno, contratos, guardado) en C++ con API expuesta a Blueprints para tuning y prototipado.
3. **Datos, no código.** Máquinas, materiales, contratos y edificios se definen en Data Assets / Data Tables para añadir contenido sin recompilar.
4. **Escalable por módulos.** Empezamos con un módulo `ConstructionWorld`; se dividirá en módulos de runtime cuando el código lo justifique.
5. **Server-authoritative desde el diseño.** Aunque el cooperativo sea posterior, el estado de juego debe vivir en el servidor y replicarse; los clientes no deforman terreno localmente.

## 2. Stack

| Área | Tecnología |
|---|---|
| Motor | Unreal Engine 5.8 |
| Lenguaje | C++ (core) + Blueprints (contenido/tuning) + Python (editor) |
| Render | Nanite, Lumen, Virtual Shadow Maps, TSR/DLSS/FSR, DX12 |
| Física | Chaos (vehículos, cargas, destrucción) |
| Partículas | Niagara |
| Audio | MetaSounds + audio espacial |
| Mundo | World Partition + streaming por capas |
| Entrada | Enhanced Input (acciones creadas por código en Fase 1; assets después) |
| Datos | Data Assets / Data Tables / Primary Data Assets |
| Guardado | `USaveGame` + serialización compacta del terreno |

## 3. Módulos previstos

Ahora solo existe `ConstructionWorld` (Runtime). La separación objetivo:

```
ConstructionWorld (Runtime, base)
  ├─ CWCore          Utilidades, interfaces, tipos de datos, save system
  ├─ CWVehicles      Base de vehículo, hidráulica aproximada, conducción
  ├─ CWTerrain       Heightfield deformable, materiales, persistencia
  ├─ CWConstruction  Fases de obra, piezas, ensamblaje progresivo
  ├─ CWEconomy       Contratos, empresa, precios, nóminas
  ├─ CWAICrew        Trabajadores IA, órdenes, rutas
  └─ CWUI            HUD, menús, mapas
ConstructionWorldEditor
```

Regla: un módulo nuevo solo se crea cuando sus tipos son reutilizados por al menos dos módulos. Evitamos sobreingeniería temprana.

## 4. Decisión crítica: terreno deformable

Es el mayor riesgo técnico del proyecto (hoja de ruta, prioridad "Muy alta"). **No se invertirá en maquinaria detallada ni en la ciudad hasta validar un prototipo de excavación.**

Enfoque recomendado (a validar en Fase 4):

- **Heightfield por chunks** (malla/rejilla deformable) con resolución variable: barato de simular, estable, fácil de guardar (solo deltas de altura) y compatible con colisión.
- Material por capas con **Runtime Virtual Texture** + blending por altura/pendiente/humedad (tierra, grava, arena, asfalto).
- Excavación por *brush* volumétrico con tests de rendimiento por frame y presupuesto fijo.
- Almacenamiento: `TArray<uint16>` de alturas por chunk + material ID + humedad, comprimido. Nada de guardar mallas completas.
- Persistencia y replicación: el servidor aplica los deltas y los replica; el cliente reconstruye visualmente.
- Alternativas a evaluar antes de escribir el sistema: plugin de terreno voxel/heightfield comercial, `Landmass`, o heightfield propio. Criterio: rendimiento estable en 150×150 m con 2 vehículos y físicas activas.

Criterios de aceptación del prototipo: 60 FPS en RTX 3060 con ajustes medios, deformación persistente tras guardar/cargar, sin artefactos de colisión y coste de CPU acotado.

## 5. Vehículos y maquinaria

- `ACWVehicleBase` (Pawn) + componentes: motor, transmisión, hidráulica, orugas/ruedas, combustible, desgaste.
- Excavadora: jerarquía de huesos del brazo controlada por objetivos (IK/servos hidráulicos aproximados) y cuchara con volumen de carga.
- Camión: `ChaosVehicleMovementComponent` o simulación propia por ruedas; caja basculante con masa variable según carga.
- Cada máquina declara capacidades en un Data Asset (`Cuchara 1.2 m³`, `Carga máx. 8 t`), usadas por contratos e IA.
- Cámara de cabina, instrumentación y entrada/salida mediante un único componente `CWInteractionComponent` en el personaje.

## 6. Contratos, economía y guardado

- `UCWContractData` (Data Asset): tipo de obra, volumen objetivo, presupuesto, plazo, penalizaciones, requisitos de maquinaria.
- Estado de contrato como máquina de estados replicada; el progreso se calcula con eventos reales (m³ excavados/entregados), nunca con barras simuladas.
- Dinero, flota, almacén y contratos en un `UCWCompanyState` (UObject de subsistema, `UGameInstanceSubsystem`).
- Guardado: `UCWSaveGame` con versión de esquema + migraciones; terreno serializado por chunks; validación de integridad.

## 7. Mundo abierto

- World Partition con celdas de 128 m (ajustable) y capas de datos (terreno, props, tráfico, audio).
- Tráfico y peatones con Mass Entity o sistemas propios ligeros, nunca Actor por peatón a gran escala.
- Servicios del mundo (gasolinera, taller, concesionario, almacén) como actores con interfaces (`ICWService`).
- El mundo se genera por datos (Data Layers) para poder recortar el mapa de pruebas de la Fase 5.

## 8. Red / cooperativo (diseño, no implementación)

- Sin lógica de juego en el cliente: movimiento validado, excavación autorizada por servidor.
- Replicación de estado compacto (alturas por delta, carga de cuchara como struct, estado de contrato).
- Evitar `RPC` por frame; usar replicación de propiedades y eventos agregados.
- UI y audio en cliente; simulación pesada en servidor o en el host.

## 9. Calidad y verificación

- Compilación limpia en `Development Editor` y `Development` Win64 antes de cada hito.
- Pruebas manuales guionizadas por fase (checklist del MVP en `Hojaruta.md`).
- Automatización progresiva: pruebas de unidad para economía y contratos, pruebas de mapa para terreno.
- Perfilado con `stat unit`, `stat game`, `stat physics` y Unreal Insights; presupuesto de frame documentado.
- Nada se marca como terminado sin una build ejecutable probada.

## 10. Correspondencia con la hoja de ruta

La hoja de ruta oficial es el Plan Maestro (`docs/Plan_Maestro_v1.0.pdf`). Sus hitos se numeran H0–H8 y el panel web (`web/`) es el seguimiento vivo. La tabla siguiente sustituye la numeración antigua por fases para evitar confusiones.

| Hito | Entregable | Estado |
|---|---|---|
| H0. Fundaciones | Proyecto compila, escenario 3D, entrada, repositorio y CI base | En curso (compila sin errores y TestLevel verificado; faltan CI y build empaquetada) |
| H1-A. Personaje y controles | 1ª/3ª persona, interacción, entrar/salir de cabina, HUD | En curso (implementado y compilado; falta probar en PIE) |
| H1-B. Maquinaria | Excavadora y camión funcionales | Pendiente |
| H2. Terreno | Prototipo de excavación validado (riesgo máximo) | Pendiente (riesgo principal) |
| H3. Contratos y vertical slice | Ciclo contractual completo + guardado (CWS-001) | Pendiente |
| H4. Pulido gráfico y sonoro | Assets y pulido visual | Pendiente |
| H5. Mundo abierto | Ciudad, carretera, streaming | Pendiente |
| H6. Construcción y empresa | Fases de obra, economía | Pendiente |
| H7. IA, clima y escalado | Trabajadores, planificación, clima | Pendiente |
| H8. Beta, QA y lanzamiento | QA, builds, rendimiento | Pendiente |

El estado detallado tarea a tarea vive en `web/assets/data.js` (fuente única de verdad del panel). No se marca nada como hecho sin build ejecutable probada.

## 11. Riesgos conocidos

1. **Terreno deformable**: complejidad muy alta; mitigado con prototipo temprano y presupuesto de rendimiento.
2. **Alcance AAA con equipo pequeño**: imposible a corto plazo; se prioriza el vertical slice y se compra/crea arte por fases.
3. **Hardware**: 16 GB de RAM es el mínimo práctico para mundo abierto; ampliar a 32 GB antes de la Fase 7.
4. **Assets**: la calidad fotorealista exige modelos/texturas profesionales (Fab/Marketplace o artistas); no se puede generar arte AAA desde código.
5. **Multijugador**: diseñar ahora, implementar después; no bloquear el single-player por ello.
