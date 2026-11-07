# Construction World — Simulador de construcción 3D (UE 5.8)

Estado actual: **H0 (Fundaciones) completado** y **H1-A** en curso (esqueleto C++, personaje, cámaras, interacción y entrar/salir de un vehículo).
El objetivo inmediato es cerrar H1-A (pruebas en PIE y malla real del personaje) antes de pasar a la maquinaria de H1-B.

> Estado de la build: el módulo C++ **compila sin errores** con UE 5.8 (`UnrealEditor-ConstructionWorld.dll`), el nivel `/Game/Maps/TestLevel` está generado y verificado con 10 actores, y el proyecto tiene CI, métricas de rendimiento y convenciones de unidades. Falta empaquetar una build base y probar el ciclo completo en PIE.

## Panel de seguimiento

Hay un panel web local con la hoja de ruta y un kanban (TO DO / DOING / DONE / BUGS):

```powershell
node web/server.js
```

Luego abre **http://127.0.0.1:8100**. También puedes hacer doble clic en `web\Iniciar panel.cmd`.
Los datos del panel viven en `web/assets/data.js` y se actualizan a medida que avanza el proyecto.

---

## 1. Requisitos

| Requisito | Estado |
|---|---|
| Windows 10/11 64 bits | OK |
| GPU compatible con DX12 (RTX 3060) | OK |
| Git + Git LFS | OK |
| Visual Studio 2022 + workload "Desarrollo de juegos con C++" | OK (MSVC 14.44 + SDK 10.0.22621) |
| Unreal Engine 5.8 (Epic Games Launcher) | OK · `C:\Program Files\Epic Games\UE_5.8` (registrado en el launcher) |
| Blender | OK · 5.2.3 LTS (instalado con winget) |
| Node.js (panel y CI) | OK · Node 20+ |
| RAM | 16 GB (suficiente para empezar; 32 GB recomendable para mundo abierto) |
| Disco | C: ~90 GB libres (UE ocupa ~50-60 GB) · J: 714 GB libres → si C: queda muy justo, mueve UE a J: |

---

## 2. Estructura del proyecto

```
ConstructionWorld.uproject          Proyecto UE (asociado a 5.8)
.vsconfig                           Componentes de Visual Studio necesarios
.gitignore / .gitattributes         Control de versiones + Git LFS
Config/
  DefaultEngine.ini                 Lumen, Nanite, Virtual Shadow Maps, DX12, mapa y GameMode
  DefaultGame.ini                   Nombre/versión del proyecto
  DefaultInput.ini                  Enhanced Input como sistema de entrada
Source/
  ConstructionWorld.Target.cs       Target de juego
  ConstructionWorldEditor.Target.cs Target de editor
  ConstructionWorld/
    ConstructionWorld.Build.cs      Dependencias del módulo
    Public/
      ConstructionWorld.h           Cabecera del módulo
      CWGameMode.h                  GameMode base
      CWPlayerCharacter.h           Personaje (Fase 1-2)
      CWInteractable.h              Interfaz de interacción
      CWInteractionComponent.h      Detección por trazado + prompt
      CWVehicleBase.h               Base de vehículo/maquinaria (Fase 2-3)
      CWExcavator.h                 Excavadora articulada (H1-B, prototipo)
      CWHUD.h                       HUD provisional (punto de mira + prompt + métricas)
      CWLog.h                       Categorías de log del proyecto
      CWUnits.h                     Constantes y conversión de unidades
      CWPerformanceSubsystem.h      Métricas de FPS y tiempo de frame
    Private/
      ConstructionWorld.cpp         Implementación del módulo
      CWGameMode.cpp
      CWPlayerCharacter.cpp
      CWInteractionComponent.cpp
      CWVehicleBase.cpp
      CWExcavator.cpp
      CWHUD.cpp
      CWLog.cpp
      CWPerformanceSubsystem.cpp
Content/Maps/                       Aquí se genera TestLevel.umap
Content/ConstructionWorld/          Assets organizados (vehículos, materiales, audio…)
ArtSource/                          Fuentes editables de arte (.blend, .psd)
Scripts/Python/
  cw_build_test_level.py            Genera el nivel de pruebas automáticamente
Scripts/
  Build.ps1                         Compila con UnrealBuildTool
  ci/validate.mjs                   Valida panel, assets y estructura
  ci/validate-project.mjs           Valida .uproject, targets y módulo
.github/workflows/ci.yml            CI de GitHub Actions
docs/                               Documentación ampliada
  CONVENCIONES.md                   Unidades, coordenadas, nombres y logging
  ASSETS.md                         Inventario de assets y licencias
  assets_manifest.json              Inventario legible por máquina
CONTRIBUTING.md                     Ramas, commits y validación previa al push
```

Contenido implementado en código (compilado y con nivel de pruebas generado el 06/11/2026):

- Módulo C++ `ConstructionWorld` con dependencias Core/CoreUObject/Engine/InputCore/EnhancedInput.
- `ACWGameMode`: pawn por defecto = personaje propio.
- `ACWPlayerCharacter`: cápsula + cámara en tercera persona (spring arm con lag) y primera persona (tecla V), movimiento relativo a cámara, salto, mirada con ratón e interacción (E).
- `UCWInteractionComponent`: trazado esférico desde la vista del jugador que detecta actores `ICWInteractable`.
- `ACWVehicleBase`: vehículo de prueba con caja de colisión, malla placeholder, cámara, entrada/salida (E/F), ocultado y re-posesión del personaje. Todavía **no conduce** (Fase 3).
- `ACWHUD`: punto de mira, prompt de interacción y overlay de rendimiento (FPS, ms, p95) dibujado con Canvas (provisional; se sustituirá por UMG).
- `CWLog`: seis categorías de log (`LogCW`, `LogCWInput`, `LogCWVehicle`, `LogCWInteraction`, `LogCWPerf`, `LogCWAssets`); `LogTemp` está prohibido y la CI lo comprueba.
- `UCWPerformanceSubsystem`: ventana móvil de 240 frames con FPS medio, frame medio, p95 y peor frame; resumen periódico en `LogCWPerf`.
- `CWUnits`: conversiones centralizadas (cm/UU, km/h/cm/s, kg/toneladas, m³/litros, grados/radianes). Convenciones completas en `docs/CONVENCIONES.md`.
- `ACWExcavator`: prototipo articulado de excavadora (torreta, pluma, brazo, cuchara y tres cámaras) pendiente de asset real.
- Enhanced Input **creado por código** (sin assets binarios): el proyecto funciona nada más compilar, sin depender de archivos `.uasset` que aún no existen.
- Placeholder visual (cubo de Engine) escalado a la cápsula; se sustituirá por un personaje real.
- Configuración de render: Lumen, Nanite, Virtual Shadow Maps, DX12, sin lightmaps estáticos.
- Script Python que genera un nivel de pruebas con suelo, bloques, vehículo, sol, cielo y niebla.

No implementado todavía (no lo vamos a fingir): física de vehículos avanzada, excavadora con asset real, terreno deformable, carga de materiales, contratos, economía, IA, mundo abierto, menús y guardado.

---

## 3. Lo que tienes que hacer ahora

### Paso 1 — Unreal Engine

**Unreal Engine 5.8** está instalado en `C:\Program Files\Epic Games\UE_5.8`, registrado en `LauncherInstalled.dat` y es plenamente funcional (compila C++ y ejecuta el editor). Si el launcher no lo muestra en la biblioteca, abre **Epic Games Launcher → Biblioteca → Motor → +** sobre la misma ruta; el manifiesto ya está completo.

El proyecto ya está asociado a `"EngineAssociation": "5.8"`.

### Paso 2 — Abrir y compilar el proyecto

1. Doble clic en `ConstructionWorld.uproject`.
2. Si pregunta con qué versión abrir, elige la 5.8 instalada.
3. Si avisa de que faltan módulos y ofrece recompilar, pulsa **Sí**. La primera compilación tarda varios minutos.
4. Alternativa si no genera el proyecto: clic derecho en `ConstructionWorld.uproject` → **Generate Visual Studio project files**, abre `ConstructionWorld.sln`, configura `Development Editor | Win64` y compila.

Compilación por línea de comandos (sustituye la ruta si instalaste en otro sitio):

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" ConstructionWorldEditor Win64 Development -Project="C:\Users\Jhony\Desktop\Juego\ConstructionWorld.uproject" -WaitMutex
```

O con el script del proyecto:

```powershell
.\Scripts\Build.ps1
```

### Paso 3 — Nivel de pruebas

El nivel `/Game/Maps/TestLevel` **ya está generado y verificado** (10 actores `CW_`). Solo hay que regenerarlo si se borra o se quiere partir de cero. Con el editor abierto:

1. Menú **Tools → Execute Python Script…**
2. Selecciona `Scripts\Python\cw_build_test_level.py`.
3. El script crea el mapa, lo guarda y lo deja abierto.

Alternativa por línea de comandos:

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\Jhony\Desktop\Juego\ConstructionWorld.uproject" -run=pythonscript -script="C:\Users\Jhony\Desktop\Juego\Scripts\Python\cw_build_test_level.py"
```

Si prefieres hacerlo a mano: crea un nivel en `Content/Maps/TestLevel`, añade un `PlayerStart`, un suelo (`/Engine/BasicShapes/Plane` escalado), `DirectionalLight`, `SkyAtmosphere`, `SkyLight` y `ExponentialHeightFog`, y guarda.

### Paso 4 — Probar

Pulsa **Play** (Alt+P). Debes ver el escenario, el cubo-placeholder del personaje y el vehículo de prueba (otro cubo, junto a los bloques). Acércate al vehículo mirándolo para ver el prompt.

**Controles provisionales**

| Acción | Tecla |
|---|---|
| Moverse | W A S D |
| Mirar | Ratón |
| Saltar | Espacio |
| Interactuar / entrar al vehículo | E |
| Cambiar 1ª / 3ª persona | V |
| Salir del vehículo | F |
| Salir del juego (PIE) | Esc |

**Métricas de rendimiento (F0.06)**

El HUD muestra `FPS | ms | p95 ms` en la esquina superior izquierda. Comandos de consola (`~`):

| Comando | Efecto |
|---|---|
| `cw.Perf.Hud 0` / `1` | Oculta / muestra el overlay de métricas |
| `cw.Perf.ReportNow` | Escribe un resumen inmediato en `LogCWPerf` |
| `cw.Perf.Report 0` / `1` | Desactiva / activa el resumen automático |
| `cw.Perf.Interval 30` | Cambia los segundos entre resúmenes |
| `cw.Perf.Reset` | Vacía la ventana de muestras |

Las unidades y convenciones de código están en `docs/CONVENCIONES.md`.

---

## 4. Problemas frecuentes

- **UAC al instalar Visual Studio**: acepta el aviso; sin permisos de administrador no se puede instalar.
- **"The following modules are missing or built with a different engine version"**: pulsa Sí para recompilar; si falla, comprueba que Visual Studio tiene el workload "Desarrollo de juegos con C++" y el Windows 11 SDK.
- **El editor se abre sin mapa**: ejecuta el script del Paso 3 o crea el mapa manualmente.
- **No se mueve el personaje**: verifica que `DefaultInput.ini` carga `EnhancedPlayerInput`/`EnhancedInputComponent` y que el GameMode global es `/Script/ConstructionWorld.CWGameMode`.
- **Git LFS**: los `.uasset`/`.umap` se guardan con LFS; no borres `.gitattributes`.

---

## 5. Siguiente hito

**H0 cerrado.** El trabajo continúa en **H1-A**:

1. Probar en PIE el personaje (1ª/3ª persona, interacción) y el vehículo (entrar/salir) con el ciclo completo.
2. Cerrar H1-A: malla real del personaje, animaciones y mapa de controles configurable.
3. H1-B: excavadora con asset real, motor, física de orugas y camión volquete funcional.
4. H2: prototipo de terreno deformable (la decisión técnica más importante del proyecto).

Objetivo del MVP: aceptar un contrato, excavar 40 m³, cargar el camión, transportar, descargar y cobrar, con guardado de partida.
