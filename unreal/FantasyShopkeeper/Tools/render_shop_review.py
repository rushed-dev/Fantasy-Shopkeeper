"""Render two static shop views without saving the map.

Run in UnrealEditor with -RenderOffscreen and -ExecCmds="py <this script>".
The high-resolution renderer receives 64 warm-up frames for temporal effects.
"""

from pathlib import Path
import unreal

project = Path(unreal.Paths.project_dir()).resolve()
output = project / "Saved/ArtReview"
output.mkdir(parents=True, exist_ok=True)
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert level.load_level("/Game/Levels/L_Shop")
world = editor.get_editor_world()
unreal.SystemLibrary.execute_console_command(world, "r.HighResScreenshotDelay 64")
camera = actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(400, 70, 225))
component = camera.get_component_by_class(unreal.CameraComponent)
component.set_field_of_view(65)
component.set_editor_property("constrain_aspect_ratio", False)
component.set_editor_property("aspect_ratio", 1.6)
tonic = unreal.EditorAssetLibrary.load_blueprint_class("/Game/Shop/Native/BP_DawnTonic")
ink = unreal.EditorAssetLibrary.load_blueprint_class("/Game/Shop/Native/BP_MoonlitInk")
for cls, position in ((tonic, (747, 430, 100)), (tonic, (769, 430, 100)),
                      (ink, (791, 430, 60)), (ink, (813, 430, 60)), (tonic, (650, 330, 100))):
    actors.spawn_actor_from_class(cls, unreal.Vector(*position))
frames = 0


def set_view(position, target):
    location = unreal.Vector(*position)
    rotation = unreal.MathLibrary.find_look_at_rotation(location, unreal.Vector(*target))
    camera.set_actor_location(location, False, False)
    camera.set_actor_rotation(rotation, False)
    editor.set_level_viewport_camera_info(location, rotation)


set_view((400, 70, 225), (665, 370, 105))


def tick(delta):
    global frames
    frames += 1
    try:
        if frames == 200:
            unreal.AutomationLibrary.take_high_res_screenshot(1600, 1000, str(output / "brewing-corner.png"), camera=camera)
        if frames == 300:
            set_view((160, -220, 215), (500, 220, 125))
        if frames == 500:
            unreal.AutomationLibrary.take_high_res_screenshot(1600, 1000, str(output / "shop-interior.png"), camera=camera)
        if frames == 600:
            unreal.unregister_slate_post_tick_callback(callback)
            unreal.SystemLibrary.quit_editor()
    except Exception:
        unreal.unregister_slate_post_tick_callback(callback)
        unreal.SystemLibrary.quit_editor()
        raise


callback = unreal.register_slate_post_tick_callback(tick)

