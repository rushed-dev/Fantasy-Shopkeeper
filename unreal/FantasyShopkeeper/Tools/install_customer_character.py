"""Upgrade the existing customer presentation and install shop navigation."""

from pathlib import Path
import shutil
import unreal

project = Path(unreal.Paths.project_dir()).resolve()
backup = project / "Saved/BeforeCustomerCharacter/L_Shop.umap"
backup.parent.mkdir(parents=True, exist_ok=True)
if not backup.exists():
    shutil.copy2(project / "Content/Levels/L_Shop.umap", backup)

library = unreal.EditorAssetLibrary
path = "/Game/Shop/Native/BP_CustomerCharacter"
blueprint = library.load_asset(path) if library.does_asset_exist(path) else None
if not blueprint:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.ShopCustomer)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset("BP_CustomerCharacter", "/Game/Shop/Native", unreal.Blueprint, factory)
mesh = library.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")
animation = library.load_blueprint_class("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed")
assert blueprint and mesh and animation
subobjects = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)


def configure(component):
    if isinstance(component, unreal.SkeletalMeshComponent):
        component.set_editor_property("skeletal_mesh_asset", mesh)
        component.set_editor_property("anim_class", animation)
        component.set_relative_location(unreal.Vector(0, 0, -88), False, False)
        component.set_relative_rotation(unreal.Rotator(yaw=-90), False, False)
    if isinstance(component, unreal.BoxComponent) and component.get_name() == "InteractionBox":
        component.set_relative_location(unreal.Vector(0, 0, 0), False, False)
        component.set_box_extent(unreal.Vector(31, 31, 88))


for handle in subobjects.k2_gather_subobject_data_for_blueprint(blueprint):
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, blueprint)
    configure(component)
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
library.save_loaded_asset(blueprint)

level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert level.load_level("/Game/Levels/L_Shop")
customer_class = library.load_blueprint_class(path)
for actor in list(actors.get_all_level_actors()):
    if isinstance(actor, unreal.ShopCustomer):
        if actor.get_class() != customer_class:
            replacement = actors.spawn_actor_from_class(customer_class, actor.get_actor_location(), actor.get_actor_rotation())
            replacement.set_actor_label(actor.get_actor_label())
            for prop in ("requested_product_id", "requested_product_name", "price_bits"):
                replacement.set_editor_property(prop, actor.get_editor_property(prop))
            actors.destroy_actor(actor)
            actor = replacement
        for component in actor.get_components_by_class(unreal.SceneComponent):
            configure(component)
        location = actor.get_actor_location()
        if location.z < 20:
            actor.set_actor_location(unreal.Vector(location.x, location.y, location.z + 88), False, True)

volume = next((a for a in actors.get_all_level_actors() if isinstance(a, unreal.NavMeshBoundsVolume)), None)
if not volume:
    volume = actors.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(350, 200, 150))
    volume.set_actor_label("ShopNavigationBounds")
    volume.set_actor_scale3d(unreal.Vector(7, 6, 2))
center, extent = volume.get_actor_bounds(False)
unreal.log(f"CUSTOMER_NAV_BOUNDS center={center} extent={extent}")
assert extent.x > 500 and extent.y > 400 and extent.z > 100, "Navigation volume has no usable brush bounds"
volume.set_folder_path("ShopNavigation")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not any(isinstance(a, unreal.RecastNavMesh) for a in actors.get_all_level_actors()):
    navigation = actors.spawn_actor_from_class(unreal.RecastNavMesh, unreal.Vector(0, 0, 0))
    navigation.set_actor_label("ShopNavMesh")
for actor in actors.get_all_level_actors():
    if isinstance(actor, unreal.RecastNavMesh):
        actor.set_editor_property("runtime_generation", unreal.RuntimeGenerationType.DYNAMIC)
        actor.set_editor_property("force_rebuild_on_load", True)
        actor.set_editor_property("agent_radius", 34)
        actor.set_editor_property("agent_height", 180)
unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
path = unreal.NavigationSystemV1.find_path_to_location_synchronously(world, unreal.Vector(350, 150, 0), unreal.Vector(530, -170, 0))
unreal.log(f"CUSTOMER_NAV_ROUTE valid={path.is_valid()} points={len(path.path_points)}")
assert path.is_valid() and not path.is_partial(), "Build navigation with the commandlet async-wait override before saving"
assert level.save_current_level()
unreal.log("CUSTOMER_CHARACTER_INSTALL_COMPLETE")
