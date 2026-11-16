"""Dress the saved greybox and add repeatable visits and paid ingredient parcels.

Run after install_shop_art.py. Maintains the saved shop layout, physical checkout,
product-locked sections, window openings, navigation and customer presentations.
"""

from datetime import datetime
from pathlib import Path
import shutil
import unreal

project = Path(unreal.Paths.project_dir()).resolve()
backup = project / f"Saved/BeforeShopExpansion/{datetime.now():%Y%m%d-%H%M%S}/L_Shop.umap"
backup.parent.mkdir(parents=True, exist_ok=True)
shutil.copy2(project / "Content/Levels/L_Shop.umap", backup)
library = unreal.EditorAssetLibrary
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert level.load_level("/Game/Levels/L_Shop")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
till = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.ShopTill))
customer = till.get_editor_property("customer")
assert customer
# The small device had been moved against the entrance wall. The full counter
# needs standing room on both sides, with its cashier side facing into the room.
p = till.get_actor_location()
if p.y <= -300:
    till.set_actor_location(unreal.Vector(p.x, -180, p.z), False, False)
    till.set_actor_rotation(unreal.Rotator(yaw=180), False)
checkout_position = unreal.MathLibrary.transform_location(till.get_actor_transform(), unreal.Vector(0, 100, 88 - till.get_actor_location().z))
customer.set_actor_location(checkout_position, False, False)



def actor(label, cls, position, yaw=0):
    result = next((a for a in actors.get_all_level_actors() if a.get_actor_label() == label), None)
    if not result:
        result = actors.spawn_actor_from_class(cls, unreal.Vector(*position), unreal.Rotator(yaw=yaw))
        result.set_actor_label(label)
    result.set_folder_path("ShopExpansion")
    return result


def mesh_actor(label, mesh, position, yaw=0, decorative=False):
    result = actor(label, unreal.StaticMeshActor, position, yaw)
    component = result.get_component_by_class(unreal.StaticMeshComponent)
    component.set_static_mesh(library.load_asset(f"/Game/Shop/Art/Meshes/{mesh}"))
    component.set_collision_profile_name("NoCollision" if decorative else "BlockAll")
    return result


# Fit a reusable interior skin to the measured perimeter. Columns are identified
# from the saved greybox, so the existing doorway opening is preserved.
plaster = library.load_asset("/Game/Shop/Art/Materials/M_WarmPlaster")
for wall in list(actors.get_all_level_actors()):
    if not isinstance(wall, unreal.StaticMeshActor) or not wall.get_actor_label().startswith("SM_Cube"):
        continue
    p = wall.get_actor_location()
    window_strip = (p.y == 700 and p.x in (500, 600)) or (p.x == 1000 and p.y in (300, 400))
    if window_strip and p.z in (0, 100, 200):
        actors.destroy_actor(wall)
        continue
    if p.x not in (-300, 1000) and p.y not in (-500, 700):
        continue
    component = wall.get_component_by_class(unreal.StaticMeshComponent)
    component.set_material(0, plaster)
    if p.z != 0:
        continue
    if p.y == 700:
        position, yaw = (p.x + 50, 698, 0), 0
    elif p.y == -500:
        position, yaw = (p.x + 50, -398, 0), 180
    elif p.x == -300:
        position, yaw = (-198, p.y + 50, 0), 90
    else:
        position, yaw = (998, p.y + 50, 0), -90
    mesh_actor(f"Art_WallPanel_{int(p.x)}_{int(p.y)}", "SM_PlasterWallPanel", position, yaw)

for x in (-190, 100, 400, 700, 990):
    mesh_actor(f"Art_BackTimber_{x}", "SM_TimberPost", (x, 688, 0))
for x in (-190, 100, 500, 800, 990):
    mesh_actor(f"Art_FrontTimber_{x}", "SM_TimberPost", (x, -388, 0))
for y in (-350, -50, 250, 550):
    for x in (-188, 988):
        mesh_actor(f"Art_SideTimber_{x}_{y}", "SM_TimberPost", (x, y, 0), 90)

for x in range(-100, 1000, 200):
    for y in range(-350, 700, 100):
        mesh_actor(f"Art_Floor_{x}_{y}", "SM_OakFloorTile", (x, y, 0.1))
        ceiling = mesh_actor(f"Art_Ceiling_{x}_{y}", "SM_OakFloorTile", (x, y, 312))
        ceiling.set_actor_scale3d(unreal.Vector(1, 1, 4))
for y in (-390, -100, 200, 500, 690):
    beam = mesh_actor(f"Art_CeilingBeam_{y}", "SM_TimberPost", (-200, y, 291))
    beam.set_actor_rotation(unreal.Rotator(pitch=90), False)
    beam.set_actor_scale3d(unreal.Vector(1, 1, 4))

for old in list(actors.get_all_level_actors()):
    if old.get_actor_label() in {"Art_WallPanel_500_700", "Art_WallPanel_600_700", "Art_WallPanel_1000_300", "Art_WallPanel_1000_400"}:
        actors.destroy_actor(old)
for label, position, yaw in (("Back", (600, 700, 0), 0), ("Side", (1000, 400, 0), -90)):
    wall = mesh_actor(f"Art_{label}WindowWall", "SM_WindowWall", position, yaw)
    wall.set_actor_location(unreal.Vector(*position), False, False)
    frame_label = "Art_WorkbenchWindow" if label == "Back" else "Art_SideWindow"
    raised_position = (position[0], position[1], 100)
    raised = unreal.Vector(*raised_position)
    frame = mesh_actor(frame_label, "SM_ArchedWindow", raised_position, yaw, decorative=True)
    frame.set_actor_location(raised, False, False)
    glass = mesh_actor(f"Art_{label}WindowGlass", "SM_WindowGlass", raised_position, yaw)
    glass.set_actor_location(raised, False, False)
    glass.get_component_by_class(unreal.StaticMeshComponent).set_editor_property("cast_shadow", False)
for label, mesh, position, yaw in (
    ("Art_BackGarden", "SM_GardenGround", (600, 1250, 0.5), 0),
    ("Art_SideGarden", "SM_GardenGround", (1500, 350, 0.5), 0),
    ("Art_BackFence", "SM_GardenFence", (600, 1150, 0), 0),
    ("Art_SideFence", "SM_GardenFence", (1350, 400, 0), -90),
    ("Art_BackTree", "SM_GardenTree", (780, 1400, 0), 0),
    ("Art_SideTree", "SM_GardenTree", (1500, 550, 0), 0),
):
    mesh_actor(label, mesh, position, yaw)
for label, x, y in (("Art_BrassPendant", 600, 350), ("Art_CheckoutPendant", 130, -180),
                    ("Art_PrepPendant", 150, 400), ("Art_EntryPendant", 350, -200)):
    pendant = mesh_actor(label, "SM_BrassPendant", (x, y, 242), decorative=True)
    pendant.set_actor_location(unreal.Vector(x, y, 242), False, False)
mesh_actor("Art_DoorSurround", "SM_ShopDoorSurround", (300, -390, 0), 180)
mesh_actor("Art_DawnRecipeBook", "SM_DawnRecipeBook", (665, 320, 100), decorative=True)
mesh_actor("Art_DryingMoonleaf_1", "SM_DryingMoonleaf", (460, 676, 200), decorative=True)
mesh_actor("Art_DryingMoonleaf_2", "SM_DryingMoonleaf", (750, 676, 200), decorative=True)
mesh_actor("Art_EntryFern", "SM_PottedFern", (920, -320, 0))
mesh_actor("Art_BenchFern", "SM_PottedFern", (905, 590, 0))
mesh_actor("Art_InkRecipeBook", "SM_InkRecipeBook", (175, 340, 100), decorative=True)

# Window and checkout lights use explicit lumen units in the enclosed room.
for label, position, target, intensity, color, width, height in (
    ("Art_BackWindowLight", (600, 645, 200), (600, 250, 110), 1600, (0.7, 0.84, 1), 120, 160),
    ("Art_SideWindowLight", (945, 400, 200), (650, 400, 100), 1000, (0.7, 0.84, 1), 120, 160),
    ("Art_CheckoutLight", (130, -180, 239), (130, -180, 90), 900, (1, 0.8, 0.57), 140, 100),
    ("Art_EntryLight", (350, -200, 239), (350, -200, 90), 900, (1, 0.8, 0.57), 100, 100),
    ("Art_PrepLight", (150, 400, 239), (150, 350, 100), 700, (1, 0.86, 0.65), 140, 100),
):
    light = actor(label, unreal.RectLight, position)
    light.set_actor_location(unreal.Vector(*position), False, False)
    light.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*position), unreal.Vector(*target)), False)
    component = light.get_component_by_class(unreal.RectLightComponent)
    component.set_mobility(unreal.ComponentMobility.MOVABLE)
    component.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
    component.set_intensity(intensity)
    component.set_light_color(unreal.LinearColor(*color, 1))
    component.set_editor_property("source_width", width)
    component.set_editor_property("source_height", height)
    component.set_editor_property("attenuation_radius", 700)
    component.set_editor_property("cast_raytraced_shadow", unreal.CastRayTracedShadow.DISABLED)
    component.set_editor_property("allow_mega_lights", False)

# The original greybox sun used six lux, which left exterior views dark.
# Give the existing atmosphere a daylight source and capture it in the skylight.
sun = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.DirectionalLight))
sun.set_actor_rotation(unreal.Rotator(pitch=-38, yaw=-135), False)
sun_component = sun.get_component_by_class(unreal.DirectionalLightComponent)
sun_component.set_intensity(12000)
sun_component.set_light_color(unreal.LinearColor(1, 0.9, 0.78, 1))
sky = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.SkyLight))
sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
sky_component.set_editor_property("real_time_capture", True)
sky_component.set_intensity(0.6)

post = actor("Art_ShopPostProcess", unreal.PostProcessVolume, (500, 150, 150))
post.set_editor_property("unbound", True)
settings = unreal.PostProcessSettings()
for property_name, value in (
    ("auto_exposure_bias", -0.65), ("bloom_intensity", 0.15), ("film_grain_intensity", 0),
    ("lumen_scene_lighting_quality", 2), ("lumen_final_gather_quality", 2),
):
    settings.set_editor_property(f"override_{property_name}", True)
    settings.set_editor_property(property_name, value)
post.set_editor_property("settings", settings)

# Keep the preparation tables; the retail tables were intentionally removed.
for label, location in (("Art_PrepFront", (150, 350, 0)), ("Art_PrepBack", (150, 450, 0))):
    table = mesh_actor(label, "SM_ApothecaryBench", location)
    table.set_actor_scale3d(unreal.Vector(300 / 220, 1, 1))
removed = []
for old in list(actors.get_all_level_actors()):
    p = old.get_actor_location()
    retail = old.get_actor_label() in {f"SM_Cube{i}" for i in range(66, 72)} and p.x in (500, 600, 700) and p.y in (-100, 0)
    prep = old.get_actor_label() in {f"SM_Cube{i}" for i in range(52, 58)} and p.x in (0, 100, 200) and p.y in (300, 400)
    if isinstance(old, unreal.StaticMeshActor) and ((p.z == 0 and (retail or prep))
            or old.get_actor_label() in {"Art_RetailFront", "Art_RetailBack"}):
        removed.append(old.get_actor_label())
        actors.destroy_actor(old)

station = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.ShopBrewingStation))
station.set_editor_property("starting_ingredients", {"Moonleaf": 6, "Spring Water": 6})
till = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.ShopTill))
customer = till.get_editor_property("customer")
assert customer

ink_path = "/Game/Shop/Native/BP_MoonlitInk"
ink_blueprint = library.load_asset(ink_path) if library.does_asset_exist(ink_path) else None
if not ink_blueprint:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.ShopPickupItem)
    ink_blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset("BP_MoonlitInk", "/Game/Shop/Native", unreal.Blueprint, factory)
subobjects = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
for handle in subobjects.k2_gather_subobject_data_for_blueprint(ink_blueprint):
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, ink_blueprint)
    if isinstance(component, unreal.StaticMeshComponent):
        component.set_static_mesh(library.load_asset("/Game/Shop/Art/Meshes/SM_MoonlitInk"))
        component.set_relative_location(unreal.Vector(0, 0, 0), False, False)
        component.set_relative_scale3d(unreal.Vector(1, 1, 1))
        component.set_relative_rotation(unreal.Rotator(), False, False)
unreal.BlueprintEditorLibrary.compile_blueprint(ink_blueprint)
ink_class = library.load_blueprint_class(ink_path)
defaults = unreal.get_default_object(ink_class)
defaults.set_editor_property("product_id", "MoonlitInk")
defaults.set_editor_property("display_name", unreal.Text("Moonlit Ink"))
library.save_loaded_asset(ink_blueprint)
recipe_path = "/Game/Shop/Recipes/DA_MoonlitInk"
ink_recipe = library.load_asset(recipe_path) if library.does_asset_exist(recipe_path) else None
if not ink_recipe:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.ShopBrewingRecipe)
    ink_recipe = unreal.AssetToolsHelpers.get_asset_tools().create_asset("DA_MoonlitInk", "/Game/Shop/Recipes", unreal.ShopBrewingRecipe, factory)
ink_recipe.set_editor_property("product_id", "MoonlitInk")
ink_recipe.set_editor_property("display_name", unreal.Text("Moonlit Ink"))
ink_recipe.set_editor_property("ingredient_costs", {"Moonleaf": 1, "Spring Water": 2})
ink_recipe.set_editor_property("output_item_class", ink_class)
library.save_loaded_asset(ink_recipe)
tonic_recipe = library.load_asset("/Game/Shop/Recipes/DA_DawnTonic")
station.set_editor_property("available_recipes", [tonic_recipe, ink_recipe])
# Replace independent sale targets with eight-position sections. Brewing output
# remains an independent station slot. Backups above preserve the prior layout.
for old in list(actors.get_all_level_actors()):
    if (isinstance(old, unreal.ShopShelfSlot) and old.get_editor_property("available_for_sale")) or old.get_actor_label() in {
        "Art_ShelfPrice", "Art_DisplayPrice", "Art_InkPrice", "Art_RetailInkPrice", "CheckoutCounter"}:
        actors.destroy_actor(old)
# The approved layout has only the four apothecary sections (32 positions).
# Do not recreate the six retired table sections when regenerating the room.
retired_sections = {f"RetailSection_{x}_{y}" for x in (550, 650, 750) for y in (-50, 50)}
for old in list(actors.get_all_level_actors()):
    if isinstance(old, unreal.ShopShelfSection) and old.get_actor_label() in retired_sections:
        actors.destroy_actor(old)
for z in (20, 60, 100, 140):
    actor(f"ApothecarySection_{z}", unreal.ShopShelfSection, (780, 440, z))
ring_mesh = library.load_asset("/Game/Shop/Art/Meshes/SM_StockMarkerRing")
marker = station.get_editor_property("output_slot").get_editor_property("slot_marker")
marker.set_static_mesh(ring_mesh)
marker.set_relative_scale3d(unreal.Vector(1, 1, 1))
marker.set_relative_location(unreal.Vector(0, 0, 0.1), False, False)
marker.set_editor_property("cast_shadow", False)

# The native till owns its keys, coin stacks, pouch, tray and world-space ticket.
till_blueprint = library.load_asset("/Game/Shop/Native/BP_ShopTill")
assert till_blueprint
counter_mesh = library.load_asset("/Game/Shop/Art/Meshes/SM_CheckoutCounter")
subobjects = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
for handle in subobjects.k2_gather_subobject_data_for_blueprint(till_blueprint):
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, till_blueprint)
    if isinstance(component, unreal.StaticMeshComponent) and component.get_name() == "TillMesh":
        component.set_static_mesh(counter_mesh)
        component.set_relative_location(unreal.Vector(), False, False)
        component.set_relative_scale3d(unreal.Vector(1, 1, 1))
unreal.BlueprintEditorLibrary.compile_blueprint(till_blueprint)
for target in (unreal.get_default_object(library.load_blueprint_class("/Game/Shop/Native/BP_ShopTill")), till):
    target.set_editor_property("physical_counter", True)
    target.set_editor_property("coin_meshes", [library.load_asset(f"/Game/Shop/Art/Meshes/SM_Coin{v}") for v in (1, 10, 100)])
    target.set_editor_property("pouch_mesh", library.load_asset("/Game/Shop/Art/Meshes/SM_CoinPouch"))
    target.set_editor_property("key_mesh", library.load_asset("/Game/Shop/Art/Meshes/SM_TillKey"))
component = till.get_editor_property("till_mesh")
component.set_static_mesh(counter_mesh)
component.set_relative_location(unreal.Vector(), False, False)
component.set_relative_scale3d(unreal.Vector(1, 1, 1))
# The original small till sat 100 cm above the floor. Migrate its origin once.
p = till.get_actor_location()
if p.z > 50:
    till.set_actor_location(unreal.Vector(p.x, p.y, p.z - 100), False, False)
library.save_loaded_asset(till_blueprint)

path = "/Game/Shop/Native/BP_SupplyCrate"
blueprint = library.load_asset(path) if library.does_asset_exist(path) else None
if not blueprint:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.ShopSupplyCrate)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset("BP_SupplyCrate", "/Game/Shop/Native", unreal.Blueprint, factory)
subobjects = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
crate_mesh = library.load_asset("/Game/Shop/Art/Meshes/SM_SupplyCrate")
for handle in subobjects.k2_gather_subobject_data_for_blueprint(blueprint):
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, blueprint)
    if isinstance(component, unreal.StaticMeshComponent):
        component.set_static_mesh(crate_mesh)
        component.set_relative_scale3d(unreal.Vector(1, 1, 1))
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
library.save_loaded_asset(blueprint)
crate = actor("SupplyCrate", library.load_blueprint_class(path), (440, 350, 0))
crate.set_editor_property("station", station)
crate.set_editor_property("offers", unreal.get_default_object(unreal.ShopSupplyCrate).get_editor_property("offers"))
crate.get_editor_property("crate_mesh").set_static_mesh(crate_mesh)
crate.get_editor_property("crate_mesh").set_relative_scale3d(unreal.Vector(1, 1, 1))

flow = actor("CustomerFlow", unreal.ShopCustomerFlow, (0, 0, 0))
flow.set_editor_property("max_concurrent_customers", 3)
flow.set_editor_property("chapter_cast", True)
flow.set_editor_property("till", till)
flow.set_editor_property("customer_class", customer.get_class())
flow_transform = flow.get_actor_transform()
flow.set_editor_property("entrance", unreal.MathLibrary.inverse_transform_location(flow_transform, unreal.Vector(300, -540, 88)))
flow.set_editor_property("checkout", unreal.MathLibrary.inverse_transform_location(flow_transform, customer.get_actor_location()))
for presentation, mesh_name in (("BP_CustomerCharacter", "SKM_Manny_Simple"),):
    blueprint = library.load_asset(f"/Game/Shop/Native/{presentation}")
    for handle in subobjects.k2_gather_subobject_data_for_blueprint(blueprint):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, blueprint)
        if isinstance(component, unreal.SkeletalMeshComponent):
            component.set_editor_property("skeletal_mesh_asset", library.load_asset(f"/Game/Characters/Mannequins/Meshes/{mesh_name}"))
            component.set_editor_property("anim_class", library.load_blueprint_class("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"))
            component.set_relative_location(unreal.Vector(0, 0, -88), False, False)
            component.set_relative_rotation(unreal.Rotator(yaw=-90), False, False)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    library.save_loaded_asset(blueprint)
for navigation in actors.get_all_level_actors():
    if isinstance(navigation, unreal.RecastNavMesh):
        navigation.set_editor_property("runtime_generation", unreal.RuntimeGenerationType.DYNAMIC)
        navigation.set_editor_property("force_rebuild_on_load", True)
        navigation.set_editor_property("agent_radius", 34)
        navigation.set_editor_property("agent_height", 180)
navigation_bounds = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.NavMeshBoundsVolume))
navigation_bounds.set_actor_location(unreal.Vector(350, 100, 150), False, False)
navigation_bounds.set_actor_scale3d(unreal.Vector(7, 7, 2))

visitor_path = "/Game/Shop/Native/BP_InkCustomer"
visitor_blueprint = library.load_asset(visitor_path) if library.does_asset_exist(visitor_path) else None
if not visitor_blueprint:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.ShopCustomer)
    visitor_blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset("BP_InkCustomer", "/Game/Shop/Native", unreal.Blueprint, factory)
for handle in subobjects.k2_gather_subobject_data_for_blueprint(visitor_blueprint):
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, visitor_blueprint)
    if isinstance(component, unreal.SkeletalMeshComponent):
        component.set_editor_property("skeletal_mesh_asset", library.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple"))
        component.set_editor_property("anim_class", library.load_blueprint_class("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"))
        component.set_relative_location(unreal.Vector(0, 0, -88), False, False)
        component.set_relative_rotation(unreal.Rotator(yaw=-90), False, False)
unreal.BlueprintEditorLibrary.compile_blueprint(visitor_blueprint)
library.save_loaded_asset(visitor_blueprint)
orders = []
for product_id, name, price, tender in (("DawnTonic", "Dawn Tonic", 12, 20), ("MoonlitInk", "Moonlit Ink", 18, 50)):
    order = unreal.ShopCustomerOrder()
    order.set_editor_property("product_id", product_id)
    order.set_editor_property("product_name", unreal.Text(name))
    order.set_editor_property("price_bits", price)
    order.set_editor_property("tender_bits", tender)
    order.set_editor_property("customer_class", library.load_blueprint_class(visitor_path) if product_id == "MoonlitInk" else customer.get_class())
    orders.append(order)
flow.set_editor_property("orders", orders)

unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
entry = unreal.MathLibrary.transform_location(flow_transform, flow.get_editor_property("entrance"))
route = unreal.NavigationSystemV1.find_path_to_location_synchronously(world, entry, customer.get_actor_location())
unreal.log(f"CHECKOUT_ROUTE till={till.get_actor_location()} customer={customer.get_actor_location()} route={route.path_points if route else None}")
assert route and route.is_valid() and not route.is_partial(), "Entrance must have a complete route to checkout"
assert level.save_current_level()
unreal.log(f"SHOP_EXPANSION_INSTALLED removed_supports={removed} entrance_route_points={len(route.path_points)} backup={backup}")
