"""Import and bind the original, short procedural shop sound cues."""

from datetime import datetime
import json
from pathlib import Path
import shutil
import unreal

project = Path(unreal.Paths.project_dir()).resolve()
source = project.parents[1] / "art/shop-audio"
manifest = json.loads((source / "manifest.json").read_text())
backup = project / f"Saved/BeforeShopAudio/{datetime.now():%Y%m%d-%H%M%S}/L_Shop.umap"
backup.parent.mkdir(parents=True, exist_ok=True)
shutil.copy2(project / "Content/Levels/L_Shop.umap", backup)
library = unreal.EditorAssetLibrary
sounds = {}
for name, expected in manifest.items():
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source / f"{name}.wav"))
    task.set_editor_property("destination_path", "/Game/Shop/Audio")
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound = library.load_asset(f"/Game/Shop/Audio/{name}")
    assert isinstance(sound, unreal.SoundWave), name
    assert abs(sound.get_editor_property("duration") - expected["duration_seconds"]) < 0.01, name
    sounds[name] = sound

for path in ("/Game/Shop/Native/BP_DawnTonic", "/Game/Shop/Native/BP_MoonlitInk",
             "/Game/Shop/Native/BP_ShopPickupItem"):
    blueprint = library.load_asset(path)
    defaults = unreal.get_default_object(library.load_blueprint_class(path))
    assert isinstance(defaults, unreal.ShopPickupItem), path
    defaults.set_editor_property("pickup_sound", sounds["S_BottlePickup"])
    defaults.set_editor_property("placement_sound", sounds["S_BottlePlace"])
    library.save_loaded_asset(blueprint)
for recipe in ("DA_DawnTonic", "DA_MoonlitInk"):
    asset = library.load_asset(f"/Game/Shop/Recipes/{recipe}")
    asset.set_editor_property("brew_sound", sounds["S_BrewComplete"])
    library.save_loaded_asset(asset)
for path, properties in (
    ("/Game/Shop/Native/BP_ShopTill", {"coin_sound": "S_CoinTray", "sale_sound": "S_SaleComplete"}),
    ("/Game/Shop/Native/BP_SupplyCrate", {"delivery_sound": "S_BottlePlace"}),
):
    blueprint = library.load_asset(path)
    defaults = unreal.get_default_object(library.load_blueprint_class(path))
    for property_name, sound_name in properties.items():
        defaults.set_editor_property(property_name, sounds[sound_name])
    library.save_loaded_asset(blueprint)

level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert level.load_level("/Game/Levels/L_Shop")
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if isinstance(actor, unreal.ShopTill):
        actor.set_editor_property("coin_sound", sounds["S_CoinTray"])
        actor.set_editor_property("sale_sound", sounds["S_SaleComplete"])
    if isinstance(actor, unreal.ShopSupplyCrate):
        actor.set_editor_property("delivery_sound", sounds["S_BottlePlace"])
assert level.save_current_level()
unreal.log(f"SHOP_AUDIO_INSTALLED cues={len(sounds)} backup={backup}")
