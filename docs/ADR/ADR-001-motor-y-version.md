# ADR-001 — Motor y versión: Unreal Engine 5.8

- **Estado:** Aceptado
- **Fecha:** 2026-11-06
- **Decisores:** Jhony Chaves

## Contexto

El proyecto `ConstructionWorld.uproject` declara `"EngineAssociation": "5.8"` y el plan maestro asume las capacidades de Unreal Engine 5 (Nanite, Lumen, World Partition, Chaos, Enhanced Input). La documentación antigua hablaba de "5.6+" de forma genérica, lo que generaba dudas al instalar y compilar.

## Decisión

1. La versión objetivo es **Unreal Engine 5.8** instalada en `C:\Program Files\Epic Games\UE_5.8`.
2. El proyecto se mantiene **C++ como núcleo** (sistemas: vehículos, terreno, contratos, guardado) y **Blueprints para contenido y ajuste**.
3. `DefaultBuildSettings = BuildSettingsVersion.V5` e `IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6` se conservan hasta que la primera compilación en 5.8 demuestre que hay que subirlos.

## Consecuencias

- Todo el equipo debe usar 5.8; no se garantiza compatibilidad con 5.6/5.7.
- La primera compilación es un hito bloqueante de H0.
- Si una API usada por código (Enhanced Input creado en runtime) cambia en 5.8, se corregirá y se anotará aquí.

## Alternativas descartadas

- **UE 5.6/5.7:** sin ventaja clara y obligaría a reinstalar el motor.
- **Unity/Godot:** descartado por el plan maestro y por el trabajo ya invertido en C++ de UE.
