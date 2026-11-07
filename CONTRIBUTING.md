# Contribuir a Construction World

Este documento fija el flujo de trabajo del repositorio (**F0.04**). Las reglas del agente están en `AGENTS.md`.

## Ramas

| Rama | Uso |
|---|---|
| `main` | Versión estable y jugable. Solo recibe merges probados. |
| `develop` | Integración continua del trabajo en curso. |
| `feature/<id>-<slug>` | Una tarea o bug concreto. Ej.: `feature/F0.06-metricas`, `feature/BUG-008-hud`. |

Flujo recomendado:

1. Crear la rama desde `develop`: `git switch -c feature/F0.06-metricas develop`.
2. Implementar y compilar en local (`Scripts/Build.ps1`).
3. Comentar y cerrar la tarjeta en `web/assets/data.js` (regla 4 de `AGENTS.md`).
4. Commit con el ID de la tarjeta.
5. Push de la rama y pull request a `develop`; cuando esté probado, `develop` → `main`.

## Commits

Formato: `<ID>: <resumen imperativo>`.

```
F0.06: logging por categorias, metricas de FPS y convenciones de unidades
BUG-008: el HUD delega la interaccion al conductor del vehiculo
```

- Un commit por tarjeta o por cambio coherente.
- No mezclar refactors grandes con correcciones de bugs.
- Al cerrar una tarjeta, el push es obligatorio (regla 5 de `AGENTS.md`).

## Compilación

```powershell
.\Scripts\Build.ps1                 # Editor Development
.\Scripts\Build.ps1 -Game           # Juego Development
.\Scripts\Build.ps1 -Configuration Shipping -Game
```

## Validación antes de subir

```powershell
node Scripts/ci/validate.mjs
node Scripts/ci/validate-project.mjs
.\Scripts\Build.ps1
```

La CI de GitHub ejecuta los dos validadores, la sintaxis de JavaScript y prohíbe `LogTemp`. La compilación con UnrealBuildTool no se puede ejecutar en los runners públicos (requiere Windows + UE 5.8), así que es responsabilidad local antes del push.

## Assets

- Todo asset debe estar registrado en `docs/assets_manifest.json` con fuente y licencia.
- Los binarios se guardan con Git LFS; no desactivar `.gitattributes`.
- Los fuentes editables van en `ArtSource/`.

## Panel de seguimiento

- `web/assets/data.js` es la fuente única de verdad de tareas y bugs.
- Toda tarjeta que se toque debe recibir un comentario fechado en su `log`.
- Al cerrar una tarea: `status: "done"` + `evidence`. Al cerrar un bug: `status: "Resuelto"`.
