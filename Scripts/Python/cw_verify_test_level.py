"""
Construction World - verificacion del nivel de pruebas.

Abre /Game/Maps/TestLevel y vuelca en el log los actores con prefijo CW_
(etiqueta, clase y posicion). No modifica el nivel.

Uso:
    UnrealEditor-Cmd.exe ConstructionWorld.uproject -run=pythonscript \
        -script="...\\cw_verify_test_level.py" -unattended -nop4 -nosplash -NullRHI
"""

import unreal

MAP_PATH = "/Game/Maps/TestLevel"
LABEL_PREFIX = "CW_"


def _log(message):
    unreal.log("[ConstructionWorld] " + message)


def verify():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if les is None or eas is None:
        unreal.log_error("[ConstructionWorld] Sin subsistemas del editor.")
        return False

    if not unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        unreal.log_error("[ConstructionWorld] No existe " + MAP_PATH)
        return False

    if not les.load_level(MAP_PATH):
        unreal.log_error("[ConstructionWorld] No se pudo abrir " + MAP_PATH)
        return False

    found = []
    for actor in eas.get_all_level_actors():
        label = actor.get_actor_label()
        if label.startswith(LABEL_PREFIX):
            loc = actor.get_actor_location()
            found.append(label + " | " + actor.get_class().get_name() +
                         " | X=%.0f Y=%.0f Z=%.0f" % (loc.x, loc.y, loc.z))

    found.sort()
    _log("Actores CW_ encontrados: %d" % len(found))
    for line in found:
        _log("  " + line)

    expected = [
        "CW_Ground", "CW_Block_A", "CW_Block_B", "CW_Block_C",
        "CW_TestVehicle", "CW_TestExcavator", "CW_TestDumpTruck", "CW_DumpZone",
        "CW_PlayerStart", "CW_Sun",
        "CW_SkyAtmosphere", "CW_SkyLight", "CW_HeightFog",
    ]
    labels = [line.split(" | ")[0] for line in found]
    missing = [name for name in expected if name not in labels]
    if missing:
        _log("FALTAN: " + ", ".join(missing))
        return False

    _log("Nivel verificado: los %d actores esperados estan presentes." % len(expected))
    return True


if __name__ == "__main__":
    verify()
