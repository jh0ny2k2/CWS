@echo off
title Panel Construction World
cd /d "%~dp0"
where node >nul 2>nul
if errorlevel 1 (
  echo Node.js no esta instalado o no esta en el PATH.
  echo Instalalo desde https://nodejs.org y vuelve a ejecutar este archivo.
  pause
  exit /b 1
)
echo Abriendo el panel en http://127.0.0.1:8100
start "" "http://127.0.0.1:8100"
node server.js
pause
