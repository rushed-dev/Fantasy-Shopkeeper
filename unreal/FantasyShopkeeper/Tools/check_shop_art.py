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
sections = [a for a in all_actors if isinstance(a, unreal.ShopShelfSection)]
expected_sections = {f"ApothecarySection_{z}" for z in (20, 60, 100, 140)}
assert len(sections) == 4, f"Expected four stock sections, found {len(sections)}"
assert {a.get_actor_label() for a in sections} == expected_sections
assert all(a.get_editor_property("available_for_sale") for a in sections)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
shelving = next(a for a in all_actors if a.get_actor_label() == "Art_ApothecaryShelving")
center, extent = shelving.get_actor_bounds(False)
assert abs(center.z - extent.z) < 0.5 and center.z + extent.z > 190, (center, extent)
for section in sections:
    markers = section.get_editor_property("position_markers")
    assert markers.get_instance_count() == 8
    assert markers.static_mesh.get_name() == "SM_StockMarkerRing"
    for index in range(8):
        transform = markers.get_instance_transform(index, world_space=True)
        position = transform.translation
        hit = unreal.SystemLibrary.line_trace_single(
            world, position + unreal.Vector(0, 0, 1), position + unreal.Vector(0, 0, -3),
            unreal.TraceTypeQuery.ECC_VISIBILITY, False, sections, unreal.DrawDebugTrace.NONE, True)
        assert hit and isinstance(hit.to_tuple()[9], unreal.StaticMeshActor), f"Unsupported position {section.get_actor_label()} [{index}]"
station = next(a for a in all_actors if isinstance(a, unreal.ShopBrewingStation))
assert station.get_editor_property("vessel_mesh").static_mesh.get_name() == "SM_BrewingVessel"
assert {str(k): v for k, v in station.get_editor_property("starting_ingredients").items()} == {"Moonleaf": 6, "Spring Water": 6}
assert station.get_editor_property("moonleaf_container").static_mesh.get_name() == "SM_MoonleafJar"
assert station.get_editor_property("water_container").static_mesh.get_name() == "SM_SpringWaterJug"
customer = next(a for a in all_actors if isinstance(a, unreal.ShopCustomer))
assert customer.get_editor_property("mesh").skeletal_mesh_asset, "Customer mannequin mesh is missing"
assert any(isinstance(a, unreal.NavMeshBoundsVolume) for a in all_actors), "Customer navigation bounds missing"
till = next(a for a in all_actors if isinstance(a, unreal.ShopTill))
assert till.get_editor_property("till_mesh").static_mesh.get_name() == "SM_CheckoutCounter"
assert till.get_editor_property("customer") == customer
assert till.get_editor_property("physical_counter")
for material_name in ("Copper", "Silver", "Brass"):
    assert unreal.load_asset(f"/Game/Shop/Art/Materials/M_{material_name}").get_editor_property("used_with_instanced_static_meshes")
assert till.get_editor_property("pouch_mesh").get_name() == "SM_CoinPouch"
assert len(till.get_editor_property("coin_meshes")) == 3
assert abs(till.get_actor_location().z) < 1
assert not any(a.get_actor_label() == "CheckoutCounter" for a in all_actors)
crate = next(a for a in all_actors if isinstance(a, unreal.ShopSupplyCrate))
assert crate.get_editor_property("station") == station
assert crate.get_editor_property("crate_mesh").static_mesh.get_name() == "SM_SupplyCrate"
assert crate.get_editor_property("price_bits") == 12
assert len(crate.get_editor_property("offers")) == 3
assert {str(k): v for k, v in crate.get_editor_property("delivery").items()} == {"Moonleaf": 6, "Spring Water": 3}
flow = next(a for a in all_actors if isinstance(a, unreal.ShopCustomerFlow))
assert flow.get_editor_property("till") == till
assert flow.get_editor_property("max_concurrent_customers") == 3
assert flow.get_editor_property("checkout_follows_till")
checkout_offset = flow.get_editor_property("till_checkout_offset")
assert checkout_offset.x ** 2 + checkout_offset.y ** 2 < 150 ** 2
assert 1 <= flow.get_editor_property("visits_per_day") <= 100
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
assert not any(isinstance(a, unreal.ShopShelfSlot) and a.get_editor_property("available_for_sale") for a in all_actors)
# Ignore glazing for a trace through each opening to its exterior garden. Any
# retained cube or interior wall panel would intercept these paths.
for side, start, end in (("Back", (600, 600, 180), (600, 1000, 180)),
                         ("Side", (900, 400, 180), (1250, 400, 180))):
    glass = next(a for a in all_actors if a.get_actor_label() == f"Art_{side}WindowGlass")
    assert glass.get_component_by_class(unreal.StaticMeshComponent).static_mesh.get_name() == "SM_WindowGlass"
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(*start), unreal.Vector(*end),
        unreal.TraceTypeQuery.ECC_VISIBILITY, False, [glass], unreal.DrawDebugTrace.NONE, True)
    assert not hit or not hit.to_tuple()[0], f"Window opening is blocked: {side} {hit}"
material = unreal.load_asset("/Game/Shop/Art/Materials/M_WindowGlass")
assert material.get_editor_property("blend_mode") == unreal.BlendMode.BLEND_TRANSLUCENT
assert sum(a.get_actor_label().endswith("Pendant") for a in all_actors) == 4
sun = next(a for a in all_actors if isinstance(a, unreal.DirectionalLight))
assert sun.get_component_by_class(unreal.DirectionalLightComponent).get_editor_property("intensity") >= 10000
assert all(any(a.get_actor_label() == label for a in all_actors) for label in (
    "Art_EntryLight", "Art_CheckoutLight", "Art_PrepLight", "Art_BackWindowLight", "Art_SideWindowLight"))
assert station.get_editor_property("output_slot").get_editor_property("slot_marker").static_mesh.get_name() == "SM_StockMarkerRing"
assert all(any(a.get_actor_label() == label for a in all_actors) for label in (
    "Art_WorkbenchWindow", "Art_SideWindow", "Art_DawnRecipeBook", "Art_DoorSurround",
    "Art_PrepFront", "Art_PrepBack"))
assert not any(a.get_actor_label() in {"Art_RetailFront", "Art_RetailBack"} for a in all_actors)
unreal.log(f"SHOP_ART_CHECK_PASSED meshes={len(manifest['assets'])} collisions={sum(a['collision_hulls'] for a in manifest['assets'].values())} recipe_mesh=verified stock_sections={len(sections)} positions={len(sections) * 8} window_openings=2 pendants=4 customer_mesh=verified")
