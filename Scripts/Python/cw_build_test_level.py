"""
Construction World - generador del nivel de pruebas.

Crea /Game/Maps/TestLevel con:
- Suelo, bloques de referencia, vehiculos, zona de descarga, terreno deformable y PlayerStart.
- Iluminacion de tarde dorada, cielo atmosferico, niebla volumetrica y postproceso (H4 / F5.02).
- Decorado de obra: conos, barreras, contenedores, palets, tuberias, monticulos y colinas lejanas (F5.02 / F5.03).

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
MATERIAL_DIR = "/Game/ConstructionWorld/Materials"

CUBE = "/Engine/BasicShapes/Cube.Cube"
CYLINDER = "/Engine/BasicShapes/Cylinder.Cylinder"
CONE = "/Engine/BasicShapes/Cone.Cone"
SPHERE = "/Engine/BasicShapes/Sphere.Sphere"
PLANE = "/Engine/BasicShapes/Plane.Plane"

_ROT_ZERO = unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0)


def _log(message):
    unreal.log("[ConstructionWorld] " + message)


def _error(message):
    unreal.log_error("[ConstructionWorld] " + message)


def _spawn(eas, actor_class, location, rotation):
    return eas.spawn_actor_from_class(actor_class, location, rotation)


def _material(name):
    path = MATERIAL_DIR + "/" + name + "." + name
    mat = unreal.load_asset(path)
    if not mat:
        _log("Material no encontrado: " + path)
    return mat


def _spawn_prop(eas, name, mesh_path, location, scale, material_name,
                rotation=None):
    """Crea un StaticMeshActor de decorado con malla y material PBR."""
    actor = _spawn(eas, unreal.StaticMeshActor, location, rotation or _ROT_ZERO)
    actor.set_actor_label(LABEL_PREFIX + name)
    mesh = unreal.load_asset(mesh_path)
    comp = actor.static_mesh_component
    if mesh:
        comp.set_static_mesh(mesh)
    actor.set_actor_scale3d(scale)

    mat = _material(material_name)
    if mat:
        comp.set_material(0, mat)
    return actor


def _setup_lighting(eas):
    """Tarde dorada, cielo atmosferico, niebla volumetrica y postproceso."""
    from unreal import (ComponentMobility, DirectionalLight, DirectionalLightComponent,
                        ExponentialHeightFog, SkyAtmosphere, SkyLight, SkyLightComponent,
                        Vector4)

    # --- Sol bajo de tarde dorada -------------------------------------------------
    sun = _spawn(eas, DirectionalLight, unreal.Vector(x=0.0, y=0.0, z=3000.0),
                 unreal.Rotator(pitch=-16.0, yaw=42.0, roll=0.0))
    sun.set_actor_label(LABEL_PREFIX + "Sun")
    sun_comp = sun.get_component_by_class(DirectionalLightComponent)
    if sun_comp:
        sun_comp.set_editor_property("mobility", ComponentMobility.MOVABLE)
        sun_comp.set_editor_property("intensity", 9.0)
        sun_comp.set_editor_property("use_temperature", True)
        sun_comp.set_editor_property("temperature", 3800.0)
        sun_comp.set_editor_property("atmosphere_sun_light", True)
        sun_comp.set_editor_property("atmosphere_sun_disk_color_scale",
                                     unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
        sun_comp.set_editor_property("cast_shadows", True)
        sun_comp.set_editor_property("volumetric_scattering_intensity", 1.2)
    else:
        _log("No se pudo configurar el componente del sol.")

    # --- Atmosfera -----------------------------------------------------------------
    atmosphere = _spawn(eas, SkyAtmosphere, unreal.Vector(x=0.0, y=0.0, z=0.0), _ROT_ZERO)
    atmosphere.set_actor_label(LABEL_PREFIX + "SkyAtmosphere")

    # --- Luz de cielo con captura en tiempo real (reflejos e indirecta) -------------
    sky_light = _spawn(eas, SkyLight, unreal.Vector(x=0.0, y=0.0, z=1000.0), _ROT_ZERO)
    sky_light.set_actor_label(LABEL_PREFIX + "SkyLight")
    sky_comp = sky_light.get_component_by_class(SkyLightComponent)
    if sky_comp:
        sky_comp.set_editor_property("mobility", ComponentMobility.MOVABLE)
        sky_comp.set_editor_property("real_time_capture", True)
        sky_comp.set_editor_property("intensity", 1.15)

    # --- Niebla volumetrica calida --------------------------------------------------
    fog = _spawn(eas, ExponentialHeightFog, unreal.Vector(x=0.0, y=0.0, z=0.0), _ROT_ZERO)
    fog.set_actor_label(LABEL_PREFIX + "HeightFog")
    fog_comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    if fog_comp:
        fog_comp.set_editor_property("fog_density", 0.012)
        fog_comp.set_editor_property("fog_height_falloff", 0.22)
        fog_comp.set_editor_property("fog_max_opacity", 0.92)
        fog_comp.set_editor_property("fog_inscattering_luminance",
                                     unreal.LinearColor(0.56, 0.64, 0.78, 1.0))
        fog_comp.set_editor_property("directional_inscattering_luminance",
                                     unreal.LinearColor(1.0, 0.72, 0.44, 1.0))
        fog_comp.set_editor_property("directional_inscattering_exponent", 2.2)
        fog_comp.set_editor_property("enable_volumetric_fog", True)
        fog_comp.set_editor_property("volumetric_fog_scattering_distribution", 0.65)
        fog_comp.set_editor_property("volumetric_fog_extinction_scale", 1.15)
        fog_comp.set_editor_property("volumetric_fog_distance", 16000.0)
        fog_comp.set_editor_property("volumetric_fog_start_distance", 400.0)
    else:
        _log("No se pudo configurar la niebla.")

    # --- Postproceso global (sin volumen delimitador) -------------------------------
    volume = _spawn(eas, unreal.PostProcessVolume, unreal.Vector(x=0.0, y=0.0, z=0.0), _ROT_ZERO)
    volume.set_actor_label(LABEL_PREFIX + "GlobalPostProcess")
    volume.set_editor_property("unbound", True)
    volume.set_editor_property("priority", 1.0)

    settings = unreal.PostProcessSettings()
    overrides = {
        # Exposicion automatica acotada: evita lavados al mirar al sol.
        "override_auto_exposure_method": True,
        "auto_exposure_method": unreal.AutoExposureMethod.AEM_HISTOGRAM,
        "override_auto_exposure_min_brightness": True,
        "auto_exposure_min_brightness": 0.45,
        "override_auto_exposure_max_brightness": True,
        "auto_exposure_max_brightness": 2.2,
        "override_auto_exposure_bias": True,
        "auto_exposure_bias": -0.15,
        "override_auto_exposure_speed_up": True,
        "auto_exposure_speed_up": 2.4,
        "override_auto_exposure_speed_down": True,
        "auto_exposure_speed_down": 1.2,
        # Bloom contenido, típico de tarde luminosa.
        "override_bloom_intensity": True,
        "bloom_intensity": 0.55,
        # Grano y vineta suaves: sensacion de camara.
        "override_film_grain_intensity": True,
        "film_grain_intensity": 0.06,
        "override_vignette_intensity": True,
        "vignette_intensity": 0.30,
        # Oclusion ambiental para asentar la maquinaria en el suelo.
        "override_ambient_occlusion_intensity": True,
        "ambient_occlusion_intensity": 0.55,
        "override_ambient_occlusion_radius": True,
        "ambient_occlusion_radius": 120.0,
        # Color grading calido de atardecer.
        "override_color_saturation": True,
        "color_saturation": Vector4(1.06, 1.06, 1.06, 1.0),
        "override_color_contrast": True,
        "color_contrast": Vector4(1.05, 1.05, 1.05, 1.0),
        "override_color_gain": True,
        "color_gain": Vector4(1.03, 1.00, 0.96, 1.0),
        "override_color_gamma": True,
        "color_gamma": Vector4(1.0, 1.0, 1.0, 1.0),
    }

    for prop, value in overrides.items():
        try:
            settings.set_editor_property(prop, value)
        except Exception as exc:  # noqa: BLE001
            _log("Postproceso: no se pudo asignar %s: %s" % (prop, exc))

    volume.set_editor_property("settings", settings)


def _dress_construction_site(eas):
    """Conos, barreras, contenedores, palets, tuberias, monticulos y colinas."""
    # Conos de obra en fila junto al acceso.
    cone_mat = "MI_CW_ConeOrange"
    for index in range(7):
        _spawn_prop(eas, "Cone_%02d" % index, CONE,
                    unreal.Vector(x=1600.0 + index * 220.0, y=-1500.0, z=45.0),
                    unreal.Vector(x=0.32, y=0.32, z=0.9), cone_mat)

    # Barreras de hormigon haciendo un pasillo hacia la zona de descarga.
    for index in range(5):
        _spawn_prop(eas, "Barrier_%02d" % index, CUBE,
                    unreal.Vector(x=900.0 + index * 430.0, y=1400.0, z=45.0),
                    unreal.Vector(x=1.7, y=0.32, z=0.9), "MI_CW_Concrete")

    # Contenedor de obra como caseta y su tejado metalico.
    _spawn_prop(eas, "SiteContainer", CUBE, unreal.Vector(x=-1800.0, y=1400.0, z=130.0),
                unreal.Vector(x=6.0, y=2.4, z=2.6), "MI_CW_ContainerBlue")
    _spawn_prop(eas, "SiteContainerRoof", CUBE, unreal.Vector(x=-1800.0, y=1400.0, z=268.0),
                unreal.Vector(x=6.3, y=2.6, z=0.12), "MI_CW_HeavySteel")
    _spawn_prop(eas, "SiteContainerDoor", CUBE, unreal.Vector(x=-1495.0, y=1400.0, z=120.0),
                unreal.Vector(x=0.05, y=0.9, z=1.9), "MI_CW_Wood")

    # Palets de madera apilados junto al contenedor.
    for index in range(3):
        _spawn_prop(eas, "Pallet_%02d" % index, CUBE,
                    unreal.Vector(x=-1150.0, y=1750.0, z=10.0 + index * 20.0),
                    unreal.Vector(x=1.6, y=1.2, z=0.14), "MI_CW_Wood")

    # Tuberias de acero en el acopio.
    for index in range(4):
        _spawn_prop(eas, "Pipe_%02d" % index, CYLINDER,
                    unreal.Vector(x=-300.0, y=2100.0, z=45.0 + index * 85.0),
                    unreal.Vector(x=0.5, y=0.5, z=4.2), "MI_CW_HeavySteel",
                    unreal.Rotator(pitch=0.0, yaw=90.0, roll=90.0))

    # Monticulos de tierra/grava del movimiento de tierras.
    piles = [
        ("DirtPile_A", unreal.Vector(x=700.0, y=700.0, z=-30.0), "MI_CW_Earth",
         unreal.Vector(x=3.2, y=2.6, z=1.1)),
        ("DirtPile_B", unreal.Vector(x=1200.0, y=1500.0, z=-35.0), "MI_CW_Earth",
         unreal.Vector(x=2.4, y=2.0, z=0.9)),
        ("GravelPile", unreal.Vector(x=-700.0, y=-1300.0, z=-30.0), "MI_CW_Gravel",
         unreal.Vector(x=2.8, y=2.2, z=0.95)),
    ]
    for name, location, material, scale in piles:
        _spawn_prop(eas, name, SPHERE, location, scale, material)

    # Colinas lejanas: masas de tierra/greda que cierran el horizonte.
    hills = [
        ("Hill_NW", unreal.Vector(x=-7000.0, y=-6000.0, z=-260.0), 38.0, 30.0, 7.0),
        ("Hill_N", unreal.Vector(x=-500.0, y=-7800.0, z=-300.0), 52.0, 24.0, 9.0),
        ("Hill_NE", unreal.Vector(x=6500.0, y=-6500.0, z=-280.0), 34.0, 28.0, 8.0),
        ("Hill_E", unreal.Vector(x=8200.0, y=800.0, z=-320.0), 46.0, 26.0, 10.0),
        ("Hill_SE", unreal.Vector(x=6000.0, y=7000.0, z=-300.0), 40.0, 32.0, 8.5),
        ("Hill_S", unreal.Vector(x=0.0, y=8400.0, z=-340.0), 60.0, 22.0, 11.0),
    ]
    for name, location, sx, sy, sz in hills:
        _spawn_prop(eas, name, SPHERE, location,
                    unreal.Vector(x=sx, y=sy, z=sz), "MI_CW_Gravel")


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

    # Suelo 200 x 200 m con material de tierra PBR.
    ground = _spawn(eas, unreal.StaticMeshActor, unreal.Vector(x=0.0, y=0.0, z=0.0), _ROT_ZERO)
    ground.set_actor_label(LABEL_PREFIX + "Ground")
    ground_comp = ground.static_mesh_component
    ground_comp.set_static_mesh(unreal.load_asset(PLANE))
    ground.set_actor_scale3d(unreal.Vector(x=200.0, y=200.0, z=1.0))
    earth_mat = _material("MI_CW_Earth")
    if earth_mat:
        ground_comp.set_material(0, earth_mat)

    # Bloques de referencia para comprobar escala, sombras y colision.
    cube = unreal.load_asset(CUBE)
    blocks = [
        ("Block_A", unreal.Vector(x=800.0, y=0.0, z=100.0), unreal.Vector(x=2.0, y=2.0, z=2.0), "MI_CW_Concrete"),
        ("Block_B", unreal.Vector(x=-600.0, y=900.0, z=150.0), unreal.Vector(x=3.0, y=1.5, z=3.0), "MI_CW_Wood"),
        ("Block_C", unreal.Vector(x=-1200.0, y=-500.0, z=50.0), unreal.Vector(x=1.0, y=4.0, z=1.0), "MI_CW_Concrete"),
    ]
    for name, location, scale, material_name in blocks:
        block = _spawn(eas, unreal.StaticMeshActor, location, _ROT_ZERO)
        block.set_actor_label(LABEL_PREFIX + name)
        block.static_mesh_component.set_static_mesh(cube)
        block.set_actor_scale3d(scale)
        mat = _material(material_name)
        if mat:
            block.static_mesh_component.set_material(0, mat)

    # Vehiculo de prueba (requiere que el modulo C++ este compilado)
    vehicle_class = unreal.load_class(None, "/Script/ConstructionWorld.CWVehicleBase")
    if vehicle_class:
        vehicle = _spawn(eas, vehicle_class, unreal.Vector(x=1500.0, y=-800.0, z=100.0), _ROT_ZERO)
        vehicle.set_actor_label(LABEL_PREFIX + "TestVehicle")
    else:
        _log("CWVehicleBase no encontrado. Compila el proyecto C++ y vuelve a ejecutar este script.")

    # Excavadora de cadenas (H1-B)
    excavator_class = unreal.load_class(None, "/Script/ConstructionWorld.CWExcavator")
    if excavator_class:
        excavator = _spawn(eas, excavator_class, unreal.Vector(x=2400.0, y=-900.0, z=75.0),
                           unreal.Rotator(pitch=0.0, yaw=180.0, roll=0.0))
        excavator.set_actor_label(LABEL_PREFIX + "TestExcavator")
    else:
        _log("CWExcavator no encontrado. Compila el proyecto C++ y vuelve a ejecutar este script.")

    # Camion volquete (H1-B)
    truck_class = unreal.load_class(None, "/Script/ConstructionWorld.CWDumpTruck")
    if truck_class:
        truck = _spawn(eas, truck_class, unreal.Vector(x=2400.0, y=300.0, z=95.0), _ROT_ZERO)
        truck.set_actor_label(LABEL_PREFIX + "TestDumpTruck")
    else:
        _log("CWDumpTruck no encontrado. Compila el proyecto C++ y vuelve a ejecutar este script.")

    # Zona de descarga (H1-B)
    zone_class = unreal.load_class(None, "/Script/ConstructionWorld.CWDumpZone")
    if zone_class:
        zone = _spawn(eas, zone_class, unreal.Vector(x=0.0, y=1800.0, z=250.0), _ROT_ZERO)
        zone.set_actor_label(LABEL_PREFIX + "DumpZone")
    else:
        _log("CWDumpZone no encontrado. Compila el proyecto C++ y vuelve a ejecutar este script.")

    # Campo de terreno deformable (H2). Se eleva 2 cm sobre el suelo de referencia
    # para evitar z-fighting; el terreno real se genera en BeginPlay.
    terrain_class = unreal.load_class(None, "/Script/ConstructionWorld.CWTerrainField")
    if terrain_class:
        terrain = _spawn(eas, terrain_class, unreal.Vector(x=0.0, y=0.0, z=2.0), _ROT_ZERO)
        terrain.set_actor_label(LABEL_PREFIX + "TerrainField")
        for prop, value in [
            ("grid_cells_x", 64), ("grid_cells_y", 64),
            ("cell_size_cm", 234.375), ("chunk_cells", 8),
            ("max_chunk_rebuilds_per_tick", 4), ("min_height_cm", -600.0),
            ("max_height_cm", 400.0),
        ]:
            try:
                terrain.set_editor_property(prop, value)
            except Exception as exc:  # noqa: BLE001
                _log("No se pudo asignar %s: %s" % (prop, exc))
    else:
        _log("CWTerrainField no encontrado. Compila el proyecto C++ y vuelve a ejecutar este script.")

    # Punto de aparicion
    player_start = _spawn(eas, unreal.PlayerStart, unreal.Vector(x=0.0, y=-600.0, z=120.0),
                          unreal.Rotator(pitch=0.0, yaw=90.0, roll=0.0))
    player_start.set_actor_label(LABEL_PREFIX + "PlayerStart")

    # H4: luz de tarde, atmosfera, niebla y postproceso.
    _setup_lighting(eas)

    # H4: decorado de obra y horizonte.
    _dress_construction_site(eas)

    if not les.save_current_level():
        _error("El nivel se creo pero no se pudo guardar.")
        return False

    _log("Nivel listo: " + MAP_PATH)
    return True


if __name__ == "__main__":
    build_test_level()
