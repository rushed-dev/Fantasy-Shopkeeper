# Fantasy Shopkeeper

A cosy first-person fantasy shop prototype built with Godot 4.7.2 and GDScript.

**Checkpoint status:** development paused for a cloud handoff on 2026-11-07. The first milestone is incomplete. Shared item-action tests pass (30 assertions); the scene has a known player-initialisation error and has not had a visual playtest. This is source for continued development, not a finished playable release.

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

`Play.cmd` is a double-click launcher after engine setup. Fix the known initialisation error before treating a launch as playable. Engine downloads and caches are deliberately ignored by Git.

## Linux / cloud development

```bash
bash tools/setup-godot-linux.sh
.tools/godot/Godot_v4.7.2-stable_linux.x86_64 --headless --editor --path . --import
.tools/godot/Godot_v4.7.2-stable_linux.x86_64 --headless --path . --script res://tests/item_actions_test.gd
.tools/godot/Godot_v4.7.2-stable_linux.x86_64 --headless --path . -- --smoke-test
```

Linux setup has been prepared for the handoff but has not been executed on the Windows host. The game target remains Windows. Headless checks are not evidence of visual quality or complete playability. Read the smoke-test caveat in the handoff.

## First milestone controls

WASD or arrows to move, mouse to look, E to pick up/place a bottle, Escape to pause. The opening menu exposes mouse sensitivity. The scene has a delivery tray, six labelled shelf spaces, and six bottles, including glowing starlight. Customers, sales, days, wages, loans, and saves are planned but not implemented yet.
