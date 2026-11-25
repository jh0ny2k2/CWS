# Ejecuta las pruebas automaticas headless del proyecto y resume el resultado.
#
# Ejemplos:
#   .\Scripts\RunTests.ps1                      # todas las suites
#   .\Scripts\RunTests.ps1 -Suite Terrain,Truck # solo algunas
#   .\Scripts\RunTests.ps1 -Build               # compila antes de probar
#
# Cada suite arranca TestLevel en modo -game -nullrhi, espera la linea
# "CW_TEST_RESULT: PASS|FAIL" en su log y comprueba el codigo de salida.
# Los logs quedan en Saved/Tests/<suite>.log. Sale con 1 si alguna falla.

param(
    [ValidateSet("EnterExit", "Excavator", "Truck", "Terrain", "Ground", "Contract")]
    [string[]]$Suite = @("EnterExit", "Excavator", "Truck", "Terrain", "Ground", "Contract"),

    [int]$Cycles = 10,

    [int]$TimeoutSeconds = 300,

    [switch]$Build,

    # Argumentos extra para el motor, p. ej. -ExtraArgs "-LogCmds=\"LogCWVehicle Verbose\""
    [string]$ExtraArgs = "",

    [string]$EnginePath = "C:\Program Files\Epic Games\UE_5.8"
)

$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$project = Join-Path $root "ConstructionWorld.uproject"
$editor = Join-Path $EnginePath "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$logDir = Join-Path $root "Saved\Tests"

if (-not (Test-Path $editor)) {
    throw "No se encuentra UnrealEditor-Cmd.exe en '$EnginePath'."
}

if ($Build) {
    & (Join-Path $PSScriptRoot "Build.ps1") -EnginePath $EnginePath
}

New-Item -ItemType Directory -Force -Path $logDir | Out-Null

$flags = @{
    EnterExit = "-CWTestEnterExit=$Cycles"
    Excavator = "-CWTestExcavator"
    Truck     = "-CWTestTruck"
    Terrain   = "-CWTestTerrain"
    Ground    = "-CWTestGround"
    Contract  = "-CWTestContract"
}

$results = @()
foreach ($name in $Suite) {
    $log = Join-Path $logDir "$name.log"
    Remove-Item $log -ErrorAction SilentlyContinue

    $engineArgs = @(
        "`"$project`"", "/Game/Maps/TestLevel",
        "-game", "-nullrhi", "-nosound", "-unattended", "-nosplash", "-nopause",
        $flags[$name], "-CWTestExit", $ExtraArgs,
        "-abslog=`"$log`"", "-stdout", "-FullStdOutLogOutput"
    )

    Write-Host ("[{0}] ejecutando..." -f $name) -NoNewline
    $watch = [Diagnostics.Stopwatch]::StartNew()
    $proc = Start-Process -FilePath $editor -ArgumentList ($engineArgs | Where-Object { $_ }) -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput (Join-Path $logDir "$name.stdout.txt")
    # Sin cachear el handle, PowerShell deja ExitCode vacio al terminar el proceso.
    $null = $proc.Handle

    if (-not $proc.WaitForExit($TimeoutSeconds * 1000)) {
        $proc.Kill()
        $exit = -1
        $line = "TIMEOUT tras $TimeoutSeconds s"
    } else {
        $exit = $proc.ExitCode
        $line = ""
        if (Test-Path $log) {
            $hit = Select-String -Path $log -Pattern "CW_TEST_RESULT: .*" | Select-Object -Last 1
            if ($hit) { $line = $hit.Matches[0].Value }
        }
        if (-not $line) { $line = "sin linea CW_TEST_RESULT (ver $log)" }
    }
    $watch.Stop()

    $pass = ($exit -eq 0) -and ($line -like "CW_TEST_RESULT: PASS*")
    $color = if ($pass) { "Green" } else { "Red" }
    Write-Host (" {0} ({1:N0} s, exit {2})" -f ($(if ($pass) { "PASS" } else { "FAIL" })), $watch.Elapsed.TotalSeconds, $exit) -ForegroundColor $color
    Write-Host ("    {0}" -f $line)

    $results += [pscustomobject]@{ Suite = $name; Pass = $pass; Exit = $exit; Seconds = [math]::Round($watch.Elapsed.TotalSeconds); Result = $line }
}

$failed = @($results | Where-Object { -not $_.Pass })
Write-Host ""
Write-Host ("Resumen: {0}/{1} suites en PASS. Logs en {2}" -f ($results.Count - $failed.Count), $results.Count, $logDir)
if ($failed.Count) { exit 1 }
exit 0
