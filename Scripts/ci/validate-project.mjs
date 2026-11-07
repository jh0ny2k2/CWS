#!/usr/bin/env node
/**
 * Validador de la estructura de Unreal (CI).
 *
 * Comprueba el .uproject, los targets, el modulo C++ y que las cabeceras
 * publicas con implementacion tengan su .cpp correspondiente.
 */

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..", "..");
const errors = [];

function fail(message) {
  errors.push(message);
}

function readJson(relativePath) {
  try {
    return JSON.parse(fs.readFileSync(path.join(root, relativePath), "utf8"));
  } catch (error) {
    fail(`${relativePath}: JSON invalido (${error.message})`);
    return null;
  }
}

function readText(relativePath) {
  const full = path.join(root, relativePath);
  if (!fs.existsSync(full)) {
    fail(`Falta ${relativePath}`);
    return "";
  }
  return fs.readFileSync(full, "utf8");
}

// .uproject
const project = readJson("ConstructionWorld.uproject");
if (project) {
  if (project.EngineAssociation !== "5.8") {
    fail(`ConstructionWorld.uproject: EngineAssociation debe ser "5.8" (es "${project.EngineAssociation}")`);
  }
  const modules = project.Modules || [];
  if (!modules.some((module) => module.Name === "ConstructionWorld")) {
    fail('ConstructionWorld.uproject: falta el modulo "ConstructionWorld"');
  }
  if (modules.length === 0) {
    fail("ConstructionWorld.uproject: no declara ningun modulo");
  }
}

// Targets
for (const target of [
  "Source/ConstructionWorld.Target.cs",
  "Source/ConstructionWorldEditor.Target.cs",
]) {
  const text = readText(target);
  if (text && !text.includes("BuildSettingsVersion.V7")) {
    fail(`${target}: debe usar BuildSettingsVersion.V7`);
  }
  if (text && !text.includes("Unreal5_8")) {
    fail(`${target}: debe usar IncludeOrderVersion.Unreal5_8`);
  }
}

// Modulo
const buildCs = readText("Source/ConstructionWorld/ConstructionWorld.Build.cs");
for (const dependency of ["Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "ProceduralMeshComponent"]) {
  if (buildCs && !buildCs.includes(`"${dependency}"`)) {
    fail(`ConstructionWorld.Build.cs: falta la dependencia ${dependency}`);
  }
}

// Pares cabecera/implementacion obligatorios
const requiredPairs = [
  "CWGameMode",
  "CWPlayerCharacter",
  "CWVehicleBase",
  "CWExcavator",
  "CWDumpTruck",
  "CWDumpZone",
  "CWInteractionComponent",
  "CWHUD",
  "CWLog",
  "CWPerformanceSubsystem",
  "CWTestRunner",
  "CWTestSubsystem",
  "CWTerrainTypes",
  "CWTerrainField",
  "CWTerrainSubsystem",
  "CWStockpile",
];

for (const name of requiredPairs) {
  for (const [folder, extension] of [
    ["Public", ".h"],
    ["Private", ".cpp"],
  ]) {
    const relative = `Source/ConstructionWorld/${folder}/${name}${extension}`;
    if (!fs.existsSync(path.join(root, relative))) {
      fail(`Falta ${relative}`);
    }
  }
}

// Convencion de logs: nada de LogTemp en Source/
function walk(directory) {
  for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
    const full = path.join(directory, entry.name);
    if (entry.isDirectory()) walk(full);
    else if (/\.(h|cpp)$/.test(entry.name)) {
      const text = fs.readFileSync(full, "utf8");
      if (text.includes("LogTemp")) {
        fail(`${path.relative(root, full)}: usa CWLog.h en lugar de LogTemp`);
      }
    }
  }
}

const sourceDir = path.join(root, "Source");
if (fs.existsSync(sourceDir)) walk(sourceDir);

if (errors.length > 0) {
  for (const message of errors) {
    console.error(`ERROR: ${message}`);
  }
  console.error(`\nValidacion de proyecto fallida: ${errors.length} error(es).`);
  process.exit(1);
}

console.log("Estructura del proyecto UE correcta.");
