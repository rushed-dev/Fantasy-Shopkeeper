"""Add the minimal-sale customer while preserving the current shop layout."""

from pathlib import Path
import shutil
import unreal

project = Path(unreal.Paths.project_dir())
backup = project / "Saved/BeforeSale/L_Shop.umap"
backup.parent.mkdir(parents=True, exist_ok=True)
if not backup.exists():
    shutil.copy2(project / "Content/Levels/L_Shop.umap", backup)

path = "/Game/Shop/Native/BP_ShopCustomer"
if not unreal.EditorAssetLibrary.does_asset_exist(path):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.ShopCustomer)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "BP_ShopCustomer", "/Game/Shop/Native", unreal.Blueprint, factory
    )
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
customer_class = unreal.EditorAssetLibrary.load_blueprint_class(path)
if not customer_class or not unreal.MathLibrary.class_is_child_of(customer_class, unreal.ShopCustomer):
    raise RuntimeError("Unexpected customer Blueprint type")

level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not level.load_level("/Game/Levels/L_Shop"):
    raise RuntimeError("Could not load shop map")
customers = [actor for actor in actors.get_all_level_actors() if isinstance(actor, unreal.ShopCustomer)]
if not customers:
    customer = actors.spawn_actor_from_class(customer_class, unreal.Vector(350, 150, 0))
    customer.set_actor_label("TonicCustomer")
for actor in actors.get_all_level_actors():
    if isinstance(actor, unreal.ShopBrewingStation):
        output = actor.get_editor_property("output_slot")
        if output:
            output.set_editor_property("available_for_sale", False)
if not level.save_current_level():
    raise RuntimeError("Could not save sale checkpoint")
unreal.log("SALE_INSTALL_COMPLETE")
