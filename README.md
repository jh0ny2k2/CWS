# Construction World — Simulador de construcción 3D (UE 5.8)

Estado actual: **H0 — Fundaciones** y parte de **H1-A** (esqueleto C++, personaje, cámaras, interacción y entrar/salir de un vehículo).
El objetivo inmediato es tener un proyecto que compile y se abra en Unreal Engine. El resto de sistemas (conducción, terreno deformable, contratos, mundo abierto) se construirá sobre esta base.

> Importante: el código de este repositorio **todavía no se ha compilado** en este equipo. Unreal Engine 5.8 se está instalando y la primera compilación está pendiente.

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
| Visual Studio 2022 + workload "Desarrollo de juegos con C++" | OK (verificado) |
| Unreal Engine 5.8 (Epic Games Launcher) | Instalándose en `C:\Program Files\Epic Games\UE_5.8` |
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
      CWHUD.h                       HUD provisional (punto de mira + prompt)
    Private/
      ConstructionWorld.cpp         Implementación del módulo
      CWGameMode.cpp
      CWPlayerCharacter.cpp
      CWInteractionComponent.cpp
      CWVehicleBase.cpp
      CWHUD.cpp
Content/Maps/                       Aquí se genera TestLevel.umap
Scripts/Python/
  cw_build_test_level.py            Genera el nivel de pruebas automáticamente
docs/                               Documentación ampliada
```

Contenido implementado en código (pendiente de compilar en tu equipo):

- Módulo C++ `ConstructionWorld` con dependencias Core/CoreUObject/Engine/InputCore/EnhancedInput.
- `ACWGameMode`: pawn por defecto = personaje propio.
- `ACWPlayerCharacter`: cápsula + cámara en tercera persona (spring arm con lag) y primera persona (tecla V), movimiento relativo a cámara, salto, mirada con ratón e interacción (E).
- `UCWInteractionComponent`: trazado esférico desde la vista del jugador que detecta actores `ICWInteractable`.
- `ACWVehicleBase`: vehículo de prueba con caja de colisión, malla placeholder, cámara, entrada/salida (E/F), ocultado y re-posesión del personaje. Todavía **no conduce** (Fase 3).
- `ACWHUD`: punto de mira y prompt de interacción dibujado con Canvas (provisional; se sustituirá por UMG).
- Enhanced Input **creado por código** (sin assets binarios): el proyecto funciona nada más compilar, sin depender de archivos `.uasset` que aún no existen.
- Placeholder visual (cubo de Engine) escalado a la cápsula; se sustituirá por un personaje real.
- Configuración de render: Lumen, Nanite, Virtual Shadow Maps, DX12, sin lightmaps estáticos.
- Script Python que genera un nivel de pruebas con suelo, bloques, vehículo, sol, cielo y niebla.

No implementado todavía (no lo vamos a fingir): conducción y física de vehículos, excavadora, terreno deformable, carga de materiales, contratos, economía, IA, mundo abierto, menús, guardado.

---

## 3. Lo que tienes que hacer ahora

### Paso 1 — Instalar Unreal Engine

Se está instalando **Unreal Engine 5.8** en `C:\Program Files\Epic Games\UE_5.8`. Espera a que termine.

Si tuvieras que reinstalarlo, ve a **Epic Games Launcher → Biblioteca → Motor → +**, elige la versión y, si C: va justo de espacio, cambia la ubicación a `J:\Epic Games\UE_5.8`.

El proyecto ya está asociado a `"EngineAssociation": "5.8"`. Si instalas otra versión, cambia ese valor en `ConstructionWorld.uproject` (o elige la versión en el diálogo al abrir el proyecto).

### Paso 2 — Abrir y compilar el proyecto

1. Doble clic en `ConstructionWorld.uproject`.
2. Si pregunta con qué versión abrir, elige la 5.8 instalada.
3. Si avisa de que faltan módulos y ofrece recompilar, pulsa **Sí**. La primera compilación tarda varios minutos.
4. Alternativa si no genera el proyecto: clic derecho en `ConstructionWorld.uproject` → **Generate Visual Studio project files**, abre `ConstructionWorld.sln`, configura `Development Editor | Win64` y compila.

Compilación por línea de comandos (sustituye la ruta si instalaste en otro sitio):

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" ConstructionWorldEditor Win64 Development -Project="C:\Users\Jhony\Desktop\Juego\ConstructionWorld.uproject" -WaitMutex
```

### Paso 3 — Generar el nivel de pruebas

El proyecto apunta a `/Game/Maps/TestLevel`, que todavía no existe. Con el editor abierto:

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

---

## 4. Problemas frecuentes

- **UAC al instalar Visual Studio**: acepta el aviso; sin permisos de administrador no se puede instalar.
- **"The following modules are missing or built with a different engine version"**: pulsa Sí para recompilar; si falla, comprueba que Visual Studio tiene el workload "Desarrollo de juegos con C++" y el Windows 11 SDK.
- **El editor se abre sin mapa**: ejecuta el script del Paso 3 o crea el mapa manualmente.
- **No se mueve el personaje**: verifica que `DefaultInput.ini` carga `EnhancedPlayerInput`/`EnhancedInputComponent` y que el GameMode global es `/Script/ConstructionWorld.CWGameMode`.
- **Git LFS**: los `.uasset`/`.umap` se guardan con LFS; no borres `.gitattributes`.

---

## 5. Siguiente hito

1. Validar esta compilación contigo (errores reales de compilación incluidos).
2. Cerrar Fase 2: primera persona con malla real, animaciones y mapa de controles configurable.
3. Fase 3: conducción, motor, excavadora y camión volquete funcionales.
4. Fase 4: prototipo de terreno deformable (la decisión técnica más importante del proyecto).

Objetivo del MVP: aceptar un contrato, excavar 40 m³, cargar el camión, transportar, descargar y cobrar, con guardado de partida.
