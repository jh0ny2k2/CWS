@echo off
setlocal
cd /d "%~dp0"
set "UE_BIN=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
if not exist "%UE_BIN%" (
    echo Error: No se encontro Unreal Engine 5.8 en %UE_BIN%
    pause
    exit /b 1
)
echo =======================================================
echo  Construction World Simulator - Modo Juego (TestLevel)
echo =======================================================
echo.
echo Controles:
echo   Personaje: WASD mover, Raton mirar, Espacio saltar, F entrar/interactuar
echo   Vehiculo:  WASD conducir, F salir
echo   Excavadora: flechas/raton para mover torreta/brazo, T excavar/descargar, V camaras
echo   Volquete:   WASD mover, R/F bascular caja
echo   Consola:    tecla ~ (cw.Test.Terrain, cw.Test.Excavator, stat fps)
echo.
start "" "%UE_BIN%" "%~dp0ConstructionWorld.uproject" /Game/Maps/TestLevel -game -windowed -ResX=1600 -ResY=900 -log
