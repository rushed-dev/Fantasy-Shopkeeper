# Milestone 1 validation — 2026-11-07

Godot 4.7.2 official ed1daf0bf, Linux x86_64. Engine and export-template archives verified against the official release SHA512 sums. No application services or credentials are required.

## Constructor follow-up

A user reported `get_global_transform(): !is_inside_tree()` at player construction on launch. The carry anchor now uses `remove_child` / `add_child` during construction instead of `reparent`, avoiding global-transform access before tree entry. Three regression assertions cover detached construction, camera parenting and local offset. This addresses the reported lifecycle path; native Windows validation remains pending.

## Passed

- Project import and compilation through `bash tools/check-godot-linux.sh`.
- Shared item-action suite: 30 assertions, 0 failures, including competing actors, capacity, compatibility, obstruction, and stock conservation.
- Scene regression: 36 headless assertions, 0 failures; walking is explicitly skipped without mouse capture. Covers constructor completion, aiming at every delivery/shelf slot, transfers of all six original items, completion HUD, opening menu, and pause/resume.
- Graphical continuous route: 74 assertions, 0 failures on Xvfb/Mesa software rendering. The player walks between the stations using movement inputs, then aims and interacts through the player path. Includes display collision, pause preventing movement, and preserving completed stock. No position relocation occurs during stocking in this mode.
- Rendered screenshot inspection before and after stocking: shop and labels visible, six bottles reach matching shelves, starlight emits light, HUD reaches 6/6 and displays completion feedback.
- Smoke regression: temporarily reintroduced the original missing-base-constructor call; the new smoke check rejected it with exit 1. Restored the fix afterwards.
- Windows x86_64 and Linux x86_64 release exports. Both ZIPs pass archive integrity checks. The Linux ZIP was extracted outside the repository and its standalone embedded game passed headless scene smoke validation.

Current test logs are generated under ignored `test-results/`; screenshots and release artifacts are under ignored `builds/`.

## Reproduce

Run `bash tools/check-godot-linux.sh` for the headless suite. With a graphical display and the XDG directories set as in that script:

```bash
.tools/godot/Godot_v4.7.2-stable_linux.x86_64 --audio-driver Dummy --disable-render-loop --path . --script res://tests/shop_scene_test.gd -- --walk-route
```

Remove `--disable-render-loop` to render the route. Add `--capture-final=/absolute/path/shop.png` to save the end state when rendering is enabled. Default scene tests relocate between stations; `--walk-route` tests continuous navigation instead. Headless use of `--walk-route` is rejected.

Run `bash tools/build-linux.sh` to validate, install checksum-verified export templates, export, smoke-test the Linux release, and create both ZIPs with SHA256 sidecars. Packages include Godot licence and third-party notices. No public release or repository push is performed.

## Still unverified

- Native Windows startup and play: this executor runs Linux and has no Windows runtime. The exported PE executable was built but has not run on Windows.
- PowerShell launcher changes were reviewed but cannot execute here because PowerShell is absent.
- Subjective mouse feel, comfort, and uncoached discovery. Scripted routes and screenshots do not replace a human playtest.
- Audio: graphical tests use the dummy audio driver because the executor has no audio device. There is currently no authored game audio.

The complete five-session prototype is not implemented. Customers, checkout, daily finances, persistence, workers, and the optional encounter remain future milestones; these checks establish the physical-shop checkpoint only.
