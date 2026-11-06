# ADR-003 — Control de versiones y Git LFS

- **Estado:** Aceptado
- **Fecha:** 2026-11-06
- **Decisores:** Jhony Chaves

## Contexto

Un proyecto de Unreal acumula binarios grandes (`.uasset`, `.umap`, texturas, audio, vídeo) que Git gestiona mal sin LFS. A la vez, el contenido generado (`Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`) no debe versionarse. El repositorio existía pero **no tenía ningún commit**, así que no había red de seguridad.

## Decisión

1. Usar **Git** con **Git LFS** (3.7.1 instalado) para binarios de Unreal y multimedia.
2. `.gitattributes` marca con LFS: `*.uasset`, `*.umap`, `*.ubulk`, `*.uexp`, `*.upk`, imágenes (`png/jpg/jpeg/tga/exr/hdr/psd`), audio (`wav/mp3/ogg`), modelos (`fbx/obj/blend/abc`), vídeo (`mp4/mov`) y **`*.pdf`** (documentación y plan maestro).
3. `.gitignore` excluye todo lo generado por el motor y del IDE.
4. Flujo de ramas: `main` estable y una rama por hito o feature (`h0-fundaciones`, `h1a-personaje`, ...), con commits pequeños y mensajes en imperativo.
5. Primer commit = checkpoint del estado auditado (código, configuración, documentación y panel web).

## Consecuencias

- Cualquiera que clone necesita `git lfs install` una vez.
- Los binarios no inflan el historial de Git normal.
- El PDF del plan maestro se versiona en LFS y queda disponible offline.

## Notas

- Añadir `*.pdf` a LFS después de haber commiteado un PDF requeriría reescribir historial; por eso se hace **antes** del primer commit.
