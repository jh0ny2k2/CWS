# Reglas obligatorias del proyecto

## 1. Leer `memoria.md` antes de nada

Antes de responder, planificar, buscar, leer, escribir o ejecutar cualquier acción, **lee siempre el archivo `memoria.md`** que está en la raíz del proyecto. Contiene el historial de lo ya hecho y el estado actual.

## 2. Registrar todo en `memoria.md`

Cada vez que completes cualquier acción (crear/editar archivos, ejecutar comandos, investigar, implementar una funcionalidad, corregir un error, etc.), **añade una entrada al final de `memoria.md`** indicando qué has hecho.

Formato de cada entrada:

```md
## [AAAA-MM-DD HH:MM] Título breve de la acción

- Qué se ha hecho
- Archivos afectados (rutas relativas)
- Resultado / notas
```

- Añade siempre al final, sin borrar entradas anteriores.
- Sé concreto y breve: qué cambió y dónde.
- Si la acción falla o queda incompleta, indícalo también.

## 3. El panel de seguimiento: kanban y hoja de ruta

El proyecto tiene dos vistas de seguimiento que **siempre** deben reflejar la realidad del trabajo:

- **Hoja de ruta:** `Hojaruta.md` (documento) y `web/index.html` (vista web). Los hitos y tareas viven en `web/assets/data.js` dentro de `CW.phases` → `phase.tasks`.
- **Kanban:** `web/kanban.html`, alimentado por los mismos datos de `web/assets/data.js`.
  - Tareas: `CW.phases[].tasks[]` con `id` (F0.01…), `title`, `status` (`todo` / `doing` / `done`), `evidence`, `note` y `log[]`.
  - Bugs: `CW.bugs[]` con `id` (BUG-001…), `title`, `detail`, `severity`, `status` (`En curso` / `Resuelto`) y `log[]`.
- El panel se sirve en `http://127.0.0.1:8100` con `web/Iniciar panel.cmd` (o `node web/server.js`).

## 4. Actualizar las tarjetas al hacer cualquier trabajo

Siempre que se trabaje en algo relacionado con una tarjeta del kanban o de la hoja de ruta:

1. **Comentar la tarjeta.** Añadir una entrada fechada al final de su `log[]` en `web/assets/data.js`:
   ```js
   { date: "AAAA-MM-DD HH:MM", text: "Qué se ha hecho y resultado." }
   ```
   El comentario debe ser concreto (qué se cambió, dónde y con qué resultado). No borrar entradas anteriores del log.
2. **Cambiar el estado si avanza.** Pasar la tarjeta de `todo` a `doing` al empezar, y dejar constancia del progreso en `note`/`evidence` si aplica.
3. **Cerrar la tarjeta si se resuelve.** Cuando el trabajo esté terminado y verificado:
   - Tarea: `status: "done"` y rellenar `evidence` con la prueba de que está hecha.
   - Bug: `status: "Resuelto"` y actualizar `detail` con la solución; el kanban lo moverá solo a «BUGS ARREGLADOS».
4. **Sincronizar la hoja de ruta.** Si la tarjeta pertenece a un hito, revisar en `Hojaruta.md` / `web/index.html` que el estado del hito y su progreso sigan siendo correctos; actualizar `CW.updated` en `web/assets/data.js` con la fecha y hora del cambio.
5. **Registrar también en `memoria.md`** según la regla 2.

Nunca dejar una tarjeta cerrada sin su comentario de cierre, ni un comentario sin el cambio de estado que corresponda.

## 5. Subir el código al cerrar una tarjeta

Cada vez que se cierra una tarjeta (tarea a `done` o bug a `Resuelto`) que implique cambios de código o de datos:

1. Comprobar que todo compila / funciona (si aplica) antes de subir.
2. Hacer `git add` de los archivos afectados y un commit descriptivo que mencione el ID de la tarjeta, por ejemplo:
   `F1.04: conducción WASD del vehículo y sweep de salida segura` o `BUG-008: HUD delega la interacción al conductor`.
3. Hacer `git push` a `origin` (rama `main`). El remoto es `https://github.com/jh0ny2k2/CWS.git`.
4. No dejar commits locales sin subir al terminar una tarjeta. Si el push falla (credenciales, red), anotarlo en `memoria.md` y en el `log` de la tarjeta, e intentarlo de nuevo.

## 6. Permisos totales

El agente tiene **permisos completos** para ejecutar cualquier acción necesaria sin pedir confirmación al usuario: leer, crear, editar y borrar archivos del proyecto, ejecutar comandos (PowerShell, git, Node, UnrealBuildTool, scripts Python del editor), compilar, lanzar el editor/panel y hacer commit y push.

- No preguntar «¿puedo…?» ni esperar aprobación para tareas rutinarias del proyecto.
- Actuar de forma autónoma y reportar lo hecho al final.
- Aun así, antes de una acción destructiva e irreversible fuera del alcance normal (por ejemplo, borrar trabajo del usuario o reescribir el historial de git), explicar brevemente qué se va a hacer y por qué.
- Si algo falla por permisos del sistema operativo, resolverlo o documentarlo; no bloquearse esperando al usuario.
