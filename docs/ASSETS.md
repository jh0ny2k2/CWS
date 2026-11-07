# Inventario de assets y licencias — Construction World

Documento de referencia de **F0.07**. El inventario legible por máquina está en `docs/assets_manifest.json` y la CI lo valida.

## 1. Reglas

1. **Ningún asset entra al proyecto sin licencia registrada** en el manifiesto.
2. Se permite: contenido de Unreal Engine, licencias Fab/Marketplace compradas, CC0 y CC-BY (con atribución).
3. **No se permite** arte de terceros sin licencia, modelos extraídos de otros juegos ni texturas de procedencia dudosa.
4. Los binarios (`.fbx`, `.blend`, `.uasset`, texturas, audio) van por **Git LFS** (ya configurado en `.gitattributes`).
5. Los archivos fuente editables (`.blend`) se guardan en `ArtSource/`; los assets finales, en `Content/ConstructionWorld/`.
6. Cada asset se anota con: ID, nombre, categoría, estado, fuente, licencia, ruta y notas.
7. Los assets de prototipo (cubos de Engine) se sustituirán por arte real **sin cambiar la lógica C++**.

## 2. Estado del inventario

Resumen actual (detalle en `docs/assets_manifest.json`):

| Categoría | Prototipo | Planificado | Total |
|---|---|---|---|
| Vehículos | 1 (excavadora) | 3 | 4 |
| Personajes | 0 | 1 | 1 |
| Materiales | 0 | 3 | 3 |
| Entorno | 0 | 2 | 2 |
| Audio | 0 | 1 | 1 |
| UI | 0 | 1 | 1 |

## 3. Assets de prototipo (ya en el proyecto)

| ID | Asset | Implementación | Licencia |
|---|---|---|---|
| AST-001 | Excavadora de cadenas | Jerarquía de cubos de `/Engine/BasicShapes/Cube` en `ACWExcavator` | Contenido de Unreal Engine (Epic Games) |
| AST-002 | Camión volquete | `ACWVehicleBase` con cubo placeholder | Contenido de Unreal Engine (Epic Games) |
| AST-003 | Operario | Cubo placeholder de `ACWPlayerCharacter` | Contenido de Unreal Engine (Epic Games) |

Los tres son **provisionales**: sirven para validar física, controles y jerarquías, no para el resultado visual final.

## 4. Assets planificados y fuentes recomendadas

| ID | Asset | Prioridad | Fuente recomendada | Licencia prevista |
|---|---|---|---|---|
| AST-004 | Excavadora AAA con huesos/pivotes | Alta (H1-B/H4) | Fab (Unreal Marketplace) | Fab Standard License |
| AST-005 | Camión volquete AAA | Alta (H1-B/H4) | Fab | Fab Standard License |
| AST-006 | Operario riggeado con animaciones | Alta (H4) | Fab / Mixamo (personaje) | Fab Standard / Mixamo terms |
| AST-007 | Tierra, grava y arena (PBR) | Alta (H2) | Quixel Megascans (Fab) | Fab/Megascans License |
| AST-008 | Asfalto y hormigón (PBR) | Media (H5) | Quixel Megascans / AmbientCG | Fab / CC0 |
| AST-009 | Props de obra (vallas, conos, señales) | Media (H5) | Fab / Kenney | Fab Standard / CC0 |
| AST-010 | Cielo, vegetación y rocas | Media (H5) | Quixel Megascans / Poly Haven | Fab / CC0 |
| AST-011 | Motor, hidráulica, impactos y ambiente | Alta (H1-B/H4) | Freesound / Fab Audio | CC0 / CC-BY / Fab |
| AST-012 | Iconos de UI e indicadores | Baja (H3) | Kenney / creación propia | CC0 |

### Fuentes y condiciones

- **Unreal Engine / Epic Games**: contenido de `/Engine` y plantillas; uso libre dentro de proyectos UE.
- **Fab (antes Unreal Marketplace)**: licencia estándar por asset; se puede usar en el juego, no redistribuir el asset suelto. Guardar el recibo o la referencia de compra.
- **Quixel Megascans**: gratuitos para UE a través de Fab; no redistribuir los assets originales.
- **CC0** (Kenney, Poly Haven, AmbientCG): uso libre, sin atribución obligatoria; aun así se registra la fuente.
- **CC-BY** (parte de Freesound): atribución obligatoria en los créditos del juego.
- **Mixamo**: animaciones y personajes gratuitos con cuenta Adobe; revisar términos antes de publicar.

## 5. Criterios de aceptación de un asset

Antes de marcar un asset como **integrado**:

- Escala correcta (1 UU = 1 cm) y pivotes en el origen lógico.
- Ejes y orientación correctos (X delante, Z arriba).
- Colisión probada (simple o compleja según el uso).
- Materiales con nombres `M_CW_*` y texturas `T_CW_*`.
- Rendimiento: sin superar el presupuesto de Nanite/Lumen del prototipo.
- Licencia registrada y archivo fuente guardado en `ArtSource/` si aplica.

## 6. Mantenimiento

- Al adquirir o crear un asset: actualizar `docs/assets_manifest.json` y este documento.
- Al integrarlo: cambiar `status` a `adquirido` o `integrado` y anotar la ruta final.
- La CI falla si un asset no tiene `license`, `source` o un estado válido.
