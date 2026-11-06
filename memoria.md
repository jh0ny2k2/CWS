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
