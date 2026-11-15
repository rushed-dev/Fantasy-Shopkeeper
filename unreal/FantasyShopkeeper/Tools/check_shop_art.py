"""Validate the imported kit and saved recipe/map presentation without gameplay."""

import json
from pathlib import Path
import unreal

project = Path(unreal.Paths.project_dir()).resolve()
manifest = json.loads((project.parents[1] / "art/shop-corner/manifest.json").read_text())
unreal.load_module("StaticMeshEditor")
editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
for name, expected in manifest["assets"].items():
    mesh = unreal.load_asset(f"/Game/Shop/Art/Meshes/{name}")
    assert mesh, name
    extent = mesh.get_bounds().box_extent
    dimensions = [extent.x * 2, extent.y * 2, extent.z * 2]
    assert all(abs(a - b) < 0.5 for a, b in zip(dimensions, expected["dimensions_cm"])), dimensions
    assert editor.get_convex_collision_count(mesh) == expected["collision_hulls"], name
    assert all(slot.material_interface for slot in mesh.get_editor_property("static_materials")), name

tonic_class = unreal.EditorAssetLibrary.load_blueprint_class("/Game/Shop/Native/BP_DawnTonic")
recipe = unreal.load_asset("/Game/Shop/Recipes/DA_DawnTonic")
assert recipe.get_editor_property("output_item_class") == tonic_class
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level("/Game/Levels/L_Shop")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
sample = actors.spawn_actor_from_class(tonic_class, unreal.Vector(0, 0, -1000))
component = sample.get_editor_property("item_mesh")
assert component.static_mesh.get_name() == "SM_DawnTonic"
transform = component.get_relative_transform()
assert transform.translation == unreal.Vector(0, 0, 0), transform.translation
assert transform.scale3d == unreal.Vector(1, 1, 1), transform.scale3d
rotation = transform.rotation.rotator()
assert abs(rotation.pitch) < 0.01 and abs(rotation.roll) < 0.01 and abs(rotation.yaw) < 0.01, rotation
assert str(sample.get_editor_property("product_id")) == "DawnTonic"
assert sample.get_editor_property("pickup_sound").get_name() == "S_BottlePickup"
assert sample.get_editor_property("placement_sound").get_name() == "S_BottlePlace"
actors.destroy_actor(sample)
all_actors = actors.get_all_level_actors()
for label in ("Art_BrewingBench", "Art_ApothecaryShelving"):
    assert sum(a.get_actor_label() == label for a in all_actors) == 1, label
slots = [a for a in all_actors if a.get_actor_label().startswith("Art_ShelfSlot_")]
assert len(slots) == 6
assert all(isinstance(a, unreal.ShopShelfSlot) and a.get_editor_property("available_for_sale") for a in slots)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
shelving = next(a for a in all_actors if a.get_actor_label() == "Art_ApothecaryShelving")
center, extent = shelving.get_actor_bounds(False)
assert abs(center.z - extent.z) < 0.5 and center.z + extent.z > 190, (center, extent)
for slot in slots:
    position = slot.get_actor_location()
    hit = unreal.SystemLibrary.line_trace_single(
        world, position + unreal.Vector(0, -100, 12), position + unreal.Vector(0, 0, 12),
        unreal.TraceTypeQuery.ECC_VISIBILITY, False, [], unreal.DrawDebugTrace.NONE, True)
    assert hit and hit.to_tuple()[9] == slot, (slot.get_actor_label(), hit)
    hit = unreal.SystemLibrary.line_trace_single(
        world, position + unreal.Vector(0, 0, 1), position + unreal.Vector(0, 0, -2),
        unreal.TraceTypeQuery.ECC_VISIBILITY, False, slots, unreal.DrawDebugTrace.NONE, True)
    assert hit and hit.to_tuple()[9] == shelving, (slot.get_actor_label(), hit)
station = next(a for a in all_actors if isinstance(a, unreal.ShopBrewingStation))
assert station.get_editor_property("vessel_mesh").static_mesh.get_name() == "SM_BrewingVessel"
assert station.get_editor_property("moonleaf_container").static_mesh.get_name() == "SM_MoonleafJar"
assert station.get_editor_property("water_container").static_mesh.get_name() == "SM_SpringWaterJug"
customer = next(a for a in all_actors if isinstance(a, unreal.ShopCustomer))
assert customer.get_editor_property("mesh").skeletal_mesh_asset, "Customer mannequin mesh is missing"
assert any(isinstance(a, unreal.NavMeshBoundsVolume) for a in all_actors), "Customer navigation bounds missing"
till = next(a for a in all_actors if isinstance(a, unreal.ShopTill))
assert till.get_editor_property("till_mesh").static_mesh.get_name() == "SM_ApothecaryTill"
assert till.get_editor_property("customer") == customer
crate = next(a for a in all_actors if isinstance(a, unreal.ShopSupplyCrate))
assert crate.get_editor_property("station") == station
assert crate.get_editor_property("crate_mesh").static_mesh.get_name() == "SM_SupplyCrate"
assert crate.get_editor_property("price_bits") == 12
assert len(crate.get_editor_property("offers")) == 3
assert {str(k): v for k, v in crate.get_editor_property("delivery").items()} == {"Moonleaf": 6, "Spring Water": 3}
flow = next(a for a in all_actors if isinstance(a, unreal.ShopCustomerFlow))
assert flow.get_editor_property("till") == till
assert flow.get_editor_property("customer_class") == customer.get_class()
assert sum(a.get_actor_label().startswith("Art_Floor_") for a in all_actors) == 66
assert sum(a.get_actor_label().startswith("Art_Ceiling_") for a in all_actors) == 66
ink_recipe = unreal.load_asset("/Game/Shop/Recipes/DA_MoonlitInk")
assert ink_recipe and str(ink_recipe.get_editor_property("product_id")) == "MoonlitInk"
assert {str(k): v for k, v in ink_recipe.get_editor_property("ingredient_costs").items()} == {"Moonleaf": 1, "Spring Water": 2}
assert len(station.get_editor_property("available_recipes")) == 2
orders = flow.get_editor_property("orders")
assert len(orders) == 2 and str(orders[1].get_editor_property("product_id")) == "MoonlitInk"
ink_class = unreal.EditorAssetLibrary.load_blueprint_class("/Game/Shop/Native/BP_MoonlitInk")
ink = actors.spawn_actor_from_class(ink_class, unreal.Vector(0, 0, -1000))
assert ink.get_editor_property("item_mesh").static_mesh.get_name() == "SM_MoonlitInk"
assert str(ink.get_editor_property("product_id")) == "MoonlitInk"
assert ink.get_editor_property("pickup_sound").get_name() == "S_BottlePickup"
actors.destroy_actor(ink)
visitor = actors.spawn_actor_from_class(orders[1].get_editor_property("customer_class"), unreal.Vector(0, 0, -1000))
assert visitor.get_editor_property("mesh").skeletal_mesh_asset.get_name() == "SKM_Quinn_Simple"
actors.destroy_actor(visitor)
post = next(a for a in all_actors if a.get_actor_label() == "Art_ShopPostProcess")
assert unreal.load_asset("/Game/Shop/Art/Materials/M_WarmPlaster").get_editor_property("used_with_nanite")
assert abs(post.get_editor_property("settings").get_editor_property("auto_exposure_bias") + 0.65) < 0.01
assert recipe.get_editor_property("brew_sound").get_name() == "S_BrewComplete"
assert ink_recipe.get_editor_property("brew_sound").get_name() == "S_BrewComplete"
assert till.get_editor_property("coin_sound").get_name() == "S_CoinTray"
assert till.get_editor_property("sale_sound").get_name() == "S_SaleComplete"
assert crate.get_editor_property("delivery_sound").get_name() == "S_BottlePlace"
sound_manifest = json.loads((project.parents[1] / "art/shop-audio/manifest.json").read_text())
for name, expected in sound_manifest.items():
    sound = unreal.load_asset(f"/Game/Shop/Audio/{name}")
    assert sound and abs(sound.get_editor_property("duration") - expected["duration_seconds"]) < 0.01, name
sale_slots = [a for a in all_actors if isinstance(a, unreal.ShopShelfSlot) and a.get_editor_property("available_for_sale")]
for slot in sale_slots:
    assert slot.get_editor_property("slot_marker").static_mesh.get_name() == "SM_StockMarkerRing"
    position = slot.get_actor_location()
    hit = unreal.SystemLibrary.line_trace_single(
        world, position + unreal.Vector(0, 0, 1), position + unreal.Vector(0, 0, -3),
        unreal.TraceTypeQuery.ECC_VISIBILITY, False, sale_slots, unreal.DrawDebugTrace.NONE, True)
    assert hit and isinstance(hit.to_tuple()[9], unreal.StaticMeshActor), f"Unsupported sale slot {slot.get_actor_label()}"
assert station.get_editor_property("output_slot").get_editor_property("slot_marker").static_mesh.get_name() == "SM_StockMarkerRing"
assert all(any(a.get_actor_label() == label for a in all_actors) for label in (
    "Art_WorkbenchWindow", "Art_SideWindow", "Art_DawnRecipeBook", "Art_DoorSurround",
    "Art_RetailFront", "Art_RetailBack", "Art_PrepFront", "Art_PrepBack"))
unreal.log(f"SHOP_ART_CHECK_PASSED meshes={len(manifest['assets'])} collisions={sum(a['collision_hulls'] for a in manifest['assets'].values())} recipe_mesh=verified stock_positions=6 approach_and_support_traces=12 customer_mesh=verified")
