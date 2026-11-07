# Fantasy Shopkeeper

A cosy first-person fantasy shop prototype built with Godot 4.7.2 and GDScript.

**Checkpoint status:** cloud development resumed on 2026-11-07. Player initialisation uses local attachment before entering the scene tree. Shared actions pass 30 assertions; the continuous graphical walk-and-stock route passes 74 assertions. All six bottles can be stocked through movement, aiming, and interaction inputs without teleporting. Windows and Linux release ZIPs are available under `builds/`. The extracted Linux release passed startup validation. Native Windows execution and subjective mouse/interaction feel still need a human playtest. See [validation evidence](docs/validation.md).

## Start here

- [Cloud handoff and immediate next steps](docs/cloud-handoff.md)
- [Approved prototype specification](docs/prototype-spec.md)
- [Implementation checklist](docs/implementation-plan.md)
- [Domain glossary](GLOSSARY.md)

## Windows development

Run `tools/setup-godot.ps1` once to download the pinned portable engine. Then use:

```powershell
./tools/run.ps1 -Test
./tools/run.ps1 -Smoke
./tools/run.ps1 -Editor
./tools/run.ps1
```

`Play.cmd` is a double-click launcher after engine setup. Engine downloads and caches are deliberately ignored by Git.

## Linux / cloud development

```bash
bash tools/setup-godot-linux.sh
.tools/godot/Godot_v4.7.2-stable_linux.x86_64 --headless --editor --path . --import
.tools/godot/Godot_v4.7.2-stable_linux.x86_64 --headless --path . --script res://tests/item_actions_test.gd
.tools/godot/Godot_v4.7.2-stable_linux.x86_64 --headless --path . -- --smoke-test
```

Run `bash tools/check-godot-linux.sh` for import, item-action tests, scene tests, and smoke validation. This runner uses writable local cache/data directories and rejects engine errors even if Godot exits zero. Logs go to ignored `test-results/`.

Headless scene tests explicitly skip walking because mouse capture requires a display. With a graphical display, run the pinned engine with `--audio-driver Dummy --path . --script res://tests/shop_scene_test.gd` (omit Dummy when testing audio). The scene test relocates the player between stations, then aims and interacts through the player path; it does not validate continuous navigation or manual mouse feel. Export templates are installed by `bash tools/setup-export-templates-linux.sh`. Run `bash tools/build-linux.sh` to validate, export and package both platforms. Native Windows execution remains untested.

## First milestone controls

WASD or arrows to move, mouse to look, E to pick up/place a bottle, Escape to pause. The opening menu exposes mouse sensitivity. The scene has a delivery tray, six labelled shelf spaces, and six bottles, including glowing starlight. Customers, sales, days, wages, loans, and saves are planned but not implemented yet.

## Continuous route validation

With a graphical display available, set the XDG directories as in `tools/check-godot-linux.sh`, then run:

```bash
.tools/godot/Godot_v4.7.2-stable_linux.x86_64 --audio-driver Dummy --disable-render-loop --path . --script res://tests/shop_scene_test.gd -- --walk-route
```

This route uses movement inputs between stations, tests collisions and pause, and aims and interacts with all six bottles. Omit `--disable-render-loop` to watch it. Add `--capture-final=/absolute/path/shop.png` only with rendering enabled to save the completed scene. The default scene test relocates between stations for a quicker geometry check; headless runs explicitly skip walking.
