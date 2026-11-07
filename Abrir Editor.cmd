@echo off
setlocal
cd /d "%~dp0"
set "UE_BIN=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
if not exist "%UE_BIN%" (
    echo Error: No se encontro Unreal Engine 5.8 en %UE_BIN%
    pause
    exit /b 1
)
echo Abriendo Construction World Simulator en Unreal Editor 5.8...
start "" "%UE_BIN%" "%~dp0ConstructionWorld.uproject"
