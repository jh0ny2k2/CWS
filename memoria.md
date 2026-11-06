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
