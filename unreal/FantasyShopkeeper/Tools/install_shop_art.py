"""Import the Blender shop kit and fit it into the saved shop greybox.

Run in UnrealEditor-Cmd with PythonScriptPlugin and EditorScriptingUtilities.
Save and close the interactive editor before running this installer.
"""

import json
from pathlib import Path
import shutil

import unreal

unreal.load_module("StaticMeshEditor")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(world, "Interchange.FeatureFlags.Import.FBX 0")

PROJECT = Path(unreal.Paths.project_dir()).resolve()
SOURCE = PROJECT.parents[1] / "art/shop-corner"
MANIFEST = json.loads((SOURCE / "manifest.json").read_text())
DESTINATION = "/Game/Shop/Art"
ASSETS = unreal.AssetToolsHelpers.get_asset_tools()
LIBRARY = unreal.EditorAssetLibrary
MATERIALS = unreal.MaterialEditingLibrary
MESHES = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
ACTORS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LEVEL = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def constant(material, value, property_name):
    node = MATERIALS.create_material_expression(material, unreal.MaterialExpressionConstant)
    node.set_editor_property("r", value)
    MATERIALS.connect_material_property(node, "", property_name)


def color_node(material, rgb):
    node = MATERIALS.create_material_expression(material, unreal.MaterialExpressionConstant3Vector)
    node.set_editor_property("constant", unreal.LinearColor(*rgb, 1))
    return node


def create_material(name, settings):
    path = f"{DESTINATION}/Materials/M_{name}"
    material = LIBRARY.load_asset(path) if LIBRARY.does_asset_exist(path) else None
    if not material:
        material = ASSETS.create_asset(f"M_{name}", f"{DESTINATION}/Materials", unreal.Material, unreal.MaterialFactoryNew())
    MATERIALS.delete_all_material_expressions(material)
    rgba, roughness, metallic = settings
    base = color_node(material, rgba[:3])
    if name in ("HoneyWood", "DarkWood", "Cork"):
        # Fine anisotropic grain in world centimetres, shared across the kit.
        position = MATERIALS.create_material_expression(material, unreal.MaterialExpressionWorldPosition)
        stretch = MATERIALS.create_material_expression(material, unreal.MaterialExpressionMultiply)
        grain_scale = color_node(material, (0.005, 0.1, 0.1))
        MATERIALS.connect_material_expressions(position, "", stretch, "A")
        MATERIALS.connect_material_expressions(grain_scale, "", stretch, "B")
        noise = MATERIALS.create_material_expression(material, unreal.MaterialExpressionNoise)
        noise.set_editor_property("scale", 1.0)
        noise.set_editor_property("levels", 2)
        noise.set_editor_property("quality", 1)
        MATERIALS.connect_material_expressions(stretch, "", noise, "Position")
        tint = MATERIALS.create_material_expression(material, unreal.MaterialExpressionLinearInterpolate)
        dark = color_node(material, [v * 0.72 for v in rgba[:3]])
        MATERIALS.connect_material_expressions(dark, "", tint, "A")
        MATERIALS.connect_material_expressions(base, "", tint, "B")
        MATERIALS.connect_material_expressions(noise, "", tint, "Alpha")
        base = tint
    MATERIALS.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    constant(material, roughness, unreal.MaterialProperty.MP_ROUGHNESS)
    constant(material, metallic, unreal.MaterialProperty.MP_METALLIC)
    if name in ("AmberGlass", "InkGlass", "WindowGlass"):
        material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        material.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
        constant(material, 0.06 if name == "WindowGlass" else 0.68, unreal.MaterialProperty.MP_OPACITY)
    if name == "LampGlow":
        emission = color_node(material, (3, 1.8, 0.5))
        MATERIALS.connect_material_property(emission, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if name == "WindowGlass":
        material.set_editor_property("two_sided", True)
        constant(material, 1.01, unreal.MaterialProperty.MP_REFRACTION)
    if name == "InkLiquid":
        emission = color_node(material, (0.025, 0.12, 0.26))
        MATERIALS.connect_material_property(emission, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if name == "WarmPlaster":
        MATERIALS.set_base_material_usage(material, unreal.MaterialUsage.MATUSAGE_NANITE)
    MATERIALS.recompile_material(material)
    LIBRARY.save_loaded_asset(material)
    return material


materials = {name: create_material(name, settings) for name, settings in MANIFEST["palette"].items()}
meshes = {}
for name, expected in MANIFEST["assets"].items():
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    data = options.get_editor_property("static_mesh_import_data")
    data.set_editor_property("combine_meshes", True)
    data.set_editor_property("auto_generate_collision", False)
    data.set_editor_property("one_convex_hull_per_ucx", True)
    data.set_editor_property("generate_lightmap_u_vs", True)
    data.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(SOURCE / f"{name}.fbx"))
    task.set_editor_property("destination_path", f"{DESTINATION}/Meshes")
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    task.set_editor_property("factory", unreal.FbxFactory())
    ASSETS.import_asset_tasks([task])
    mesh = LIBRARY.load_asset(f"{DESTINATION}/Meshes/{name}")
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError(f"Missing imported static mesh {name}")
    for index, slot in enumerate(mesh.get_editor_property("static_materials")):
        slot_name = str(slot.get_editor_property("imported_material_slot_name"))
        # Reimport preserves legacy slots even after their geometry is removed.
        if slot_name == "WindowGlow":
            slot_name = "WindowGlass"
        mesh.set_material(index, materials[slot_name])
    count = MESHES.get_convex_collision_count(mesh)
    if count != expected["collision_hulls"]:
        raise RuntimeError(f"{name} collision hull count {count}, expected {expected['collision_hulls']}")
    extent = mesh.get_bounds().box_extent
    dimensions = [extent.x * 2, extent.y * 2, extent.z * 2]
    if any(abs(a - b) > 0.5 for a, b in zip(dimensions, expected["dimensions_cm"])):
        raise RuntimeError(f"Unexpected dimensions for {name}: {dimensions}")
    LIBRARY.save_loaded_asset(mesh)
    meshes[name] = mesh
    unreal.log(f"SHOP_ART_MESH {name} dimensions={dimensions} collision={count}")

# A recipe-specific presentation child keeps the generic pickup class reusable.
path = "/Game/Shop/Native/BP_DawnTonic"
blueprint = LIBRARY.load_asset(path) if LIBRARY.does_asset_exist(path) else None
if not blueprint:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.ShopPickupItem)
    blueprint = ASSETS.create_asset("BP_DawnTonic", "/Game/Shop/Native", unreal.Blueprint, factory)
subobjects = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
for handle in subobjects.k2_gather_subobject_data_for_blueprint(blueprint):
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, blueprint)
    if isinstance(component, unreal.StaticMeshComponent):
        component.set_static_mesh(meshes["SM_DawnTonic"])
        component.set_relative_location(unreal.Vector(0, 0, 0), False, False)
        component.set_relative_scale3d(unreal.Vector(1, 1, 1))
        component.set_relative_rotation(unreal.Rotator(), False, False)
        component.set_editor_property("override_materials", [])
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
LIBRARY.save_loaded_asset(blueprint)
tonic_class = LIBRARY.load_blueprint_class(path)
tonic_defaults = unreal.get_default_object(tonic_class)
tonic_defaults.set_editor_property("product_id", "DawnTonic")
tonic_defaults.set_editor_property("display_name", unreal.Text("Dawn Tonic"))
LIBRARY.save_loaded_asset(blueprint)
recipe = LIBRARY.load_asset("/Game/Shop/Recipes/DA_DawnTonic")
recipe.set_editor_property("output_item_class", tonic_class)
LIBRARY.save_loaded_asset(recipe)

backup = PROJECT / "Saved/BeforeShopArt/L_Shop.umap"
backup.parent.mkdir(parents=True, exist_ok=True)
if not backup.exists():
    shutil.copy2(PROJECT / "Content/Levels/L_Shop.umap", backup)
if not LEVEL.load_level("/Game/Levels/L_Shop"):
    raise RuntimeError("Could not load the shop map")


def actor_with_label(label, actor_class, location):
    actor = next((a for a in ACTORS.get_all_level_actors() if a.get_actor_label() == label), None)
    if not actor:
        actor = ACTORS.spawn_actor_from_class(actor_class, unreal.Vector(*location))
        actor.set_actor_label(label)
    return actor


def furniture(label, mesh, location):
    actor = actor_with_label(label, unreal.StaticMeshActor, location)
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    component.set_static_mesh(mesh)
    component.set_collision_profile_name("BlockAll")
    actor.set_folder_path("ShopArt")
    return actor


furniture("Art_BrewingBench", meshes["SM_ApothecaryBench"], (600, 350, 0))
furniture("Art_ApothecaryShelving", meshes["SM_ApothecaryShelving"], (780, 440, 0))
furniture("Art_MortarAndPestle", meshes["SM_MortarAndPestle"], (665, 375, 100))
pendant = furniture("Art_BrassPendant", meshes["SM_BrassPendant"], (600, 350, 220))
pendant.get_component_by_class(unreal.StaticMeshComponent).set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)

for actor in ACTORS.get_all_level_actors():
    if isinstance(actor, unreal.ShopBrewingStation):
        for component in actor.get_components_by_class(unreal.StaticMeshComponent):
            replacements = {"VesselMesh": "SM_BrewingVessel", "MoonleafContainer": "SM_MoonleafJar", "WaterContainer": "SM_SpringWaterJug"}
            if component.get_name() in replacements:
                component.set_editor_property("static_mesh", meshes[replacements[component.get_name()]])
                component.set_relative_scale3d(unreal.Vector(1, 1, 1))
                old = component.get_relative_transform().translation
                if component.get_name() == "MoonleafContainer" and old.x == -36 and old.y == 0:
                    old = unreal.Vector(-45, 25, 0)
                if component.get_name() == "WaterContainer" and old.x == 36 and old.y == 0:
                    old = unreal.Vector(45, 35, 0)
                component.set_relative_location(unreal.Vector(old.x, old.y, 0), False, False)
                component.set_editor_property("override_materials", [])

# Soft local lighting makes the first material set readable in the greybox room.
for label, position, target, intensity, color in (
    ("Art_WarmKey", (590, 220, 260), (650, 350, 80), 1200, (255, 219, 171)),
    ("Art_CoolFill", (840, 260, 230), (750, 440, 100), 600, (194, 221, 255)),
):
    light = actor_with_label(label, unreal.RectLight, position)
    light.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*position), unreal.Vector(*target)), False)
    component = light.get_component_by_class(unreal.RectLightComponent)
    component.set_mobility(unreal.ComponentMobility.MOVABLE)
    component.set_intensity(intensity)
    component.set_editor_property("cast_raytraced_shadow", unreal.CastRayTracedShadow.DISABLED)
    component.set_editor_property("allow_mega_lights", False)
    component.set_light_color(unreal.LinearColor(color[0] / 255, color[1] / 255, color[2] / 255, 1))
    component.set_editor_property("source_width", 160)
    component.set_editor_property("source_height", 100)
    component.set_editor_property("attenuation_radius", 650)
    light.set_folder_path("ShopArt")

# Replace only the six measured one-metre blocks supporting the brewing table.
removed = []
for actor in list(ACTORS.get_all_level_actors()):
    p = actor.get_actor_location()
    if isinstance(actor, unreal.StaticMeshActor) and actor.get_actor_label() in {
        "SM_Cube60", "SM_Cube61", "SM_Cube62", "SM_Cube63", "SM_Cube64", "SM_Cube65"
    } and p.x in (500, 600, 700) and p.y in (300, 400) and p.z == 0:
        removed.append(actor.get_actor_label())
        ACTORS.destroy_actor(actor)

# Give the loose sample the same product identity as its authored bottle.
for actor in ACTORS.get_all_level_actors():
    if isinstance(actor, unreal.ShopPickupItem) and actor.get_actor_label() == "BP_Bottle":
        actor.set_editor_property("product_id", "DawnTonic")
        actor.set_editor_property("display_name", unreal.Text("Dawn Tonic"))
        component = actor.get_editor_property("item_mesh")
        component.set_static_mesh(meshes["SM_DawnTonic"])
        component.set_relative_location(unreal.Vector(0, 0, 0), False, False)
        component.set_relative_scale3d(unreal.Vector(1, 1, 1))
        component.set_relative_rotation(unreal.Rotator(), False, False)
        component.set_editor_property("override_materials", [])

if not LEVEL.save_current_level():
    raise RuntimeError("Could not save the art checkpoint")
unreal.log(f"SHOP_ART_INSTALL_COMPLETE replaced={removed}")
