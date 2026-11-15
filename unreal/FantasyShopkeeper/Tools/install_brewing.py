"""Install the first recipe and brewing station through Unreal's editor asset API."""

from pathlib import Path
import shutil
import unreal

project = Path(unreal.Paths.project_dir())
backup = project / "Saved/BeforeBrewing/L_Shop.umap"
backup.parent.mkdir(parents=True, exist_ok=True)
if not backup.exists():
    shutil.copy2(project / "Content/Levels/L_Shop.umap", backup)

assets = unreal.AssetToolsHelpers.get_asset_tools()
recipe_path = "/Game/Shop/Recipes/DA_DawnTonic"
recipe = unreal.EditorAssetLibrary.load_asset(recipe_path) if unreal.EditorAssetLibrary.does_asset_exist(recipe_path) else None
if not recipe:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.ShopBrewingRecipe)
    recipe = assets.create_asset("DA_DawnTonic", "/Game/Shop/Recipes", unreal.ShopBrewingRecipe, factory)
    recipe.set_editor_property("product_id", "DawnTonic")
    recipe.set_editor_property("display_name", unreal.Text("Dawn Tonic"))
    recipe.set_editor_property("ingredient_costs", {"Moonleaf": 2, "Spring Water": 1})
    recipe.set_editor_property("output_item_class", unreal.EditorAssetLibrary.load_blueprint_class("/Game/Shop/Native/BP_ShopPickupItem"))
    unreal.EditorAssetLibrary.save_loaded_asset(recipe)
if not isinstance(recipe, unreal.ShopBrewingRecipe):
    raise RuntimeError("Unexpected recipe asset type")

station_path = "/Game/Shop/Native/BP_BrewingStation"
if not unreal.EditorAssetLibrary.does_asset_exist(station_path):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.ShopBrewingStation)
    blueprint = assets.create_asset("BP_BrewingStation", "/Game/Shop/Native", unreal.Blueprint, factory)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
station_class = unreal.EditorAssetLibrary.load_blueprint_class(station_path)
if not station_class or not unreal.MathLibrary.class_is_child_of(station_class, unreal.ShopBrewingStation):
    raise RuntimeError("Unexpected brewing station Blueprint type")

level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not level.load_level("/Game/Levels/L_Shop"):
    raise RuntimeError("Could not load shop map")
stations = [actor for actor in actors.get_all_level_actors() if isinstance(actor, unreal.ShopBrewingStation)]
if not stations:
    station = actors.spawn_actor_from_class(station_class, unreal.Vector(550, 350, 100))
    station.set_actor_label("BrewingStation")
    slot_class = unreal.EditorAssetLibrary.load_blueprint_class("/Game/Shop/Native/BP_ShopShelfSlot")
    output = actors.spawn_actor_from_class(slot_class, unreal.Vector(620, 350, 100))
    output.set_actor_label("BrewingOutput")
    output.set_editor_property("available_for_sale", False)
    station.set_editor_property("recipe", recipe)
    station.set_editor_property("starting_ingredients", {"Moonleaf": 6, "Spring Water": 3})
    station.set_editor_property("output_slot", output)
if not level.save_current_level():
    raise RuntimeError("Could not save brewing checkpoint")
unreal.log("BREWING_INSTALL_COMPLETE")
