# Fantasy Shopkeeper

A cosy first-person fantasy shop game about brewing, stocking and serving customers by hand.

The Unreal 5.8.4 production project is under `unreal/FantasyShopkeeper`. Its C++ shop loop includes two recipes, physical stock, walking customers, manual checkout and paid ingredient replenishment, with an original modelled interior. The Godot project remains the earlier behavioural reference. Read the [game design document](docs/game-design-document.md), [engine decision](docs/engine-decision.md), [art direction](docs/art-direction.md) and [content policy](docs/content-provenance.md).

## The Lantern Walk MVP

The startup menu introduces the shop and explains morning checkpoints. **F1** pauses/resumes through a mouse-operated menu with help, settings, requests, credits and confirmed restart/new-game/quit actions. **F10** retains the quick day-recovery controls. Sensitivity, master volume and High/Epic graphics settings persist separately from shop progress. Fresh installations default to High; existing graphics choices are retained. Quitting does not save the current day; rest to commit the next morning.

Three regulars visit: **Ivo**, the courier (Dawn Tonic), **Fen**, the lamplighter (Moonlit Ink), and **Nessa**, the printer (Dawn Tonic). Their badges, requests and return lines distinguish them. On day one, Fen's note in **Requests & supplier account** offers a voluntary paid collection of one Moonlit Ink on day two. Open it from the supplier book, the menu, or E on Fen. Accepting does not reserve stock or charge money. A real 18-bit sale to Fen on day two lights a lantern beside the entrance. Declining or missing the collection causes no fine or loss of ordinary trade.

When evening supplies are short and cash is below 12 bits, the supplier book's account page can order **6 Moonleaf + 6 Water for next morning**, with **18 bits owed**, no interest and no deadline. One loan may be outstanding; repayment is an explicit action at the book in the evening. Credit adds ingredients to the next parcel, never spendable cash. The account and story persist together with the morning checkpoint (schema 5; older mornings remain compatible).

After evening three, the chapter summary recognises the shop's progress and Fen's outcome. Continue trading for as many days as you like. Order and rest to save that evening's progress. Workers, gathering, additional premises and mid-day saving remain outside this MVP.

## Unreal shop checkpoint

Open `unreal/FantasyShopkeeper/FantasyShopkeeper.uproject`. The startup map is `L_Shop`. Aim the centre dot at a bottle and press **E** within two metres to pick it up. Aim anywhere on a shelf section and press **E** to stock its next free position. Each section has **eight positions** and locks to its first stocked product until empty. With empty hands, **E** retrieves the first stocked bottle. The section's label shows its product and count.

With an item in hand, aim at a clear, flat floor or table within two metres and press **G** to set it down. A blocked placement keeps it in hand. E retrieves the loose item again. This also lets you free a hand when shelves are full. Items placed loosely can be moved onto sale slots for customers to collect.

The brewing workbench has a brass-and-ceramic vessel between two ingredient containers, with a circular output marker alongside. With empty hands, aim at the vessel and press **E** to brew the selected recipe. Press **R** while aiming at it to switch recipes. **Dawn Tonic** uses **two Moonleaf and one Spring Water**. The blue **Moonlit Ink** uses **one Moonleaf and two Spring Water**. The station begins with six Moonleaf and six Spring Water. Existing morning checkpoints retain their saved quantities; use **F10 → F → Enter** for a fresh day 1 with the revised supplies. The shop starts in preparation, or restores its saved morning checkpoint.

Collect the finished potion from the output marker and shelve it using the same E interaction. An occupied output, insufficient ingredients or full hands block brewing without spending ingredients. The vessel prompt shows the current stock and recipe cost. Hands-on checks are to assess aiming, readable feedback, collection and repeated batches.

Edit recipes under `/Game/Shop/Recipes`. The placed `BrewingStation` exposes `Recipe`, `Available Recipes`, `Starting Ingredients`, `Ingredient Capacity` and `Output Slot`. Its Blueprint child is `/Game/Shop/Native/BP_BrewingStation`. `ShopBrewingStation` validates and commits batches, while `ShopBrewingRecipe` contains ingredient costs, output type and brewing sound. Produced bottles acquire ordinary slot ownership through the shared item placement code.

The first mannequin customer wants **Dawn Tonic for 12 bits** and tenders **20 bits**. After checkout, they carry their purchase out. The next arrival wants **Moonlit Ink for 18 bits**, tendering **50 bits**. The authored orders alternate within a finite day (three visits by default). The HUD shows the current request. Customers walk to an accessible sale position, pick up the physical bottle through the shared interaction component and carry it back to checkout. The shelf becomes free at collection. Customers select matching products from sale slots.

Customers offer a brief request as they arrive and a short reaction after a successful purchase. Nearby subtitles fade in and out without capturing input or adding a floating speech board. Three phrasing styles vary the request pause, reaction pause and departure timing slightly; selections are stable for the same day and visit after restoring a morning. The ordinary request/status remains available after the spoken line expires. Rejected payments and unserved exits do not trigger purchase thanks. There are no voices, dialogue choices, waiting penalties or new economic effects.

When the customer returns to checkout, they put their actual bottle on the goods mat and their payment coins in the dish. There is **no total-entry or accept-payment step**. The small ledger records the item price and a slip records the tender; work out the change yourself. Goods stay reserved on the counter until the sale is complete. The bottle travels from the shopper's hand to the mat, settles with a soft placement sound, and payment coins drop into the dish with staggered clinks. These are controlled visual gestures; the shared ownership transfer and guarded sale remain authoritative.

Aim at the copper, silver or brass stack and press **E** to put a **1 / 10 / 100-bit** coin in the change mat. Aim at the matching mat lane and press **E** to take one back. Keyboard **1 / 2 / 3** also add those denominations while looking at the till, and **Shift** removes them. The mat shows counted coins and a written total. Dawn Tonic needs **8 bits** change. Moonlit Ink needs **32 bits**. Press **E** on the **PAID** stamp once the change is exact. A flat ledger records the price and feedback; small counter slips show tender and counted change. Change coins briefly settle onto the mat, with distinct pitches for each denomination and a lower return tone. Incorrect change produces a short ledger note without revealing the answer. A successful sale presses the PAID stamp, sounds a soft thunk and confirmation chime, and leaves a green PAID receipt. Movement and mouse look stay active throughout checkout, and looking away preserves the pending transaction.

Aim at the physical **supplier book** and press **E** with empty hands to open the ingredient-ordering page. It shows stored ingredients, the next trading day's recipe requirements, paid deliveries, capacity, the projected stock after the selected order and any remaining shortfall. The forecast follows the configured visitor sequence and deducts matching bottles already on sale shelves. Unstocked brewing output is not counted. Missing recipes are reported as an unavailable forecast.

Use the page's buttons or **R** to change pack, **F / T** to decrease/increase quantity (0-10), and **Enter** to sign and pay. **F10** closes the book. Packs remain **six Moonleaf plus three Water for 12 bits**, **six Moonleaf for 6 bits**, or **three Water for 6 bits**. Reading the book never spends money. Orders can be paid **in the evening only**; at other times the page is read-only. Quantity zero confirms no additional supplies. After payment, changing pack or quantity allows another deliberate order; repeating confirmation cannot charge again. Pending orders must fit ingredient storage and the purse must cover the cost. The physical PACK / LESS / MORE / REST tabs remain available.

Paid orders arrive together in a sealed crate by the entrance **next morning**, and **E** unpacks them before opening the shop. No same-day replenishment is added. The unopened delivery, cash and stored ingredients are checkpointed together; restoring cannot replay a payment or duplicate ingredients. A failed morning write retains the paid order for retry. Farming and foraging remain future work. Prices and quantities are provisional tuning.


`CustomerFlow` admits up to three concurrent visitors in the production shop, the entrance/exit waypoint, counter-relative checkout destination, visit interval and authored `Orders`. The default `Checkout Follows Till` keeps arrivals aligned when the counter moves; disable it to use the flow's authored `Checkout` waypoint. Each order can specify its customer Blueprint, product, price and tender. Customers browse concurrently and queue in arrival order at counter-relative positions. Only the front customer can present payment; the next advances after the paid customer carries their bottle out. Closing stops new arrivals and finishes everyone already inside. There are no waiting penalties. The new request resets the old till receipt. Manny and Quinn currently provide the two customer presentations through the existing unarmed animation Blueprint.

`ShopCustomer` uses Character Movement, an AI controller, dynamic navigation and the same `ShopInteractionComponent` and item pickup operation as the player. Acceleration-driven path following supplies the animation Blueprint's movement signal, and the mesh keeps its pose and hand socket updated offscreen. `ShopGameMode` builds missing navigation tiles at startup when a newly imported map has bounds without tile data. The current presentation is `/Game/Shop/Native/BP_CustomerCharacter`, using the project's existing Manny mannequin and unarmed animation Blueprint. The carry anchor follows the right hand with the bottle neck near the palm. Detailed reach, grasp and finger animation is future presentation work. Shelf sections have `Available For Sale` enabled by default. Add `ShopShelfSection` actors to give furniture another eight-position stock area. Brewing output slots are marked unavailable for sale. Unreachable or removed stock is retried without teleporting or charging money.

`ShopTill` snapshots the waiting customer's item, price and tender, moves the actual item into a reserved counter slot, validates counted change, and retains the pending checkout state. `ShopTillControl` provides reachable world-space coin stacks, mat lanes, a PAID stamp and a payment dish. `ShopGameState` commits item removal and net payment through one guarded transaction. Edit `Price Bits` and `Tender Bits` on the customer, and assign that customer to the till's `Customer` property. The till rejects stale orders, invalid tender, remote confirmation and repeated payment. The map before till installation is backed up under `Saved/BeforeTill`.

Gameplay lives in `Source/FantasyShopkeeper`. `ShopInteractionComponent` performs the trace and tracks the held item. `ShopPickupItem` owns attachment, collision and item transfers. `ShopShelfSection` owns eight sequential stock positions and enforces the product lock through the underlying `ShopShelfSlot` placement API. `ShopPlayerController` binds E through Enhanced Input and activates the template's movement and separate mouse-look mappings. `ShopGameMode` uses the existing first-person Blueprint character.

Edit visuals through the Blueprint children in `/Game/Shop/Native`. The original Blueprint prototypes are retained in `/Game/Shop`. The map before migration is backed up locally under `Saved/BeforeNativeInteraction`.

The modelled art kit adds a timber brewing bench, sage-backed shelving, brass-and-ceramic brewer, Moonleaf jar, water jug, mortar and pendant lamp, under the `ShopArt` Outliner folder. Four eight-position sections fit the apothecary shelving, providing 32 stock positions. The two retail tables and their former six stock sections have been removed. Dawn Tonic uses the recipe-specific `BP_DawnTonic` presentation child, with a flattened tapered cork sized to fit the neck. Editable Blender source, FBX exports, studio and cork-fit previews, and regeneration instructions live in [art/shop-corner](art/shop-corner/README.md). The pre-art map is backed up locally under `Saved/BeforeShopArt`, and the map before the mannequin migration is under `Saved/BeforeCustomerCharacter`. The room has plaster and timber walls, transparent arched windows with garden views, and four visible pendant lights.

Run the build and forty-nine interaction, brewing, customer, supply, till, day and persistence regressions from the repository root when Unreal Editor is closed:

```powershell
./unreal/FantasyShopkeeper/Tools/check.ps1
```

The runner disables distributed builds, network discovery and the separate trace server, and keeps Zen on loopback. It also supports `-Action Rendered`, `Assets`, `Build`, `Package` and `Editor`, sharing those settings. See [local tooling and the remaining built-in trace listener](docs/local-tooling.md).

The runner checks the report as well as the process exit code and reports passing tests that logged warnings. The actual-map keyboard regression opens and closes through the sign and completes three sales, evening ordering and next-morning delivery, including both recipes, R selection, G set-down/retrieval, natural walking departure and arrival, supply purchases, checkout controls, uninterrupted mouse look and the finite customer sequence. Focused checks cover ownership, reach, occlusion, occupied slots, blocked set-down surfaces, preserved scale, destruction cleanup, exact ingredient costs, malformed recipes, changed recipes during spawning, rollback, walking animation, purchased-item presentation, rejected and reentrant payments, cash overflow, mixed change, pending checkout state, supply capacity and delivery overflow, malformed orders and interrupted visitor spawning. The keyboard fixture isolates desktop events and verifies physical coin dispensing, guarded confirmation and uninterrupted look input. Focused tests cover eight-slot capacity, sequential filling, product rejection, reassignment after emptying and shared customer pickup. A further regression combines bundle and water orders over three consecutive trading days through morning four, verifies delivery restoration and sells all three daily products without typing a total. Reports are under the Unreal project's ignored `Saved/Automation` folder. Pass `-EngineRoot` if the engine is installed elsewhere.

The seven maintained Python tools regenerate models and audio, import assets, install the current shop layout, validate content and render review images. Five completed one-off migration scripts have been removed. See [asset regeneration](art/shop-corner/README.md#regeneration-and-verification) for the supported order.

In the evening, aim at the **extra drawers cabinet** beside the supplier and press **E** with empty hands to buy it for **24 bits**. It doubles storage from **30 to 60 of each ingredient**, allowing larger orders without adding ingredients. Extra drawers appear immediately, and the order book records upgrade spending separately. The purchase becomes permanent at the next successful morning checkpoint; restoring an earlier morning rolls back both the drawers and their cost.

Buy the **batch bottling kit** at the front of the brewing bench with **E in the evening**, for **36 bits**. Its brass rack adds two bottle positions, allowing **three bottles per brew** at the normal ingredient cost per bottle. After purchase, E on the kit's selector plaque switches between **one and three**; E on the vessel brews, and R still changes recipes. Collect every finished bottle with the ordinary pickup interaction before brewing again. Insufficient ingredients or any occupied output blocks the entire batch without spending ingredients. Purchase, selection and finished output positions persist at the next morning checkpoint.

## Unreal shop days and morning checkpoints

Aim at the wooden **CLOSED** sign beside the entrance and press **E** with empty hands to open. Brewing and stocking remain available throughout the day; supplies must be ordered the previous evening. `CustomerFlow.VisitsPerDay` defaults to three; its `Orders` list supplies the repeating product sequence within that allocation. There is no time limit or waiting penalty.

Use **E** on the **OPEN** sign while trading to close early and skip unstarted visits. Everyone already inside must finish and walk out before evening. Press **X** to end a visit without a sale before the customer picks up a bottle, useful when stock runs out. Once they carry stock, finish checkout. Serving the full allocation also reaches evening automatically. The order book shows takings (net sale prices), supply spending, upgrade spending and current closing cash. No daily expense or loan has been added. Evening dims the sky and window light over four seconds while the pendants remain lit; this follows shop phases, not a timed deadline.

Confirm a supplier order (including a zero order), then use **E** on the book's **REST** tab with empty hands. **N** remains a keyboard shortcut. The shop fades to black, a short original wind/cricket cue marks the night, then morning fades in with the paid delivery by the entrance. Movement and interaction pause during the 3.5-second night; failed saving returns to evening with the order intact. Cash, purchased upgrades, selected batch size, ingredient quantities, exact shelf positions, brewing output and loose bottles carry into the next morning. The successful transition saves once and resets daily totals. A failed write keeps the current evening available for retry.

Press **F10** for recovery options. **Enter** restarts the saved morning, rolling back today's cash, brewing, stock, orders and upgrades together. To begin again on **day 1**, press **F10**, then **F** for **New game**, then **Enter** to confirm. F10 cancels either confirmation. Escape remains Unreal Editor's stop-play shortcut. A new game restores the authored starting shop and saves it immediately; the old morning is preserved beside `Morning.sav` as `Morning.sav.before-new-game-<id>.bak`. Restarting a morning does not add resources that were already missing when that morning began.

Reopening automatically restores `Saved/SaveGames/Morning.sav` under the Unreal project (or the packaged game's saved-data directory). Changes made after that morning—including sales, brews, upgrades and supply purchases—roll back together. This is a morning checkpoint, not a mid-day save. The initial morning is saved on first launch. Automation uses isolated files and never reads or writes the player's checkpoint.

Version 5 checkpoints also store the lantern story and supplier debt alongside storage and batch upgrades, selected batch size and every finished bottle. Versions 1–4 remain readable with their existing entitlements. Checkpoints have a schema version and checksum, validate products, ingredients and destinations, and replace the previous file atomically on Windows. Invalid or incompatible files are reported in the HUD and preserved; further saving is blocked. Use the confirmed Start a new shop menu action to begin again and retain a backup of the previous morning. Renaming shelf sections or stations can invalidate existing checkpoints; migration of authored map identities is future work.

## Godot prototype reference

**The Lantern Walk** has stocking and manual checkout inside preparation, trading, closing and evening planning. Set prices, buy the next delivery, manage cash and a simple recovery loan, and resume from morning checkpoints. Fen has an optional two-day encounter within the existing visits. Customers arrive one at a time. Queues, workers and further stories are future work.

## Play this checkpoint

1. Continue the saved morning, or confirm **Start a fresh two-day experiment** from the opening/pause menu. A fresh shop starts with 24 bits and six already-paid delivery bottles. The menu says **THE LANTERN WALK**.
2. Stock shelves at your pace. Approach the counter, aim at the ledger and press **E**. During preparation you can set shelf prices, then explicitly **Open the shop**. Prices lock while trading; unreasonable prices can lose purchases.
3. Invite each of the three customers. Only shelf stock is available. Add the displayed basket prices yourself, agree the total, and count change from the customer's payment. Click/hold pouches or keys **1 / 2 / 3** for 1/10/100; click tray stacks or **Shift + 1 / 2 / 3** to take coins back. Confirm the sale when ready. Corrections and leaving/returning preserve the transaction; waiting has no penalty.
4. **Close shop** from the ledger after the visits, or close early and forgo unstarted visits. An existing customer finishes before the evening starts. The 12-bit shop expense is charged once.
5. Review cash, takings, expenses and debt. Change tomorrow's prices and order bottles at supplier costs of 5/9/14 bits for tonic/ink/starlight. Pay the order once, including an explicit zero-bottle order if desired. Unsold stock stays and occupies its existing spaces. The order must fit the delivery tray.
6. Put down any carried bottle, then sleep. The paid shipment arrives once on day two and a morning checkpoint is saved. Stock, open, serve and close again, then finish the two-day experiment.

If funds are low on evening one, a **one-time recovery loan** provides 112 bits with 118 bits total debt. Repayment is voluntary, with no compounding or late fee. Terms are visible before accepting. This funds roughly two ordinary six-bottle deliveries; the values are prototype tuning, not a finished economy.

WASD/arrows move, mouse looks, **E** handles stock or the ledger, and **Escape** returns from a ledger/planning screen or pauses the shop. Look sensitivity is in the opening/pause menu.

## Fen and the Lantern Walk

For the full encounter, start a fresh run or resume a day-one morning. Older day-two saves remain playable but do not invent a promise you never made.

Fen is the second visitor on both days, with a name label and the usual ink/starlight basket. On day one a small optional question appears in the ledger. You can keep trading, read it, promise to try, defer until evening planning, or decline. There are no automatic story popups. A deferred note remains in the ledger until bedtime.

If you promise, have bottled starlight on a shelf for Fen's second-visitor return on day two. It is an ordinary paid purchase at your shelf price, with the usual price rejection; nothing is donated or reserved automatically. Only completing the correct total and change fulfills it. Fen then walks to the lamp by the shop entrance and tests the light.

Fen offers one spare dawn tonic as a thank-you. Accept it through their ledger note when a delivery space is free. A full tray leaves it pending safely; collecting it twice is impossible. Ignoring, declining or missing the promise leaves ordinary customer opportunities and finances intact, with a gentle bakery-lantern fallback instead of a fine.

Morning saves retain the promise. Restarting day two rolls the story outcome, claimed gift, stock and money back together to that morning. The lamp is decoration, not another bottle of saleable stock. Nessa and Ivo's proposed stories are not implemented yet.

## Morning saves

A checkpoint is created at the start of the first morning and after day-two deliveries, under Godot's per-project `user://morning.json`. Reopening the game or choosing **Restart saved morning** restores that morning's starting stock, prices, cash and debt. Preparation changes and mid-day transactions are deliberately not saved. Loading replaces the world; it does not repeat deliveries or loan payments. Invalid saves are reported and preserved until you explicitly choose a fresh experiment. Starting fresh replaces the checkpoint after confirmation.

## Project documents

- [Unreal version, version control and production roadmap](docs/unreal-roadmap.md)
- [Production game design document](docs/game-design-document.md)
- [Engine decision and evaluation plan](docs/engine-decision.md)
- [Selected art direction and references](docs/art-direction.md)
- [AI assistance and content provenance](docs/content-provenance.md)

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

The check runner imports the project, runs shared item actions, the physical scene regression, checkout regression, day-loop/save/recovery regression, Fen encounter regression, and scene smoke. It rejects engine errors even when Godot exits zero. Logs are generated under ignored `test-results/`.

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

To render the encounter regression and its screenshots:

```bash
.tools/godot/Godot_v4.7.2-stable_linux.x86_64 --audio-driver Dummy --fixed-fps 60 --path . --script res://tests/fen_story_test.gd -- --capture-fen
```

## Export

`bash tools/build-linux.sh` runs validation, installs checksum-verified official templates, exports Windows/Linux, checks the Linux release, and packages ZIPs with licence notices and SHA256 sidecars under ignored `builds/`. Native Windows execution and human usability of the new encounter remain unverified here. No additional services or credentials are required.

### Three-day MVP route

The supplier page lists tomorrow's product mix and potential sales as well as ingredient needs. Rest checks stored ingredients, sale-shelf bottles and **paid** next-morning orders. An unsigned selection does not count. If supplies are short, return to planning with F10 or explicitly accept with Enter; no money is spent by this choice. Unstocked output and carried bottles must be placed on sale shelves to count toward the plan.

Each day currently requests two Dawn Tonics and one Moonlit Ink: five Moonleaf and four Water, earning 42 bits. One viable fresh-shop route is:

| Evening | Purchases | Cash after purchases | Ingredients next morning, after unpacking |
| --- | --- | ---: | --- |
| Day 1 | Storage drawers (24), bundle (12) | 6 | 7 Moonleaf / 5 Water |
| Day 2 | Batch kit (36), bundle (12) | 0 | 8 Moonleaf / 4 Water |
| Day 3 | Bundle (12), Water pack (6) | 24 | 9 Moonleaf / 6 Water |

Select **one bottle** on the upgraded kit for day three's mixed demand. The kit increases throughput, not ingredient yield; three-bottle batches can consume ingredients needed by another recipe. These purchases are optional. The milestone keeps two recipes, three visits per day and the existing prices; workers and additional content remain outside this slice.
