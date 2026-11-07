# Compila el proyecto con UnrealBuildTool.
#
# Ejemplos:
#   .\Scripts\Build.ps1
#   .\Scripts\Build.ps1 -Configuration Shipping
#   .\Scripts\Build.ps1 -Game
#   .\Scripts\Build.ps1 -EnginePath "D:\Epic Games\UE_5.8"

param(
    [ValidateSet("Development", "DebugGame", "Shipping")]
    [string]$Configuration = "Development",

    [switch]$Game,

    [string]$EnginePath = "C:\Program Files\Epic Games\UE_5.8"
)

$ErrorActionPreference = "Stop"

$projectPath = (Resolve-Path (Join-Path $PSScriptRoot "..\ConstructionWorld.uproject")).Path
$target = if ($Game) { "ConstructionWorld" } else { "ConstructionWorldEditor" }
$buildBat = Join-Path $EnginePath "Engine\Build\BatchFiles\Build.bat"

if (-not (Test-Path $buildBat)) {
    throw "No se encuentra Build.bat en '$EnginePath'. Usa -EnginePath con la ruta de UE 5.8."
}

Write-Host "Compilando $target Win64 $Configuration..."
& $buildBat $target Win64 $Configuration "-Project=$projectPath" -WaitMutex

if ($LASTEXITCODE -ne 0) {
    throw "La compilacion fallo con codigo $LASTEXITCODE."
}

Write-Host "Compilacion correcta."
