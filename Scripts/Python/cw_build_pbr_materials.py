"""
Construction World Simulator - Generador de materiales PBR de alta fidelidad (H4).

Crea:
- /Game/ConstructionWorld/Materials/M_CW_MasterPBR: material maestro con BaseColor,
  DirtColor, DirtAmount, Metallic y Roughness. La suciedad se mezcla con una funcion
  de ruido para que la pintura y el acero no se vean planos.
- Instancias PBR para maquinaria, terreno y decorado.
- /Game/ConstructionWorld/Materials/M_CW_TranslucentFX: material unlit translucido
  usado por el polvo, las huellas de orugas y la lluvia (parametros Color y Opacity).

Uso dentro del editor:
    Tools > Execute Python Script... -> seleccionar este archivo

Uso por linea de comandos (con el proyecto compilado):
    UnrealEditor-Cmd.exe ConstructionWorld.uproject -run=pythonscript -script="...\\cw_build_pbr_materials.py"
"""

import unreal

MATERIAL_DIR = "/Game/ConstructionWorld/Materials"
MASTER_NAME = "M_CW_MasterPBR"
FX_NAME = "M_CW_TranslucentFX"


def _log(msg):
    unreal.log("[CW_PBR] " + msg)


def _try_set(obj, prop, value):
    try:
        obj.set_editor_property(prop, value)
        return True
    except Exception as exc:  # noqa: BLE001
        _log("No se pudo asignar %s: %s" % (prop, exc))
        return False


def _connect(mel, source, source_output, target, target_input):
    try:
        mel.connect_material_expressions(source, source_output, target, target_input)
        return True
    except Exception as exc:  # noqa: BLE001
        _log("No se pudo conectar %s.%s -> %s: %s" % (
            source.get_name(), source_output, target_input, exc))
        return False


def _connect_property(mel, source, source_output, material, prop):
    try:
        mel.connect_material_property(source, source_output, prop)
        return True
    except Exception as exc:  # noqa: BLE001
        _log("No se pudo conectar propiedad %s: %s" % (prop, exc))
        return False


def _get_or_create_material(name, package_dir):
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    path = package_dir + "/" + name
    mat = unreal.load_asset(path)
    if mat:
        _log("Material existente: " + path)
        return mat

    _log("Creando material: " + path)
    factory = unreal.MaterialFactoryNew()
    return asset_tools.create_asset(name, package_dir, unreal.Material, factory)


def create_master_material():
    mat = _get_or_create_material(MASTER_NAME, MATERIAL_DIR)
    if not mat:
        unreal.log_error("[CW_PBR] Fallo al crear el material maestro.")
        return None

    mel = unreal.MaterialEditingLibrary

    # Nota: DeleteAllMaterialExpressions provoca un assert (IsRooted) en headless
    # con UE 5.8, asi que solo construimos materiales nuevos. Si el material ya
    # existe se conserva tal cual (idempotente).
    existing_expressions = None
    try:
        existing_expressions = mat.get_editor_property("expressions")
    except Exception:  # noqa: BLE001
        existing_expressions = None
    if existing_expressions:
        _log("El material maestro ya tiene expresiones; se reutiliza sin recrear.")
        return mat

    # Parametros base.
    base_color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -900, -250)
    _try_set(base_color, "parameter_name", "BaseColor")
    _try_set(base_color, "default_value", unreal.LinearColor(0.8, 0.8, 0.8, 1.0))

    dirt_color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -900, -100)
    _try_set(dirt_color, "parameter_name", "DirtColor")
    _try_set(dirt_color, "default_value", unreal.LinearColor(0.12, 0.09, 0.06, 1.0))

    dirt_amount = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -900, 50)
    _try_set(dirt_amount, "parameter_name", "DirtAmount")
    _try_set(dirt_amount, "default_value", 0.25)

    metallic = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -900, 180)
    _try_set(metallic, "parameter_name", "Metallic")
    _try_set(metallic, "default_value", 0.0)

    roughness = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -900, 300)
    _try_set(roughness, "parameter_name", "Roughness")
    _try_set(roughness, "default_value", 0.5)

    # Ruido de suciedad: Lerp(BaseColor, DirtColor, clamp(noise * DirtAmount)).
    noise = mel.create_material_expression(mat, unreal.MaterialExpressionNoise, -900, 450)
    _try_set(noise, "scale", 18.0)
    _try_set(noise, "quality", 1)
    _try_set(noise, "levels", 3)

    noise_amount = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -700, 420)
    _connect(mel, noise, "", noise_amount, "A")
    _connect(mel, dirt_amount, "", noise_amount, "B")

    lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -500, -100)
    _connect(mel, base_color, "", lerp, "A")
    _connect(mel, dirt_color, "", lerp, "B")
    _connect(mel, noise_amount, "", lerp, "Alpha")

    _connect_property(mel, lerp, "", mat, unreal.MaterialProperty.MP_BASE_COLOR)
    _connect_property(mel, metallic, "", mat, unreal.MaterialProperty.MP_METALLIC)
    _connect_property(mel, roughness, "", mat, unreal.MaterialProperty.MP_ROUGHNESS)

    mel.layout_material_expressions(mat)
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    _log("Material maestro %s configurado y guardado." % MASTER_NAME)
    return mat


def create_material_instance(master_mat, name, color, metallic, roughness, dirt_amount=0.25):
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    inst_path = MATERIAL_DIR + "/" + name

    inst = unreal.load_asset(inst_path)
    if not inst:
        _log("Creando instancia: " + name)
        factory = unreal.MaterialInstanceConstantFactoryNew()
        inst = asset_tools.create_asset(name, MATERIAL_DIR, unreal.MaterialInstanceConstant, factory)

    mel = unreal.MaterialEditingLibrary
    mel.set_material_instance_parent(inst, master_mat)
    mel.set_material_instance_vector_parameter_value(inst, "BaseColor", color)
    mel.set_material_instance_scalar_parameter_value(inst, "Metallic", metallic)
    mel.set_material_instance_scalar_parameter_value(inst, "Roughness", roughness)
    mel.set_material_instance_scalar_parameter_value(inst, "DirtAmount", dirt_amount)
    mel.update_material_instance(inst)
    unreal.EditorAssetLibrary.save_loaded_asset(inst)
    _log("Instancia " + name + " guardada.")
    return inst


def create_translucent_fx_material():
    mat = _get_or_create_material(FX_NAME, MATERIAL_DIR)
    if not mat:
        unreal.log_error("[CW_PBR] Fallo al crear el material translucido.")
        return None

    _try_set(mat, "blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    _try_set(mat, "shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    _try_set(mat, "two_sided", True)
    _try_set(mat, "cast_dynamic_shadows", False)

    mel = unreal.MaterialEditingLibrary

    existing_expressions = None
    try:
        existing_expressions = mat.get_editor_property("expressions")
    except Exception:  # noqa: BLE001
        existing_expressions = None
    if existing_expressions:
        _log("El material translucido ya tiene expresiones; se reutiliza sin recrear.")
        return mat

    color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, -100)
    _try_set(color, "parameter_name", "Color")
    _try_set(color, "default_value", unreal.LinearColor(0.55, 0.42, 0.28, 1.0))

    opacity = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, 120)
    _try_set(opacity, "parameter_name", "Opacity")
    _try_set(opacity, "default_value", 0.45)

    _connect_property(mel, color, "", mat, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    _connect_property(mel, opacity, "", mat, unreal.MaterialProperty.MP_OPACITY)

    mel.layout_material_expressions(mat)
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    _log("Material translucido %s configurado y guardado." % FX_NAME)
    return mat


def build_all_materials():
    master = create_master_material()
    if not master:
        return False

    variants = [
        # Amarillo maquinaria de construccion (CAT / Komatsu / JCB)
        ("MI_CW_ConstructionYellow", unreal.LinearColor(0.96, 0.65, 0.04, 1.0), 0.05, 0.30, 0.22),
        # Acero estructural oscuro
        ("MI_CW_HeavySteel", unreal.LinearColor(0.12, 0.12, 0.13, 1.0), 0.85, 0.50, 0.30),
        # Goma negra mate para orugas y neumaticos
        ("MI_CW_Rubber", unreal.LinearColor(0.04, 0.04, 0.04, 1.0), 0.0, 0.85, 0.18),
        # Cromo brillante reflectante para cilindros hidraulicos
        ("MI_CW_Chrome", unreal.LinearColor(0.95, 0.95, 0.95, 1.0), 0.98, 0.08, 0.10),
        # Cristal tintado para cabinas
        ("MI_CW_Glass", unreal.LinearColor(0.10, 0.15, 0.20, 1.0), 0.1, 0.12, 0.05),
        # Suelo de tierra
        ("MI_CW_Earth", unreal.LinearColor(0.32, 0.22, 0.13, 1.0), 0.0, 0.92, 0.55),
        # Suelo de grava
        ("MI_CW_Gravel", unreal.LinearColor(0.42, 0.40, 0.38, 1.0), 0.0, 0.85, 0.45),
        # Hormigon de barreras y bloques
        ("MI_CW_Concrete", unreal.LinearColor(0.58, 0.57, 0.54, 1.0), 0.0, 0.82, 0.35),
        # Madera de palets
        ("MI_CW_Wood", unreal.LinearColor(0.42, 0.26, 0.12, 1.0), 0.0, 0.78, 0.22),
        # Naranja de conos de obra
        ("MI_CW_ConeOrange", unreal.LinearColor(0.95, 0.28, 0.02, 1.0), 0.0, 0.45, 0.20),
        # Azul de contenedor de obra
        ("MI_CW_ContainerBlue", unreal.LinearColor(0.05, 0.20, 0.38, 1.0), 0.25, 0.55, 0.28),
    ]

    for name, col, met, rough, dirt in variants:
        create_material_instance(master, name, col, met, rough, dirt)

    create_translucent_fx_material()

    _log("Todos los materiales de H4 generados correctamente.")
    return True


if __name__ == "__main__":
    build_all_materials()
