# Memoria del proyecto

Historial de acciones realizadas. Se lee antes de cualquier tarea y se actualiza al terminar cada una.

---

## [2026-11-06] Creación del sistema de memoria

- Se ha creado `AGENTS.md` con la regla de leer `memoria.md` antes de cualquier acción y de registrar cada tarea aquí.
- Se ha creado `memoria.md` como historial de acciones.
- Archivos afectados: `AGENTS.md`, `memoria.md`.
- Nota: hay que reiniciar opencode para que empiece a cargar `AGENTS.md` automáticamente.

## [2026-11-06] Aclaración sobre el formato de `memoria.md`

- Se ha confirmado que `memoria.md` es incremental: solo se añaden entradas nuevas al final, nunca se borran las anteriores.
- Archivos afectados: `memoria.md`.

## [2026-11-06] Consulta sobre lectura de PDF/DOCX

- El usuario pregunta si puedo leer PDFs o DOCX. Se comprobó el entorno: no hay Python/pip ni pandoc instalados.
- Conclusión: PDF se lee de forma nativa con la herramienta de lectura; DOCX requiere extraer el texto (PowerShell descomprimiendo `word/document.xml`, o Word vía COM si está instalado).
- Archivos afectados: `memoria.md`.

## [2026-11-06] Inventario y análisis de la carpeta del proyecto

- Se recorrió todo el proyecto y se leyeron `README.md`, `Hojaruta.md`, `promt.md`, `docs/ARQUITECTURA.md`, `Config/*.ini` y las cabeceras C++ de `Source/`.
- Resultado: proyecto Unreal Engine 5.8 en C++ llamado Construction World (simulador de construcción), en estado Fase 1 + parte de Fase 2, sin compilar (no hay `Binaries/`, `Intermediate/` ni `.sln`) y con `Content/Maps` vacío (falta `TestLevel`).
- Comprobado que Unreal Engine 5.8 ya está instalado en `C:\Program Files\Epic Games\UE_5.8` (el README aún decía "instalándose").
- El repositorio git existe pero no tiene ningún commit (todo sin trackear).
- Archivos afectados: `memoria.md` (solo lectura del resto).

## [2026-11-06 16:45] Extracción y archivo del Plan Maestro (PDF de 35 páginas)

- Se instaló `pdf-parse` en una carpeta temporal y se extrajo el texto completo del PDF del Plan Maestro.
- El texto se archivó en `docs/Plan_Maestro_extraido.txt` (1393 líneas) y el PDF original en `docs/Plan_Maestro_v1.0.pdf`.
- Se leyó el plan completo para construir la hoja de ruta H0–H8 del panel.
- Archivos afectados: `docs/Plan_Maestro_extraido.txt`, `docs/Plan_Maestro_v1.0.pdf`.

## [2026-11-06 17:25] Panel de seguimiento web (hoja de ruta + kanban)

- Creado un panel local sin dependencias servido por Node en `http://127.0.0.1:8100`.
- `web/index.html`: hoja de ruta con 10 hitos (H0–H8 + H1-A/H1-B), 102 tareas, progreso global, filtros y buscador.
- `web/kanban.html`: tablero TO DO / DOING / DONE / BUGS ENCONTRADOS.
- Datos centralizados en `web/assets/data.js`; servidor en `web/server.js`; lanzador `web/Iniciar panel.cmd`.
- Verificadas todas las rutas por HTTP (200), sintaxis con `node --check` y renderizado con un smoke test de jsdom.
- Archivos afectados: `web/index.html`, `web/kanban.html`, `web/assets/style.css`, `web/assets/data.js`, `web/assets/app.js`, `web/assets/kanban.js`, `web/server.js`, `web/Iniciar panel.cmd`, `README.md`.

## [2026-11-06 18:05] ADR, Git LFS, versión 5.8 y commit inicial

- Actualizados `README.md`, `Hojaruta.md`, `promt.md` y `docs/ARQUITECTURA.md` a Unreal Engine 5.8, y la tabla de fases renombrada a hitos H0–H8.
- Creados `docs/ADR/ADR-001-motor-y-version.md`, `ADR-002-backend-terreno.md` y `ADR-003-git-lfs.md`.
- Añadido `*.pdf` a Git LFS y verificado que el PDF del plan se guarda como puntero LFS.
- Creado el commit inicial `d24acd0` con 43 archivos (antes el repo no tenía ningún commit).
- Archivos afectados: `.gitattributes`, `README.md`, `Hojaruta.md`, `promt.md`, `docs/ARQUITECTURA.md`, `docs/ADR/*`.

## [2026-11-06 18:22] Kanban: detalle por tarjeta y columna de bugs arreglados

- Cada tarjeta del kanban se puede abrir (clic o Enter) y muestra un modal con estado, hito, severidad, notas y bitácora fechada de lo realizado.
- Añadida la columna «BUGS ARREGLADOS»: los bugs con estado «Resuelto» se mueven automáticamente allí.
- Añadidas bitácoras reales a tareas y bugs en `web/assets/data.js` (incluye el propio panel como tarea F0.08).
- Smoke test ampliado: 5 columnas, 2 bugs arreglados, apertura/cierre del modal con clic, Escape y botón X.
- Archivos afectados: `web/kanban.html`, `web/assets/kanban.js`, `web/assets/style.css`, `web/assets/data.js`.

## [2026-11-06 18:26] UE 5.8: instalación real, Visual Studio y primer intento de compilación

- La instalación de UE 5.8 avanzó: ya existen `Engine\Binaries\Win64\UnrealEditor.exe`, `UnrealBuildTool` y `Build.bat`; sigue extrayendo (13,5 GB en `Engine`, 15,6 GB en `.egstore`) y aún falta `Engine\Source`.
- El primer intento de compilación falló porque `Build.bat` exige ejecutarse desde su propio directorio y porque falta `Engine\Source`; no es un error del código del proyecto.
- Verificado Visual Studio 2022 Community con el conjunto de herramientas C++ (`VC.Tools.x86.x64`). Blender no está instalado.
- Lanzado un vigilante en segundo plano (`%TEMP%\opencode\cwwatch.ps1`) que espera a `Engine\Source` y compila `ConstructionWorldEditor Win64 Development` automáticamente.
- Archivos afectados: ninguno del proyecto (solo comprobaciones).

## [2026-11-06 18:28] Remoto de GitHub configurado (push pendiente de credenciales)

- Añadido el remoto `origin` → `https://github.com/jh0ny2k2/CWS.git` y renombrada la rama local `master` a `main`.
- El repositorio remoto existe y está vacío, pero `git push` falla: el token guardado en el Credential Manager ya no es válido ("Invalid username or token").
- Pendiente: que el usuario inicie sesión o facilite un token con permiso de escritura; después se subirán los commits `d24acd0` y `cffa2d7`.
- Archivos afectados: configuración de git (sin cambios de archivos).

## [2026-11-06 18:40] Push a GitHub completado

- Autenticado con el token clásico facilitado por el usuario: el token fine-grained no tenía permiso de escritura (403).
- Subidos a `https://github.com/jh0ny2k2/CWS.git` los 3 commits (`d24acd0`, `cffa2d7`, `1309219`) y el objeto LFS del PDF del plan.
- La URL del remoto se dejó limpia (`https://github.com/jh0ny2k2/CWS.git`), sin token incrustado en `.git/config`.
- La rama local `main` queda en seguimiento de `origin/main`.
- Archivos afectados: ninguno (operación de git).

## [2026-11-06 18:55] Primera compilación: error de ajustes de build corregido

- La instalación de UE 5.8 seguía copiando archivos muy despacio, pero ya tenía `Engine\Source`, `UnrealBuildTool` y `UnrealEditor`; se intentó compilar sin esperar al final.
- Primer intento: UBT rechazó el target porque `BuildSettingsVersion.V5` mantiene avisos en `Off` mientras `UnrealEditor` (motor instalado) los espera en `Error`; los productos de build se comparten.
- Corregidos `Source/ConstructionWorld.Target.cs` y `Source/ConstructionWorldEditor.Target.cs`: `BuildSettingsVersion.V7` e `IncludeOrderVersion.Unreal5_8`.
- Relanzada la compilación de `ConstructionWorldEditor Win64 Development`.
- Archivos afectados: `Source/ConstructionWorld.Target.cs`, `Source/ConstructionWorldEditor.Target.cs`.

## [2026-11-06 18:53] Compilación correcta del proyecto C++

- `Build.bat ConstructionWorldEditor Win64 Development` terminó con **Result: Succeeded** (13/13 acciones, 182 s).
- Se compilaron los 6 `.cpp` (personaje, vehículo, interacción, HUD, GameMode y módulo) y se enlazó `Binaries/Win64/UnrealEditor-ConstructionWorld.dll` (311 KB).
- El Enhanced Input creado por código compila sin errores en 5.8.
- Herramientas usadas: Visual Studio 14.44 / MSVC 14.44.35207, Windows SDK 10.0.22621.0.
- Archivos afectados: `Binaries/Win64/*`, `Intermediate/Build/*` (generados, ignorados por git).

## [2026-11-06 18:57] Nivel de pruebas generado y verificado

- `cw_build_test_level.py` creó `/Game/Maps/TestLevel` (26 KB) con 0 errores y 0 avisos.
- Añadido `Scripts/Python/cw_verify_test_level.py`: reabre el nivel y lista los actores `CW_`.
- Verificación real: 10 actores presentes (suelo, bloques A/B/C, `CW_TestVehicle` del módulo C++, PlayerStart, sol, SkyAtmosphere, SkyLight y niebla).
- Archivos afectados: `Content/Maps/TestLevel.umap`, `Scripts/Python/cw_verify_test_level.py`.

## [2026-11-06 19:00] Auditoría de hardware cerrada

- Registrado el equipo: AMD Ryzen 5 3600 (6 núcleos/12 hilos), 15,9 GB de RAM, NVIDIA RTX 3060 y 63 GB libres en C:.
- Medido el primer tiempo de compilación real: 182,6 s, con UBT limitado a 3 acciones en paralelo por memoria disponible.
- Archivos afectados: `web/assets/data.js`, `docs/ARQUITECTURA.md`, `README.md`.

## [2026-11-06 19:02] Estado real del motor y documentación sincronizada

- El motor sigue siendo funcional (29,9 GB), pero el log de Epic registra `installation failed` a las 17:45 y `LauncherInstalled.dat` no incluye UE 5.8: el launcher no lo marca como instalado. Se anota como pendiente, no bloquea compilar ni ejecutar.
- Actualizados `README.md`, `docs/ARQUITECTURA.md` y el panel: F0.02 y F0.05 hechos, BUG-003 resuelto, BUG-001 actualizado con el fallo de registro.
- Panel verificado otra vez con jsdom: 5 columnas, 104 tarjetas, 4 bugs abiertos y 4 arreglados, modal y cierre correctos.
- Nota: el motor añadió solo `AndroidFileServerRuntimeSettings` a `Config/DefaultEngine.ini` al arrancar; se conserva (token inocuo).
- Archivos afectados: `README.md`, `docs/ARQUITECTURA.md`, `web/assets/data.js`, `memoria.md`, `Config/DefaultEngine.ini`.


## [2026-11-06 22:15] Conducción de vehículo, corrección de bugs BUG-006/007/008 y compilación optimizada con UBA local

- Se implementó la conducción WASD completa y rotación en `ACWVehicleBase` (`IA_DriveForward`, `IA_DriveRight`, `MaxDriveSpeed`, `TurnRate`) conectada a `IMC_Vehicle`.
- Se corrigió BUG-006: `ExitVehicle` ahora llama a `FindSafeExitLocation()`, que realiza un sweep de colisión con cápsula Pawn en 4 candidatos antes de teleportar al personaje, evitando quedar incrustado en objetos o en el terreno.
- Se corrigió BUG-007: el acoplamiento del conductor (`AttachToComponent`) ahora usa `BodyMesh` y valida si existe el socket `DriverSeatSocket`; en caso contrario se ancla de forma segura al origen del componente con log informativo.
- Se corrigió BUG-008: `ACWHUD` detecta si el peón poseído es un vehículo y delega la consulta de interacción al personaje conductor (`Driver`), además de mostrar indicaciones en pantalla: `Conduciendo | WASD: mover | F: salir`.
- Se implementaron los límites de cabeceo (pitch [-60°, 60°]) y colisión con `ProbeSize=12` en `ACWPlayerCharacter` (F1.02).
- Se resolvió el bloqueo de compilación de UE 5.8: el acelerador UBA intentaba escribir sesiones en `C:\ProgramData` provocando un bucle infinito de denegaciones de acceso; se configuró `BuildConfiguration.xml` con `RootDir` en `Intermediate\UBA`, logrando compilación limpia en 33,2 s (8 acciones, DLL actualizada a 320 KB).
- Actualizado el panel kanban y hoja de ruta en `web/assets/data.js` con las tareas F1.01-F1.06 completadas y los bugs BUG-006, BUG-007 y BUG-008 resueltos.
- Archivos afectados: `Source/ConstructionWorld/Public/CWVehicleBase.h`, `Source/ConstructionWorld/Private/CWVehicleBase.cpp`, `Source/ConstructionWorld/Public/CWHUD.h`, `Source/ConstructionWorld/Private/CWHUD.cpp`, `Source/ConstructionWorld/Private/CWPlayerCharacter.cpp`, `Saved/UnrealBuildTool/BuildConfiguration.xml`, `web/assets/data.js`, `memoria.md`.
