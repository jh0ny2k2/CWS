#!/usr/bin/env node
/**
 * Validador de datos del proyecto (CI).
 *
 * Comprueba:
 *  - web/assets/data.js: fases, tareas y bugs con IDs unicos y estados validos.
 *  - docs/assets_manifest.json: inventario de assets con licencia y estado.
 *  - Existencia de los documentos y archivos clave del proyecto.
 *
 * Sin dependencias externas: usa el modulo nativo `vm` para cargar data.js.
 */

import fs from "node:fs";
import path from "node:path";
import vm from "node:vm";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..", "..");
const errors = [];
const warnings = [];

function fail(message) {
  errors.push(message);
}

function warn(message) {
  warnings.push(message);
}

function exists(relativePath) {
  return fs.existsSync(path.join(root, relativePath));
}

function readJson(relativePath) {
  try {
    return JSON.parse(fs.readFileSync(path.join(root, relativePath), "utf8"));
  } catch (error) {
    fail(`${relativePath}: JSON invalido (${error.message})`);
    return null;
  }
}

// ---------------------------------------------------------------------------
// 1. Datos del panel
// ---------------------------------------------------------------------------
const TASK_STATUSES = ["todo", "doing", "done"];
const PHASE_STATUSES = ["todo", "doing", "done"];
const BUG_STATUSES = ["En curso", "Resuelto"];
const BUG_SEVERITIES = ["Crítico", "Alto", "Medio", "Bajo"];

const dataPath = "web/assets/data.js";
if (!exists(dataPath)) {
  fail(`Falta ${dataPath}`);
} else {
  const sandbox = { window: {} };
  vm.createContext(sandbox);
  try {
    vm.runInContext(fs.readFileSync(path.join(root, dataPath), "utf8"), sandbox, {
      filename: dataPath,
    });
  } catch (error) {
    fail(`${dataPath}: no se pudo evaluar (${error.message})`);
  }

  const CW = sandbox.window.CW;
  if (!CW) {
    fail(`${dataPath}: no define window.CW`);
  } else {
    for (const field of ["updated", "project", "engine", "phases", "bugs"]) {
      if (!(field in CW)) fail(`${dataPath}: falta el campo CW.${field}`);
    }

    if (typeof CW.updated === "string" && !/^\d{4}-\d{2}-\d{2}( \d{2}:\d{2})?$/.test(CW.updated)) {
      fail(`${dataPath}: CW.updated debe ser YYYY-MM-DD o YYYY-MM-DD HH:MM`);
    }

    const phaseIds = new Set();
    const taskIds = new Set();
    let taskCount = 0;
    let doneCount = 0;

    for (const phase of CW.phases || []) {
      if (!phase.id) fail(`${dataPath}: fase sin id`);
      if (phaseIds.has(phase.id)) fail(`${dataPath}: id de fase duplicado ${phase.id}`);
      phaseIds.add(phase.id);

      if (!PHASE_STATUSES.includes(phase.status)) {
        fail(`${dataPath}: fase ${phase.id} con estado invalido "${phase.status}"`);
      }

      if (!Array.isArray(phase.tasks) || phase.tasks.length === 0) {
        fail(`${dataPath}: fase ${phase.id} sin tareas`);
        continue;
      }

      for (const task of phase.tasks) {
        taskCount += 1;
        if (!task.id) fail(`${dataPath}: tarea sin id en fase ${phase.id}`);
        if (taskIds.has(task.id)) fail(`${dataPath}: id de tarea duplicado ${task.id}`);
        taskIds.add(task.id);

        if (!TASK_STATUSES.includes(task.status)) {
          fail(`${dataPath}: tarea ${task.id} con estado invalido "${task.status}"`);
        }
        if (task.status === "done") {
          doneCount += 1;
          if (!task.evidence) warn(`${dataPath}: tarea ${task.id} marcada done sin evidence`);
          if (!Array.isArray(task.log) || task.log.length === 0) {
            warn(`${dataPath}: tarea ${task.id} marcada done sin bitacora`);
          }
        }
        for (const entry of task.log || []) {
          if (!entry.date || !entry.text) {
            fail(`${dataPath}: entrada de bitacora incompleta en tarea ${task.id}`);
          }
        }
      }
    }

    const bugIds = new Set();
    for (const bug of CW.bugs || []) {
      if (!bug.id) fail(`${dataPath}: bug sin id`);
      if (bugIds.has(bug.id)) fail(`${dataPath}: id de bug duplicado ${bug.id}`);
      bugIds.add(bug.id);

      if (!BUG_STATUSES.includes(bug.status)) {
        fail(`${dataPath}: bug ${bug.id} con estado invalido "${bug.status}"`);
      }
      if (!BUG_SEVERITIES.includes(bug.severity)) {
        fail(`${dataPath}: bug ${bug.id} con severidad invalida "${bug.severity}"`);
      }
      if (!bug.detail) warn(`${dataPath}: bug ${bug.id} sin detail`);
      if (!Array.isArray(bug.log) || bug.log.length === 0) {
        warn(`${dataPath}: bug ${bug.id} sin bitacora`);
      }
    }

    console.log(
      `Panel: ${phaseIds.size} fases, ${taskCount} tareas (${doneCount} hechas), ${bugIds.size} bugs.`
    );
  }
}

// ---------------------------------------------------------------------------
// 2. Manifiesto de assets
// ---------------------------------------------------------------------------
const ASSET_STATUSES = ["planificado", "prototipo", "adquirido", "integrado"];
const manifestPath = "docs/assets_manifest.json";
const manifest = readJson(manifestPath);
if (!manifest) {
  fail(`Falta o es invalido ${manifestPath}`);
} else {
  if (!Array.isArray(manifest.assets) || manifest.assets.length === 0) {
    fail(`${manifestPath}: falta el array assets`);
  } else {
    const ids = new Set();
    for (const asset of manifest.assets) {
      for (const field of ["id", "title", "category", "status", "license", "source"]) {
        if (!asset[field]) fail(`${manifestPath}: asset sin campo ${field}`);
      }
      if (ids.has(asset.id)) fail(`${manifestPath}: id de asset duplicado ${asset.id}`);
      ids.add(asset.id);
      if (!ASSET_STATUSES.includes(asset.status)) {
        fail(`${manifestPath}: asset ${asset.id} con estado invalido "${asset.status}"`);
      }
    }
    console.log(`Assets: ${ids.size} entradas en el manifiesto.`);
  }
}

// ---------------------------------------------------------------------------
// 3. Documentos y estructura clave
// ---------------------------------------------------------------------------
const requiredFiles = [
  "ConstructionWorld.uproject",
  "Config/DefaultEngine.ini",
  "Config/DefaultGame.ini",
  "Config/DefaultInput.ini",
  "Source/ConstructionWorld.Target.cs",
  "Source/ConstructionWorldEditor.Target.cs",
  "Source/ConstructionWorld/ConstructionWorld.Build.cs",
  "Source/ConstructionWorld/Public/CWLog.h",
  "Source/ConstructionWorld/Public/CWUnits.h",
  "Source/ConstructionWorld/Public/CWPerformanceSubsystem.h",
  "docs/CONVENCIONES.md",
  "docs/ASSETS.md",
  "README.md",
  "Hojaruta.md",
  "AGENTS.md",
  "memoria.md",
];

for (const file of requiredFiles) {
  if (!exists(file)) fail(`Falta el archivo obligatorio ${file}`);
}

// ---------------------------------------------------------------------------
// Resumen
// ---------------------------------------------------------------------------
for (const message of warnings) {
  console.warn(`AVISO: ${message}`);
}

if (errors.length > 0) {
  for (const message of errors) {
    console.error(`ERROR: ${message}`);
  }
  console.error(`\nValidacion fallida: ${errors.length} error(es).`);
  process.exit(1);
}

console.log("Validacion correcta: panel, assets y estructura del proyecto OK.");
