# Contenido del proyecto

Carpetas de assets de Construction World. La nomenclatura y el flujo están en `docs/CONVENCIONES.md` y `docs/ASSETS.md`.

```
Content/ConstructionWorld/
  Vehicles/      Excavadoras, camiones y maquinaria
  Characters/    Operarios, trabajadores y NPCs
  Materials/     Materiales PBR (tierra, grava, asfalto, hormigón)
  Environment/   Props de obra, vegetación, rocas y cielo
  Audio/         Motor, hidráulica, impactos y ambiente
  Data/          Data Assets / Data Tables (máquinas, materiales, contratos)
  UI/            Iconos, widgets y fuentes
```

Reglas rápidas:

- Nombres con prefijo `BP_CW_`, `SM_CW_`, `M_CW_`, `T_CW_`, `DA_CW_`, `L_CW_`.
- Todo asset binario va por Git LFS.
- Todo asset debe estar en `docs/assets_manifest.json` con fuente y licencia.
- Los fuentes editables (`.blend`, `.psd`) van en `ArtSource/`, no aquí.
