"""
Construction World - generador del nivel de pruebas (Fase 1).

Crea /Game/Maps/TestLevel con suelo, luz solar, cielo, niebla y un PlayerStart.
Es idempotente: si el nivel ya existe, elimina solo los actores con prefijo CW_
y los vuelve a crear.

Uso dentro del editor:
    Tools > Execute Python Script...  -> seleccionar este archivo

Uso por linea de comandos (con el proyecto ya compilado):
    UnrealEditor-Cmd.exe ConstructionWorld.uproject -run=pythonscript -script="...\\cw_build_test_level.py"

Requiere los plugins PythonScriptPlugin y EditorScriptingUtilities (ya activados
en ConstructionWorld.uproject).
"""

import unreal

MAP_PATH = "/Game/Maps/TestLevel"
LABEL_PREFIX = "CW_"


def _log(message):
    unreal.log("[ConstructionWorld] " + message)


def _error(message):
    unreal.log_error("[ConstructionWorld] " + message)


def _spawn(eas, actor_class, location, rotation):
    return eas.spawn_actor_from_class(actor_class, location, rotation)


def build_test_level():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if les is None or eas is None:
        _error("No se pudieron obtener los subsistemas del editor.")
        return False

    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        _log("El nivel ya existe; se reutiliza y se limpian los actores " + LABEL_PREFIX + "*")
        if not les.load_level(MAP_PATH):
            _error("No se pudo abrir " + MAP_PATH)
            return False
        for actor in eas.get_all_level_actors():
            if actor.get_actor_label().startswith(LABEL_PREFIX):
                eas.destroy_actor(actor)
    else:
        _log("Creando nivel " + MAP_PATH)
        if not les.new_level(MAP_PATH):
            _error("No se pudo crear el nivel " + MAP_PATH)
            return False

    # Suelo 200 x 200 m
    ground = _spawn(eas, unreal.StaticMeshActor, unreal.Vector(x=0.0, y=0.0, z=0.0),
                    unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0))
    ground.set_actor_label(LABEL_PREFIX + "Ground")
    ground_comp = ground.static_mesh_component
    ground_comp.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Plane.Plane"))
    ground.set_actor_scale3d(unreal.Vector(x=200.0, y=200.0, z=1.0))

    # Bloques de referencia para comprobar escala, sombras y colision
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    blocks = [
        ("Block_A", unreal.Vector(x=800.0, y=0.0, z=100.0), unreal.Vector(x=2.0, y=2.0, z=2.0)),
        ("Block_B", unreal.Vector(x=-600.0, y=900.0, z=150.0), unreal.Vector(x=3.0, y=1.5, z=3.0)),
        ("Block_C", unreal.Vector(x=-1200.0, y=-500.0, z=50.0), unreal.Vector(x=1.0, y=4.0, z=1.0)),
    ]
    for name, location, scale in blocks:
        block = _spawn(eas, unreal.StaticMeshActor, location,
                       unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0))
        block.set_actor_label(LABEL_PREFIX + name)
        block.static_mesh_component.set_static_mesh(cube)
        block.set_actor_scale3d(scale)

    # Vehiculo de prueba (requiere que el modulo C++ este compilado)
    vehicle_class = unreal.load_class(None, "/Script/ConstructionWorld.CWVehicleBase")
    if vehicle_class:
        vehicle = _spawn(eas, vehicle_class, unreal.Vector(x=1500.0, y=-800.0, z=100.0),
                         unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0))
        vehicle.set_actor_label(LABEL_PREFIX + "TestVehicle")
    else:
        _log("CWVehicleBase no encontrado. Compila el proyecto C++ y vuelve a ejecutar este script.")

    # Punto de aparicion
    player_start = _spawn(eas, unreal.PlayerStart, unreal.Vector(x=0.0, y=-600.0, z=120.0),
                          unreal.Rotator(pitch=0.0, yaw=90.0, roll=0.0))
    player_start.set_actor_label(LABEL_PREFIX + "PlayerStart")

    # Sol
    sun = _spawn(eas, unreal.DirectionalLight, unreal.Vector(x=0.0, y=0.0, z=2000.0),
                 unreal.Rotator(pitch=-40.0, yaw=35.0, roll=0.0))
    sun.set_actor_label(LABEL_PREFIX + "Sun")
    sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
    if sun_comp:
        sun_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
        sun_comp.set_editor_property("intensity", 8.0)

    # Atmosfera y cielo
    atmosphere = _spawn(eas, unreal.SkyAtmosphere, unreal.Vector(x=0.0, y=0.0, z=0.0),
                        unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0))
    atmosphere.set_actor_label(LABEL_PREFIX + "SkyAtmosphere")

    sky_light = _spawn(eas, unreal.SkyLight, unreal.Vector(x=0.0, y=0.0, z=1000.0),
                       unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0))
    sky_light.set_actor_label(LABEL_PREFIX + "SkyLight")
    sky_comp = sky_light.get_component_by_class(unreal.SkyLightComponent)
    if sky_comp:
        sky_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
        sky_comp.set_editor_property("real_time_capture", True)

    # Niebla atmosferica suave
    fog = _spawn(eas, unreal.ExponentialHeightFog, unreal.Vector(x=0.0, y=0.0, z=0.0),
                 unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0))
    fog.set_actor_label(LABEL_PREFIX + "HeightFog")

    if not les.save_current_level():
        _error("El nivel se creo pero no se pudo guardar.")
        return False

    _log("Nivel listo: " + MAP_PATH)
    return True


if __name__ == "__main__":
    build_test_level()

