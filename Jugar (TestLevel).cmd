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
echo   Personaje:  WASD mover, raton mirar, rueda zoom, Espacio saltar, E entrar, V camara 1a/3a
echo   Mando:      stick izq. mover, stick der. mirar, A saltar, X entrar, B salir
echo   Vehiculo:   WASD conducir, F salir
echo   Excavadora: G motor, R modo traslado/excavacion, W/S pluma, A/D brazo,
echo               Q/E torreta, Z/X cuchara, T excavar/descargar, C camara
echo   Volquete:   WASD conducir, B bascular caja, T descargar
echo   Consola:    tecla ~ (cw.Test.Terrain, cw.Test.Excavator, cw.Weather.Rain 1, stat fps)
echo.
start "" "%UE_BIN%" "%~dp0ConstructionWorld.uproject" /Game/Maps/TestLevel -game -windowed -ResX=1600 -ResY=900 -log
