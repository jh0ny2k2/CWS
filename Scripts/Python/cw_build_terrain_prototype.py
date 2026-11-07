"""
Construction World - escena comparativa de terreno (F3.01, H2).

Crea /Game/Maps/TerrainPrototype con tres referencias visuales para comparar el
prototipo de terreno deformable sin arte pesado:

- CW_Ground: suelo plano de referencia (malla estatica).
- CW_TerrainFlat: CWTerrainField plano, sin zanja.
- CW_TerrainPit: CWTerrainField con una zanja inicial tallada.
- CW_Stockpile: acopio de material de referencia.
- Iluminacion, cielo, niebla y PlayerStart.

Es idempotente: si el nivel ya existe, elimina solo los actores CW_ y los
vuelve a crear.

Uso por linea de comandos (con el proyecto compilado):
    UnrealEditor-Cmd.exe ConstructionWorld.uproject -run=pythonscript \
        -script="...\\cw_build_terrain_prototype.py" -unattended -nop4 -nosplash -NullRHI
"""

import unreal

MAP_PATH = "/Game/Maps/TerrainPrototype"
LABEL_PREFIX = "CW_"


def _log(message):
    unreal.log("[ConstructionWorld] " + message)


def _error(message):
    unreal.log_error("[ConstructionWorld] " + message)


def _spawn(eas, actor_class, location, rotation):
    return eas.spawn_actor_from_class(actor_class, location, rotation)


def _apply(actor, properties):
    for prop, value in properties:
        try:
            actor.set_editor_property(prop, value)
        except Exception as exc:  # noqa: BLE001
            _log("No se pudo asignar %s a %s: %s" % (prop, actor.get_actor_label(), exc))


def _spawn_terrain(eas, label, location, carve_pit, pit_depth_cm):
    terrain_class = unreal.load_class(None, "/Script/ConstructionWorld.CWTerrainField")
    if not terrain_class:
        _log("CWTerrainField no encontrado. Compila el proyecto C++ y vuelve a ejecutar.")
        return None

    terrain = _spawn(eas, terrain_class, location,
                     unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0))
    terrain.set_actor_label(LABEL_PREFIX + label)
    _apply(terrain, [
        ("grid_cells_x", 64), ("grid_cells_y", 64),
        ("cell_size_cm", 234.375), ("chunk_cells", 8),
        ("max_chunk_rebuilds_per_tick", 4),
        ("min_height_cm", -600.0), ("max_height_cm", 400.0),
        ("generate_collision", True),
        ("carve_initial_pit", carve_pit),
        ("initial_pit_depth_cm", pit_depth_cm),
    ])
    return terrain


def build_terrain_prototype():
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

    # Suelo plano de referencia entre los dos campos (40 x 40 m).
    ground = _spawn(eas, unreal.StaticMeshActor, unreal.Vector(x=0.0, y=0.0, z=0.0),
                    unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0))
    ground.set_actor_label(LABEL_PREFIX + "Ground")
    ground.static_mesh_component.set_static_mesh(
        unreal.load_asset("/Engine/BasicShapes/Plane.Plane"))
    ground.set_actor_scale3d(unreal.Vector(x=40.0, y=40.0, z=1.0))

    # Campo plano (sin zanja) y campo con zanja inicial, separados 320 m.
    _spawn_terrain(eas, "TerrainFlat", unreal.Vector(x=-16000.0, y=0.0, z=2.0),
                   carve_pit=False, pit_depth_cm=0.0)
    _spawn_terrain(eas, "TerrainPit", unreal.Vector(x=16000.0, y=0.0, z=2.0),
                   carve_pit=True, pit_depth_cm=200.0)

    # Acopio de material de referencia.
    stockpile_class = unreal.load_class(None, "/Script/ConstructionWorld.CWStockpile")
    if stockpile_class:
        pile = _spawn(eas, stockpile_class, unreal.Vector(x=0.0, y=8000.0, z=0.0),
                      unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0))
        pile.set_actor_label(LABEL_PREFIX + "Stockpile")
    else:
        _log("CWStockpile no encontrado. Compila el proyecto C++ y vuelve a ejecutar.")

    # Punto de aparicion mirando al campo con zanja.
    player_start = _spawn(eas, unreal.PlayerStart, unreal.Vector(x=0.0, y=-3000.0, z=250.0),
                          unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0))
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

    fog = _spawn(eas, unreal.ExponentialHeightFog, unreal.Vector(x=0.0, y=0.0, z=0.0),
                 unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0))
    fog.set_actor_label(LABEL_PREFIX + "HeightFog")

    if not les.save_current_level():
        _error("El nivel se creo pero no se pudo guardar.")
        return False

    _log("Escena comparativa lista: " + MAP_PATH)
    return True


if __name__ == "__main__":
    build_terrain_prototype()
