"""Add the physical till and checkout counter to the current saved shop."""

from pathlib import Path
import shutil
import unreal

project = Path(unreal.Paths.project_dir()).resolve()
backup = project / "Saved/BeforeTill/L_Shop.umap"
backup.parent.mkdir(parents=True, exist_ok=True)
if not backup.exists():
    shutil.copy2(project / "Content/Levels/L_Shop.umap", backup)

library = unreal.EditorAssetLibrary
path = "/Game/Shop/Native/BP_ShopTill"
blueprint = library.load_asset(path) if library.does_asset_exist(path) else None
if not blueprint:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.ShopTill)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset("BP_ShopTill", "/Game/Shop/Native", unreal.Blueprint, factory)
mesh = library.load_asset("/Game/Shop/Art/Meshes/SM_ApothecaryTill")
assert mesh
subobjects = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)


def configure(component):
    if isinstance(component, unreal.StaticMeshComponent) and component.get_name() == "TillMesh":
        component.set_editor_property("static_mesh", mesh)
        component.set_relative_location(unreal.Vector(0, 0, 0), False, False)
        component.set_relative_scale3d(unreal.Vector(1, 1, 1))


for handle in subobjects.k2_gather_subobject_data_for_blueprint(blueprint):
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    configure(unreal.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, blueprint))
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
library.save_loaded_asset(blueprint)

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert level.load_level("/Game/Levels/L_Shop")
customer = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.ShopCustomer))
location = customer.get_actor_location()
floor_z = location.z - 88
till = next((a for a in actors.get_all_level_actors() if isinstance(a, unreal.ShopTill)), None)
if not till:
    till = actors.spawn_actor_from_class(library.load_blueprint_class(path), unreal.Vector(location.x, location.y - 100, floor_z + 100))
    till.set_actor_label("ShopTill")
till.set_editor_property("customer", customer)
till.set_folder_path("Checkout")
configure(till.get_editor_property("till_mesh"))
counter = next((a for a in actors.get_all_level_actors() if a.get_actor_label() == "CheckoutCounter"), None)
if not counter:
    counter = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(location.x, location.y - 100, floor_z))
    counter.set_actor_label("CheckoutCounter")
    counter.set_actor_scale3d(unreal.Vector(0.65, 0.6, 1))
counter.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(library.load_asset("/Game/Shop/Art/Meshes/SM_ApothecaryBench"))
counter.set_folder_path("Checkout")
assert level.save_current_level()
unreal.log("SHOP_TILL_INSTALL_COMPLETE")
