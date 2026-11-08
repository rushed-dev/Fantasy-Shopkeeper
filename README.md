# Fantasy Shopkeeper

A cosy first-person fantasy shop prototype built with Godot 4.7.2 and GDScript.

**Current checkpoint: first customers.** Movement and stocking have been playtested. Customers now browse shelf stock, carry baskets to the counter, and pay through a manual total/change interaction. This is a one-customer-at-a-time slice, with three explicitly invited visits. Days, expenses, loans, workers, saves and the optional encounter remain future milestones.

## Play this checkpoint

1. Stock the six delivery bottles on their matching shelves. Prices are printed on the shelf labels.
2. Approach the counter, aim at the ledger, and press **E**.
3. Choose **Invite a customer**, then watch them browse and return to the counter. Only items on shelves are available for sale; they can miss items even when delivery stock exists.
4. Open the ledger again. Add the listed prices yourself, enter the basket total, and choose **Agree total**.
5. Read the customer's payment and dispense change. Click or hold a coin pouch, or hold **1 / 2 / 3** for bits (1), marks (10), and crowns (100). Click a tray stack or press **Shift + 1 / 2 / 3** to take back one coin.
6. Choose **Confirm sale**. Incorrect totals or change allow correction. Any correct combination of coins works. Leave the counter and return whenever you like; the pending sale and tray remain intact.
7. After the customer leaves, invite the next one. There are three visits, with baskets of one, two, and three items. Waiting never penalises the player. Reset from the pause menu to try again.

WASD/arrows move, mouse looks, **E** handles stock or the ledger, and **Escape** returns from the ledger or pauses the shop. Look sensitivity is in the opening/pause menu. The opening menu says **FIRST CUSTOMERS**, which distinguishes this checkpoint from the earlier delivery-only build.

## Project documents

- [Prototype specification](docs/prototype-spec.md)
- [Implementation checklist](docs/implementation-plan.md)
- [Validation evidence and limitations](docs/validation.md)
- [Domain glossary](GLOSSARY.md)
- [Historical cloud handoff](docs/cloud-handoff.md)

## Windows development

Pull the latest `main` in the project folder opened by Godot. Run `tools/setup-godot.ps1` once to download the pinned portable engine, then:

```powershell
./tools/run.ps1 -Test
./tools/run.ps1 -Smoke
./tools/run.ps1 -Editor
./tools/run.ps1
```

`Play.cmd` launches the source project after engine setup. PowerShell launcher changes have not been executed on this Linux host. For packaged play, extract the Windows ZIP in `builds/` and launch `FantasyShopkeeper.exe`; pulling source does not update an already exported executable.

## Linux / cloud development

```bash
export XDG_CACHE_HOME="$PWD/.tools/cache"
export XDG_DATA_HOME="$PWD/.tools/data"
export XDG_CONFIG_HOME="$PWD/.tools/config"
mkdir -p "$XDG_CACHE_HOME" "$XDG_DATA_HOME" "$XDG_CONFIG_HOME"
bash tools/setup-godot-linux.sh
bash tools/check-godot-linux.sh
```

The check runner imports the project, runs shared item actions, the physical scene regression, checkout regression, and scene smoke. It rejects engine errors even when Godot exits zero. Logs are generated under ignored `test-results/`.

With a graphical display available, use the pinned engine to launch the editor or game:

```bash
.tools/godot/Godot_v4.7.2-stable_linux.x86_64 --editor --path .
.tools/godot/Godot_v4.7.2-stable_linux.x86_64 --path .
```

Run the checkout regression with rendering (omit Dummy to test real audio):

```bash
.tools/godot/Godot_v4.7.2-stable_linux.x86_64 --audio-driver Dummy --fixed-fps 60 --path . --script res://tests/checkout_test.gd -- --capture-checkout
```

This saves ledger/customer screenshots under `builds/screenshots/`. Headless checks do not play coin audio. To test continuous navigation and stocking with a display:

```bash
.tools/godot/Godot_v4.7.2-stable_linux.x86_64 --audio-driver Dummy --disable-render-loop --path . --script res://tests/shop_scene_test.gd -- --walk-route
```

Remove `--disable-render-loop` to watch the route; add `--capture-final=/absolute/path/shop.png` only with rendering enabled. The default scene test relocates between stations for quicker geometry checks; headless runs explicitly skip walking.

## Export

`bash tools/build-linux.sh` runs validation, installs checksum-verified official templates, exports Windows/Linux, checks the Linux release, and packages ZIPs with licence notices and SHA256 sidecars under ignored `builds/`. Native Windows execution and human usability of the new checkout remain unverified here. No additional services or credentials are required.
