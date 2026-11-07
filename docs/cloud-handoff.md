# Cloud handoff — 2026-11-07

## User intent and current authorization

The user completed a grilling session and approved the two-day prototype scope, repository specification, implementation checklist, and starting the first playable milestone. During that milestone they asked: "can we pause and move the work into the cloud". Local feature work stopped immediately. This checkpoint preserves work for a cloud continuation; it does not claim that a cloud task has been started.

Read prototype-spec.md, implementation-plan.md, and the root GLOSSARY.md before resuming. These capture the decisions from the conversation; do not restart the interview or silently expand the prototype. The current next goal is to finish and validate milestone 1. The entire two-day game is not yet built.

## Non-negotiable design decisions

- Cosy and relaxed, first person, single player, Windows, keyboard/mouse, Godot/GDScript.
- Satisfying ordinary retail is the foundation. Optional stories unlock bonus opportunities; mundane play remains viable.
- Player and workers use the same physical item actions and constraints. No worker-only inventory shortcuts.
- Manual calculation of basket totals and change. Three base-ten coin grades; click or hold dispensing; any correct coin combination accepted; mistakes allow correction.
- Fixed daily customer opportunities, independent of time spent calculating. Queues may prompt remarks but never punish waiting. Missing shelf stock can lose sales, and early players must notice the missing product themselves.
- Unlimited mornings/evenings. Order in the evening; deliver next morning. Two days so the player can experience manual stocking followed by a mundane restocking helper.
- Daily shop costs, stock costs, and labour costs make manual work economically useful. Future magical machinery has costs/maintenance too.
- Most recent correction: use a basic recovery loan funding roughly one or two days of stock instead of forced bankruptcy/replay. Rich lender factions (nobles, thieves, dragons) are future ideas. Exact loan amounts and repayment tuning remain implementation decisions to validate, not agreed numerical requirements.
- Morning saves and unrestricted pause are in scope for later milestones. Crate carrying is an early follow-up after validating individual-item handling.
- Five focused sessions is an effort cap, followed by an evidence-based review. Target ten minutes of active trading plus unlimited preparation/arithmetic time.

## What exists

- Procedural shop scene: timber/plaster interior, delivery area, labelled display, placeholder counter, HUD, opening/pause menus, and glowing bottles.
- Shared `ItemActions.inspect/interact` boundary with reach, line-of-sight, product compatibility, occupancy, and unique item ownership checks.
- `ShopActor` intended for both player and future worker. `ShopPlayer` adds camera and movement.
- A public-boundary test with two actors, competing pickup/placement, invalid destinations, wall obstruction, and conservation of physical items.
- Portable Windows Godot setup and launch scripts; Linux setup script added for handoff.
- No customers, checkout implementation, calendar, supplier ordering, wages, loans, worker navigation, save system, optional encounter, or exported game yet.

## Verification actually performed

Windows engine: Godot 4.7.2 stable official, build ed1daf0bf, downloaded from godotengine/godot-builds.

1. Headless editor import succeeded after rerunning with permission outside the Windows sandbox. First restricted import hit engine-cache and certificate-store access errors.
2. `--headless --path . --script res://tests/item_actions_test.gd`: **30 assertions, 0 failures**. Restricted runtime also emitted user log/certificate-store access errors; these were environment errors, not assertion failures. A clean cloud run is still needed.
3. Scene smoke command reached its timed exit but emitted a real script error:

```text
SCRIPT ERROR: Cannot call method 'reparent' on a null value.
at: ShopPlayer._init (res://scripts/player.gd:26)
```

`carry_anchor` is null when the player constructor tries to reparent it. Investigate base/derived initialization, likely an explicit base constructor call or moving dependent setup to a safe initialization stage. This is a diagnosis to verify, not a fix already applied.

**The current smoke harness prints "SCENE SMOKE PASS" even after script errors and exits zero. Do not treat that line/exit status as a passing validation.** Strengthen it or check engine logs for script errors and actual player/scene invariants before reporting success.

4. No graphical run, screenshot review, movement test, full stocking playthrough, or Windows export has been completed. Shelf reach, obstruction geometry, labels, UI size, and mouse capture still need visual validation.

## Cloud environment setup

Repository: https://github.com/rushed-dev/Fantasy-Shopkeeper

Use a Linux environment with Python 3, curl, and Godot 4.7.2. `bash tools/setup-godot-linux.sh` downloads the official portable Linux engine under ignored `.tools/godot/`. No game assets or package registries are required. Godot export templates will be needed later for a proper Windows build.

The Linux setup script is untested on the local Windows machine. Confirm its download and engine launch during environment setup. Internet policy must allow the official GitHub release download and redirects. Do not copy the Windows executable into Linux or commit engine/cache files.

After setup:

```bash
GODOT=.tools/godot/Godot_v4.7.2-stable_linux.x86_64
"$GODOT" --headless --editor --path . --import
"$GODOT" --headless --path . --script res://tests/item_actions_test.gd
"$GODOT" --headless --path . -- --smoke-test
```

Use the environment's supported display/screenshot workflow for visual testing if available. A headless smoke test alone does not validate the user experience. The scene also supports `-- --capture=/absolute/path/shop.png` during a graphical run; this path has not been tested yet.

## Resume prompt

> Read docs/cloud-handoff.md, docs/prototype-spec.md, docs/implementation-plan.md, and GLOSSARY.md. Resume milestone 1 of the approved Fantasy Shopkeeper prototype. Set up Godot 4.7.2 for Linux, fix the documented player-initialisation error, make smoke validation reject script errors, rerun the shared item-action tests, and visually verify walking and stocking all six bottles. Preserve the shared player/worker action interface. Report the playable result and any remaining limitations before expanding into later milestones. The approved full prototype includes a basic recovery loan, not forced bankruptcy. Do not restart the grilling session.

## Skills and environment boundary

The local Matt Pocock skills were installed in the user's home directory, not the repository. They are not part of this checkpoint. The agreed design is fully captured here; finishing milestone 1 does not require re-running a skill setup interview. If a later task explicitly needs those skills, install them in the cloud environment rather than assuming the Windows personal skill paths exist there.
