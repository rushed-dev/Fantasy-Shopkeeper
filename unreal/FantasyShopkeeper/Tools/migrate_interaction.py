"""Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>.

Creates native Blueprint children and upgrades the shop's placed interaction actors.
The original map is backed up under Saved before its first migration.
"""

from pathlib import Path
import shutil
import unreal


def native_blueprint(name, parent):
    path = f"/Game/Shop/Native/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        result = unreal.EditorAssetLibrary.load_blueprint_class(path)
        if not result or not unreal.MathLibrary.class_is_child_of(result, parent):
            raise RuntimeError(f"Unexpected existing asset at {path}")
        return result
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, "/Game/Shop/Native", unreal.Blueprint, factory
    )
    if not asset:
        raise RuntimeError(f"Could not create {path}")
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return unreal.EditorAssetLibrary.load_blueprint_class(path)


project = Path(unreal.Paths.project_dir())
map_file = project / "Content/Levels/L_Shop.umap"
backup = project / "Saved/BeforeNativeInteraction/L_Shop.umap"
backup.parent.mkdir(parents=True, exist_ok=True)
if not backup.exists():
    shutil.copy2(map_file, backup)

item_class = native_blueprint("BP_ShopPickupItem", unreal.ShopPickupItem)
slot_class = native_blueprint("BP_ShopShelfSlot", unreal.ShopShelfSlot)

level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not level.load_level("/Game/Levels/L_Shop"):
    raise RuntimeError("Could not load the shop")

for old in list(actors.get_all_level_actors()):
    class_path = old.get_class().get_path_name()
    if class_path == "/Game/Shop/BP_PickupItem.BP_PickupItem_C":
        replacement = actors.spawn_actor_from_class(item_class, old.get_actor_location(), old.get_actor_rotation())
        replacement.set_actor_transform(old.get_actor_transform(), False, True)
        replacement.set_actor_label(old.get_actor_label())
        old_mesh = old.get_component_by_class(unreal.StaticMeshComponent)
        mesh = replacement.get_component_by_class(unreal.StaticMeshComponent)
        if old_mesh:
            mesh.set_static_mesh(old_mesh.static_mesh)
            mesh.set_relative_transform(old_mesh.get_relative_transform(), False, True)
            for index, material in enumerate(old_mesh.get_materials()):
                mesh.set_material(index, material)
        actors.destroy_actor(old)
    elif class_path == "/Game/Shop/BP_ShelfSlot.BP_ShelfSlot_C":
        replacement = actors.spawn_actor_from_class(slot_class, old.get_actor_location(), old.get_actor_rotation())
        replacement.set_actor_transform(old.get_actor_transform(), False, True)
        replacement.set_actor_label(old.get_actor_label())
        old_arrows = old.get_components_by_class(unreal.ArrowComponent)
        if old_arrows:
            replacement.get_component_by_class(unreal.ArrowComponent).set_relative_transform(
                old_arrows[0].get_relative_transform(), False, True
            )
        actors.destroy_actor(old)

existing_slots = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.ShopShelfSlot)]
for index in range(len(existing_slots), 2):
    slot = actors.spawn_actor_from_class(slot_class, unreal.Vector(610 + index * 60, -50, 100))
    slot.set_actor_label(f"ShopShelfSlot_{index + 1}")

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.ShopGameMode)
if not level.save_current_level():
    raise RuntimeError("Could not save the migrated shop")
unreal.log("NATIVE_INTERACTION_MIGRATION_COMPLETE")
