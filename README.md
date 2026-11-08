# Fantasy Shopkeeper

A cosy first-person fantasy shop prototype built with Godot 4.7.2 and GDScript.

**Current checkpoint: two trading days.** Stocking and manual checkout now sit inside preparation, trading, closing and evening planning. Set prices, buy the next delivery, manage cash and a simple recovery loan, and resume from morning checkpoints. Customers remain one at a time; queues, workers and the optional encounter are future work.

## Play this checkpoint

1. Continue the saved morning, or confirm **Start a fresh two-day experiment** from the opening/pause menu. A fresh shop starts with 24 bits and six already-paid delivery bottles. The menu says **TWO DAYS**.
2. Stock shelves at your pace. Approach the counter, aim at the ledger and press **E**. During preparation you can set shelf prices, then explicitly **Open the shop**. Prices lock while trading; unreasonable prices can lose purchases.
3. Invite each of the three customers. Only shelf stock is available. Add the displayed basket prices yourself, agree the total, and count change from the customer's payment. Click/hold pouches or keys **1 / 2 / 3** for 1/10/100; click tray stacks or **Shift + 1 / 2 / 3** to take coins back. Confirm the sale when ready. Corrections and leaving/returning preserve the transaction; waiting has no penalty.
4. **Close shop** from the ledger after the visits, or close early and forgo unstarted visits. An existing customer finishes before the evening starts. The 12-bit shop expense is charged once.
5. Review cash, takings, expenses and debt. Change tomorrow's prices and order bottles at supplier costs of 5/9/14 bits for tonic/ink/starlight. Pay the order once, including an explicit zero-bottle order if desired. Unsold stock stays and occupies its existing spaces. The order must fit the delivery tray.
6. Put down any carried bottle, then sleep. The paid shipment arrives once on day two and a morning checkpoint is saved. Stock, open, serve and close again, then finish the two-day experiment.

If funds are low on evening one, a **one-time recovery loan** provides 112 bits with 118 bits total debt. Repayment is voluntary, with no compounding or late fee. Terms are visible before accepting. This funds roughly two ordinary six-bottle deliveries; the values are prototype tuning, not a finished economy.

WASD/arrows move, mouse looks, **E** handles stock or the ledger, and **Escape** returns from a ledger/planning screen or pauses the shop. Look sensitivity is in the opening/pause menu.

## Morning saves

A checkpoint is created at the start of the first morning and after day-two deliveries, under Godot's per-project `user://morning.json`. Reopening the game or choosing **Restart saved morning** restores that morning's starting stock, prices, cash and debt. Preparation changes and mid-day transactions are deliberately not saved. Loading replaces the world; it does not repeat deliveries or loan payments. Invalid saves are reported and preserved until you explicitly choose a fresh experiment. Starting fresh replaces the checkpoint after confirmation.

## Project documents

- [Story strategy and connected examples](docs/story-strategy.md)
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

The check runner imports the project, runs shared item actions, the physical scene regression, checkout regression, day-loop/save/recovery regression, and scene smoke. It rejects engine errors even when Godot exits zero. Logs are generated under ignored `test-results/`.

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

Run the complete day-loop regression with screenshots:

```bash
.tools/godot/Godot_v4.7.2-stable_linux.x86_64 --audio-driver Dummy --fixed-fps 60 --path . --script res://tests/day_loop_test.gd -- --capture-days
```

Tests use isolated checkpoint files and do not modify the player's saved morning.

## Export

`bash tools/build-linux.sh` runs validation, installs checksum-verified official templates, exports Windows/Linux, checks the Linux release, and packages ZIPs with licence notices and SHA256 sidecars under ignored `builds/`. Native Windows execution and human usability of the new day/economy screens remain unverified here. No additional services or credentials are required.
