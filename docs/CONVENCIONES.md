# Convenciones de unidades y código — Construction World

Documento de referencia de **F0.06**. Cualquier sistema nuevo debe respetar estas reglas; si necesita una excepción, se anota aquí y en el ADR correspondiente.

## 1. Unidades de medida

| Magnitud | Unidad de diseño / gameplay | Unidad interna del motor | Constante / helper |
|---|---|---|---|
| Longitud, posición, tamaño | metro (m) en documentación; cm en código | centímetro (1 UU = 1 cm) | `CWUnits::CmPerMeter`, `MetersToUU`, `UUToMeters` |
| Masa | kilogramo (kg) | kilogramo | `CWUnits::KgPerTonne`, `TonnesToKg` |
| Volumen de material | metro cúbico (m³) | cm³ si se calcula con geometría | `CubicMetersToLiters`, `CubicCmToCubicMeters` |
| Velocidad | km/h en UI y diseño | cm/s en el motor | `CWUnits::KmhToCms`, `CmsToKmh`, `FormatSpeedKmh` |
| Ángulo | grados (°) | grados en API; radianes solo en trigonometría | `CWUnits::DegreesToRadians`, `RadiansToDegrees` |
| Tiempo | segundo (s) | segundo | — |
| Tiempo de frame | milisegundo (ms) | ms | `UCWPerformanceSubsystem` |

Reglas rápidas:

- **Nunca** mezclar metros y Unreal Units en la misma variable. Si el nombre no lleva sufijo, está en UU (cm).
- Los campos editables exponen unidades en el nombre o en `meta` (`MaxDriveSpeed` está en cm/s, `TurretSpeed` en °/s).
- La economía y los contratos hablan en **m³ y toneladas**; la física habla en **cm³ y kg**.
- La UI muestra **km/h, m³, litros y toneladas**, no unidades del motor.

## 2. Sistema de coordenadas

- Mundo **Z-up**, mano izquierda (convención de Unreal).
- **X** = delante, **Y** = derecha, **Z** = arriba.
- Los actores miran hacia su `ForwardVector` (+X local).
- Grados positivos de *yaw* giran a la derecha; *pitch* positivo mira hacia arriba.
- Los assets de Blender se exportan con **-Y forward, Z up** y escala 1 unidad = 1 cm (o se aplica escala 100 al importar).

## 3. Nombres y carpetas

| Tipo | Prefijo | Ejemplo |
|---|---|---|
| Clase C++ | `CW` | `ACWExcavator`, `UCWPerformanceSubsystem` |
| Interfaz | `ICW` | `ICWInteractable` |
| Enumeración | `ECW` | `ECWExcavatorControlMode` |
| Estructura | `FCW` | `FCWContractState` |
| Blueprint | `BP_CW_` | `BP_CW_Excavator` |
| Material | `M_CW_` | `M_CW_Dirt` |
| Malla | `SM_CW_` | `SM_CW_ExcavatorBody` |
| Textura | `T_CW_` | `T_CW_Dirt_Albedo` |
| Nivel | `L_CW_` | `L_CW_TestLevel` |
| Data Asset | `DA_CW_` | `DA_CW_ExcavatorSpec` |

Carpetas de contenido: `Content/ConstructionWorld/{Vehicles,Characters,Materials,Environment,Audio,Data,UI}`.

## 4. Logging

Categorías declaradas en `Source/ConstructionWorld/Public/CWLog.h`:

| Categoría | Uso |
|---|---|
| `LogCW` | Mensajes generales del proyecto |
| `LogCWInput` | Enhanced Input, mapeos y posesión |
| `LogCWVehicle` | Vehículos, conducción, hidráulica |
| `LogCWInteraction` | Interacción y prompts |
| `LogCWPerf` | Métricas de FPS y tiempo de frame |
| `LogCWAssets` | Carga/validación de assets |

- **Prohibido `LogTemp`** en `Source/` (lo comprueba la CI).
- Nivel por defecto `Log`; usar `Verbose`/`VeryVerbose` para diagnóstico puntual.
- Filtrar en consola: `Log LogCWVehicle Verbose`.

## 5. Rendimiento

- Métricas en vivo: overlay del HUD (`cw.Perf.Hud 1`) y comando `cw.Perf.ReportNow`.
- Ventana móvil de 240 frames (~4 s a 60 FPS): FPS medio, frame medio, p95 y peor frame.
- Resumen automático cada 30 s en `LogCWPerf` (`cw.Perf.Report 1`, `cw.Perf.Interval 30`).
- Presupuesto objetivo del prototipo: **60 FPS en RTX 3060 a 1080p con ajustes medios**.
- Un frame de 16,67 ms es el objetivo; p95 por encima de 20 ms indica un problema a investigar.

## 6. Unidades en la API expuesta a Blueprints

- Los `UPROPERTY` editables documentan la unidad en el nombre o en el comentario.
- Los getters devuelven siempre la unidad interna (cm/s, kg, cm³) salvo que el nombre indique otra cosa.
- Para texto de UI usar los helpers `FormatSpeedKmh` y `FormatVolumeM3`.
