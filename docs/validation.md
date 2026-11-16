# First-day supplies and supplier ordering page — 2026-11-16

The native station default and authored `L_Shop` station now start with six Moonleaf and six Spring Water. Two Dawn Tonics and one Moonlit Ink consume five leaf and four water. Existing checkpoints retain their saved inventory; a fresh game uses the revised starting quantities. The maintained expansion installer writes these defaults and the asset validator checks them.

E on the supplier book opens a native Slate ordering page. It shows stored ingredients, forecast requirements after deducting matching sale-shelf bottles, paid deliveries, capacity, projected stock and remaining shortfall. Unknown recipes produce an unavailable forecast rather than partial totals. The page has mouse buttons and R/F/T quantity/pack controls, Enter to sign/pay and F10 to close. Reading is allowed outside evening; purchasing remains evening-only. Opening never spends money, and paying twice cannot repeat an order. Further deliberate selections still support multiple orders. Player movement/look are held while reading and restored on close; invalid reach, held items or overnight transition close the page.

The page reuses the existing guarded purchase and sealed next-morning delivery mechanisms. No recipe discounts, same-day replenishment, farming, save-schema changes or new tool scripts were added.

- Local build/gameplay: **44 passed, 0 automation warnings**, `Saved/Automation/Gameplay-20261116-201616-989`; log `Saved/Logs/OrderingChecks.log`. New coverage verifies the complete first-day recipe budget, shelf-adjusted forecasts and unknown recipes. The actual-map keyboard workflow checks six-of-each initialization, read-only preparation access, opening without payment, one-time confirmation, input restoration and morning unpacking.
- Rendered workflow: **1 passed, 0 automation warnings**, `Saved/Automation/Rendered-20261116-202107-543`; log `Saved/Logs/OrderingFinalRendered.log`. The final test additionally sends quantity keys through the focused Slate page, rather than only controller injection. Inspected the page in `Saved/ArtReview/20261116-201752-014/ordering.png` from the initial rendered pass; the capture includes editor chrome because Slate UI requires a UI-inclusive screenshot.
- Asset checks: **34 meshes, 87 collision hulls, four sections / 32 positions, two windows and four pendants; 0 errors and 0 warnings**, log `Saved/Logs/Assets-20261116-201724-949.log`. All seven maintained Python tools parse. Installer log `Saved/Logs/Editor-20261116-201516-267.log` records no removed furniture and a complete three-point entrance route. The previous map is backed up under `Saved/BeforeShopExpansion/20261116-201521`.
- Windows package: **passed**, `builds/unreal-ingredient-orders/Windows/FantasyShopkeeper.exe`; log `Saved/Logs/OrderingPackage.log`. Isolated `Saved/IngredientOrdersSmoke` startup/reopening preserved checkpoint hashes/timestamps without warnings/errors. Existing compiler preferred-version and engine-header deprecation notices remain build-only.

All checks used the local tooling. Player saves were not touched. Manual assessment of mouse handling and menu feel remains with the user.

# Recovery shortcut in Play-in-Editor — 2026-11-16

Changed the recovery menu toggle and cancellation from Escape to F10. Unreal Editor reserves Escape for Stop Play Session before the game controller receives it. F10 opens recovery options; F then Enter starts a fresh day 1, Enter alone restarts the morning, and F10 cancels. The editor's Escape binding is unchanged.

The previous controller-injected workflow did not exercise editor interception. The rendered workflow now checks `FPlayWorldCommands::GlobalPlayWorldActions->ProcessCommandBindings` for F10 during active PIE before invoking the game's input, then verifies confirmation and cancellation. Unreal's F10 Step Over command is gated to debugger breakpoints; it does not consume the key during ordinary play. Slate dependencies are editor-only for this regression.

- Local gameplay: **43 passed, 0 automation warnings**, `Saved/Automation/Gameplay-20261116-194920-925`; log `Saved/Logs/RecoveryKeyChecks.log`.
- Rendered workflow with editor-routing regression: **1 passed, 0 automation warnings**, `Saved/Automation/Rendered-20261116-195250-652`; log `Saved/Logs/RecoveryKeyRouting.log`. The F10 confirmation wording was also inspected in `Saved/ArtReview/20261116-195057-036/restart.png` from the initial rendered pass.
- Local Windows package: **passed**, `builds/unreal-recovery-key/Windows/FantasyShopkeeper.exe`; log `Saved/Logs/RecoveryKeyPackage.log`. Isolated `Saved/RecoveryKeySmoke` startup/reopening preserved the checkpoint hash and timestamp without warnings/errors. No player saves or editor shortcut preferences were modified.

# Day restart and fresh day-one recovery — 2026-11-16

Esc opens recovery options. Enter restarts the current saved morning. F switches to a separate fresh-game confirmation, where Enter starts day 1 and Esc cancels. Movement, look and trading inputs are blocked while confirming; key releases still reach Enhanced Input. Recovery is unavailable during the overnight transition.

The game captures the authored initial morning before loading an existing checkpoint. A confirmed new game backs up the current save to `Morning.sav.before-new-game-<id>.bak`, atomically writes that initial morning, then uses the existing restoration path. Cash, ingredients, stock, paid deliveries and upgrades reset together. Failed writes leave the current run intact; failed restoration attempts recover the previous disk checkpoint from its retained backup. Restarting only the current morning does not create extra resources.

- Local build/gameplay: **43 passed, 0 automation warnings**, `Saved/Automation/Gameplay-20261116-194308-455`; log `Saved/Logs/FreshGameChecks.log`. Tests cover confirmation/cancellation, blocked trading shortcuts, held-item cleanup, cash/ingredient/upgrade rollback, repeat restarts, invalid saves, a resource-empty day 4 returning to day 1, byte-identical backup preservation and a failed write followed by a successful retry.
- Rendered keyboard workflow: **1 passed, 0 automation warnings**, `Saved/Automation/Rendered-20261116-194459-980`; log `Saved/Logs/FreshGameRendered.log`. It opens the new-game confirmation, captures `Saved/ArtReview/20261116-194459-980/restart.png`, cancels, verifies input recovery and completes the ordinary evening/morning workflow.
- Windows package: **passed**, `builds/unreal-day-recovery/Windows/FantasyShopkeeper.exe`; log `Saved/Logs/FreshGamePackage.log`. Automatic startup/reopening used isolated `Saved/DayRecoverySmoke` data, preserved the morning checkpoint hash and timestamp, and logged no warnings/errors. Existing compiler preferred-version and engine-header deprecation notices remain build-only.

All runs used the local tooling. No player checkpoint, saved map, generated art or maintained tool script was changed by verification. Manual playtesting remains with the user.

# Purchasable batch bottling kit — 2026-11-16

The bench-front plaque offers a 36-bit evening purchase. After purchase, E on the plaque selects one or three bottles per brew; E on the vessel brews and R still changes recipes. A brass rack and two additional non-sale output positions appear. All finished outputs must be collected before another brew. The ordinary pickup and stocking operations remain authoritative.

A batch snapshots its recipe and destinations, stages all bottles, acquires every output slot, then charges the authored per-bottle ingredient costs once. A failed spawn or placement destroys the whole staged batch without spending ingredients. Version 4 morning checkpoints store the upgrade and selection alongside cash, ingredients and every output bottle. Versions 1-3 retain single brewing and their existing storage entitlement. Invalid batch selections or stock in an unpurchased rack are rejected without changing the live shop.

- Local build and gameplay: **41 passed, 0 automation warnings**, report `Saved/Automation/Gameplay-20261116-192255-753`; log `Saved/Logs/BatchFinalChecks.log`. New tests cover physical purchase/selector, insufficient funds, full hands/reach, duplicate debits, both recipes, scaled-cost overflow, failure on the second bottle, reentrancy, partial pickup, single mode, save-write retry, invalid/legacy saves and repeated restoration without duplicate stock or charges. The first pickup fixture approached its second slot through the first; moving the fixture to each bottle's front resolved the occlusion without changing gameplay reach rules.
- Rendered keyboard workflow: **1 passed, 0 automation warnings**, report `Saved/Automation/Rendered-20261116-192428-976`; log `Saved/Logs/BatchRendered.log`. Inspected `Saved/ArtReview/20261116-192428-976/batch.png`: the wooden offer plaque is readable on the bench front, below the vessel. This capture shows the unpurchased offer; batch output and pickup are covered by focused automation.
- Asset checks: **34 meshes, 87 collision hulls, four sections / 32 positions, two windows and four pendants; 0 errors and 0 warnings**, log `Saved/Logs/Assets-20261116-192221-762.log`. All seven maintained Python tools parse. No generated assets, map changes or new scripts were required.
- Windows package: **passed**, `builds/unreal-batch-brewing/Windows/FantasyShopkeeper.exe`; log `Saved/Logs/BatchPackage.log`. Automatic startup/reopening used isolated `Saved/BatchBrewingSmoke` data and preserved checkpoint hashes/timestamps with no log warnings/errors. A separate startup used a copy of the previous packaged build's version 3 checkpoint; legacy state restoration is asserted by the gameplay suite. Existing compiler preferred-version and engine-header deprecation notices remain build-only.

All checks used the local tooling. Manual assessment of batch handling and the provisional 36-bit price remains with the user.

# Purchasable ingredient storage — 2026-11-16

A physical cabinet beside the supplier offers one evening upgrade for 24 bits. E with empty hands doubles each ingredient limit from 30 to 60 and reveals upper drawers. The guarded transaction rejects duplicate purchases, insufficient funds, remote interaction, full hands and overflowing capacities. The order book distinguishes upgrade spending from supplies. Buying capacity adds no ingredients.

Morning schema 3 stores each station's upgrade alongside cash, stock and sealed deliveries. Restoring rolls these back together; invalid saves leave the live state intact. Schemas 1/2 retain base capacity. A failed morning write retains the paid upgrade and order for retry. No maps, generated assets or additional scripts were needed.

- Local build and gameplay: **38 passed, 0 automation warnings**, report `Saved/Automation/Gameplay-20261116-182308-483`; log `Saved/Logs/StorageFinalChecks.log`. New coverage exercises physical purchase, daily totals, rollback, larger orders, failed writes, repeated restoration/unpacking, invalid capacity and legacy saves.
- Rendered keyboard workflow: **1 passed, 0 automation warnings**, report `Saved/Automation/Rendered-20261116-182441-334`; log `Saved/Logs/StorageFinalRendered.log`. The evening cabinet offer is captured in `Saved/ArtReview/20261116-182441-334/storage.png`. A first capture caught a drawer pull covering the label; the final model leaves that area clear. The rendered workflow shows the unpurchased cabinet; focused tests cover purchased drawer visibility and its restoration.
- Asset checks: **34 meshes, 87 collision hulls, four sections / 32 positions, two windows and four pendants; 0 errors and 0 warnings**, log `Saved/Logs/Assets-20261116-182121-580.log`. All seven maintained Python tools parse.
- Windows package: **passed**, `builds/unreal-storage-upgrade/Windows/FantasyShopkeeper.exe`; log `Saved/Logs/StorageFinalPackage.log`. Automatic startup/reopening used isolated `Saved/StorageUpgradeSmoke` data, preserved the checkpoint hash and timestamp, and logged no warnings/errors. Existing compiler preferred-version and engine-header deprecation notices remain build-only.

All checks used the local tooling. Manual assessment of the purchase's feel and economic balance remains with the user.

# Customer requests and purchase reactions — 2026-11-16

Customers now offer a brief product-specific request as they arrive and a short reaction after a committed purchase. Three authored phrasing styles use small greeting/reaction delays, four-to-five-second subtitle lifetimes and departure delays scaled between 0.85 and 1.15 of the authored value. The day and visit select the variation deterministically, without changing the global random stream or save schema. Explicit zero departure delay still works for tests and authored instant departures.

Subtitles appear near the bottom of the screen within nine metres, fade in/out and never capture input. There are no floating speech boards, voices, dialogue choices, new rewards or impatience penalties. Existing status/request text remains available. Failed checkout and unserved exits do not trigger purchase thanks; route retries and repeated confirmation do not repeat dialogue. An unserved customer's interaction prompt now says they are heading home instead of thanking the player for a purchase.

- Local build/gameplay: **36 passed, 0 automation warnings**, report `Saved/Automation/Gameplay-20261116-180608-937`; log `Saved/Logs/CustomerPersonalityChecks.log`. New checks cover delayed requests, correct product names, expiry, three variants, stable visit selection, bounded timing, successful-sale gating, repeated confirmations and unserved exits. Existing transaction, physical checkout, multi-day and save/load tests pass.
- Rendered workflow: **1 passed, 0 automation warnings**, report `Saved/Automation/Rendered-20261116-180804-412`; log `Saved/Logs/CustomerPersonalityRendered.log`. Inspected `Saved/ArtReview/20261116-180804-412/paid.png`: the purchase reaction is readable below checkout without obscuring its goods or controls, and the customer retains their purchased bottle.
- Asset checks: **34 meshes, 87 collision hulls, four sections / 32 positions, two windows and four pendants; 0 errors and 0 warnings**, log `Saved/Logs/Assets-20261116-180817-137.log`. All seven maintained Python tools parse. No maps, assets, media or new scripts were required.
- Windows package: **passed**, `builds/unreal-customer-personality/Windows/FantasyShopkeeper.exe`; log `Saved/Logs/CustomerPersonalityPackage.log`. Automatic startup/reopening used isolated `Saved/CustomerPersonalitySmoke` data and preserved the checkpoint hash and timestamp; launch/restore logs have no warnings/errors. Builds retain the existing compiler preferred-version and engine-header deprecation notices.

All checks used the local tooling. Human assessment of dialogue tone and timing remains with the user.

# Tactile checkout presentation — 2026-11-16

Checkout now presents the carried bottle through a 0.45-second hand-to-mat motion and a placement sound, followed by staggered payment coin drops/clinks. A collision-free visual copy handles the gesture while the real item retains its reserved checkout-slot ownership. Completion or cancellation restores the real item's visibility. Change coins settle into controlled mat positions; denomination pitches, louder placement clinks and lower return tones reuse the existing original audio cues.

Manual change counting remains authoritative. Incorrect change writes a short ledger note without revealing the required amount. Successful payment produces a stamp gesture, soft thunk, confirmation chime and green PAID receipt; repeating confirmation cannot overwrite the successful receipt or repeat payment. Movement, mouse look, reach checks and guarded transactions remain intact. This is object animation, not bespoke hand/finger animation or free coin physics. No assets, maps or sound files were changed, and no scripts were added.

- Local build and gameplay: **34 passed, 0 automation warnings**, report `Saved/Automation/Gameplay-20261116-173237-488`; log `Saved/Logs/CheckoutPolishChecks.log`. Added checks cover animation progress, reserved ownership, single visibility, exactly-once tender clinks, cancellation, manual-change rejection, preserved paid feedback and the stamp returning to rest. Existing multi-day, save/load and transaction guards pass.
- Rendered keyboard workflow: **1 passed, 0 automation warnings**, report `Saved/Automation/Rendered-20261116-173813-788`; log `Saved/Logs/CheckoutPolishRendered.log`. Inspected settled counter and paid-receipt captures under `Saved/ArtReview/20261116-173813-788`. The extra receipt capture waits for its frame before restoring aim to the stamp for the repeated-confirmation check.
- Asset validation: **34 meshes, 87 collision hulls, four shelf sections / 32 positions, two window openings, four pendants; 0 errors and 0 warnings**, log `Saved/Logs/Assets-20261116-173346-369.log`. All seven maintained Python tools parse.
- Windows package: **passed**, `builds/unreal-checkout-polish/Windows/FantasyShopkeeper.exe`; log `Saved/Logs/CheckoutPolishPackage.log`. Automatic startup and reopening used isolated `Saved/CheckoutPolishSmoke` data and preserved the morning checkpoint hash and timestamp, with no launch/restore warnings or errors. Packaging recovered from a stale Zen lock and reported the existing compiler preferred-version notice; engine-header deprecation notices remain build-only.

All runs used the local build tooling. Audio event timing is covered by automation; perceived loudness, balance and hands-on feel remain for the user's playtest.

# Physical evening planning, night and morning deliveries — 2026-11-16

Implemented an order book on the supplier crate with tomorrow's visitor count, takings, supply spending, cash, stored ingredients, pending paid quantities and the selected order. Reachable PACK / LESS / MORE / REST tabs use E; R/F/T and N remain shortcuts. Orders still use the existing guarded debit and duplicate-confirmation protection. Evening totals live on the book, and supply prompts sit below the physical labels.

Rest fades to black over 1.4 seconds, holds through a 3.5-second night with an original procedural wind/cricket cue, then fades into morning. Movement, new interaction and ordering are blocked during the night; key releases still reach Enhanced Input. A failed checkpoint write returns to evening with the paid order intact. The night has no HUD or trading deadline.

Paid supplies appear as a closed, labelled crate beside the entrance. E with empty hands unpacks the complete order into ingredient storage once. Opening requires unpacking first. Morning save version 2 stores the unopened parcel separately from usable ingredients; restoring rolls both back together. Version 1 mornings remain readable with their existing stored ingredients and no invented parcel. This is still a morning checkpoint, not mid-day saving. Unpacking is one bulk transfer, not individual sack carrying or a bespoke hand animation.

- Local runner build and gameplay: **33 passed, 0 automation warnings**, report `Saved/Automation/Gameplay-20261116-133538-383`; log `Saved/Logs/PlanningFinal.log`. Includes physical book tabs/rest, transition guards, actual E unpacking, remote/repeated unpack rejection, malformed parcel data, older-save compatibility, save-write retry, repeated restoration, and three consecutive trading days. The multi-day fixture now reuses its flow's till instead of placing overlapping counters at the same location.
- Rendered workflow: **1 passed, 0 automation warnings**, report `Saved/Automation/Rendered-20261116-133641-476`; log `Saved/Logs/PlanningRenderedFinal.log`. Inspected `Saved/ArtReview/20261116-133641-476/{evening,night,morning}.png`: readable planning figures and tabs, unobstructed parcel label, black night frame without HUD, morning delivery and successful unpacking. Existing counter/sign checks also pass.
- Asset validation: **34 meshes, 87 hulls, four sections / 32 positions, two window openings, four pendants; 0 errors and 0 warnings**, `Saved/Logs/Assets-20261116-132720-943.log`. No saved map or asset changes were needed; native components reuse existing geometry and materials. All seven maintained Python tools parse; no scripts added.
- Windows package: **passed**, `builds/unreal-morning-deliveries/Windows/FantasyShopkeeper.exe`; log `Saved/Logs/PlanningPackageFinal.log`. Automatic startup and reopening used isolated `Saved/PlanningPackageSmoke` data and preserved the checkpoint hash and timestamp. Launch/restore logs contain no warnings or errors. The build reports the existing installed compiler preferred-version and engine-header deprecation notices.

All engine invocations used the local tooling settings. Manual playtesting, text comfort and audible mix remain with the user.

# Local execution tooling — 2026-11-16

The maintained PowerShell runner now shares local build, shader, cache and discovery settings across gameplay, rendered checks, asset checks, editor tools and packaging. See [local tooling](local-tooling.md) for commands and the retained stock-engine trace-control limitation. No firewall rules or installed engine binaries were changed; no scripts were added.

- Forced editor compile/link passed with `[NoUba]` actions and no UBA listening service (`Saved/Logs/LocalToolingRebuild.log`).
- Gameplay: **31 passed, 0 automation warnings**, report `Saved/Automation/Gameplay-20261116-130438-984`. Rendered keyboard workflow: **1 passed, 0 automation warnings**, report `Saved/Automation/Rendered-20261116-125924-572`; inspected the counter capture under `Saved/ArtReview/20261116-125924-572`.
- Final asset checks: **34 meshes, 87 hulls, four sections / 32 positions, two windows, four pendants; 0 errors and 0 warnings** (`Saved/Logs/Assets-20261116-130523-264.log`). The log explicitly confirms distributed shader compilation is disabled.
- Windows packaging passed with the final settings forwarded to UBT and the cooker (`Saved/Logs/LocalToolingPackageFinal.log`). Output: `builds/unreal-local-tooling/Windows/FantasyShopkeeper.exe`. Zen emitted one sponsor-attachment warning during packaging, restarted on loopback and completed successfully. The forced build also reported the installed compiler's existing preferred-version warning.
- Automated packaged startup and reopening used isolated `Saved/LocalToolingPackageSmoke` data; the morning checkpoint retained its hash and modification time. Launch/restore logs have no warnings or errors.
- Process/socket snapshots (`Saved/Logs/LocalToolingSockets.json`) showed Zen bound only to loopback, no UDP endpoints for the inspected processes and no separate trace server or UBA service. The editor's built-in wildcard trace-control socket remained, including its fallback port when two editor processes ran concurrently. This is an observed snapshot, not a continuous network audit or a guarantee of no firewall prompts.

# Door sign, counter goods and additional orders — 2026-11-16

Implemented the latest playtest revision. E on the entrance sign replaces O for opening/closing. Customers put the actual pickup item into a reserved counter slot and present tender immediately. Total-entry controls are removed; the player works out change and confirms with the PAID stamp. Closing still waits for an active visit. Unsold counter goods are protected from player/customer pickup, retained after rejected sales, and released as loose stock if their checkout is cancelled. Temporary checkout slots are excluded from morning destinations.

The reported day-three blocker was the one-order-per-evening restriction, not the 30-per-ingredient storage limit. Selecting another pack or changing quantity now permits another paid order. Repeated E presses on an unchanged confirmed selection remain harmless. Orders accumulate with capacity/overflow checks across the whole pending delivery; the prompt shows ordered quantities. Morning delivery, failed-write retry and restoring the saved morning preserve one coherent cash/stock result.

- `Tools/check.ps1`: **31 passed, 0 automation warnings**. Final report: `Saved/Automation/Interaction-20261116-122706-857`. Coverage includes reachable/full-hands sign interaction, automatic goods/tender, reserved stock, rejected/duplicate/reentrant sales, wrong change, incompatible checkout destinations in saves, multiple orders with repeated E protection, aggregate capacity, and three consecutive trading days through morning four with three successful sales per day and repeated restoration.
- Rendered actual-map keyboard workflow: **passed, 0 warnings**. Report: `Saved/Automation/CounterGoodsRendered`; log: `Saved/Logs/CounterGoodsRendered.log`. It opens/closes via the sign, completes two sales without total entry, orders supplies and advances morning. Inspected `Saved/ArtReview/door-sign.png` and `counter-goods.png`: sign visible beside the entrance, bottle on the goods mat, customer's hand empty, tender and change on the counter, no number grid.
- Asset commandlet: **34 meshes, 87 hulls, four sections / 32 stock positions, two window openings, four pendants; 0 errors and 0 warnings**. Log: `Saved/Logs/CounterGoodsAssets.log`. Only the counter mesh was reimported; map layout unchanged. Seven maintained Python tools parse; no scripts added.
- Windows package: **passed**, `builds/unreal-counter-goods/Windows/FantasyShopkeeper.exe`. Log: `Saved/Logs/CounterGoodsPackage.log`. Automatic startup/reopening used isolated `Saved/CounterGoodsPackageSmoke`; the 2,336-byte morning checkpoint retained its hash and modification time. Launch/restore logs have no warnings or errors. Earlier builds remain available.

The customer placement currently uses the shared item transfer rather than a bespoke hand/drop animation. The counter remains intentionally simple. Multi-item baskets, technology/magic upgrades and mid-day saving remain future work. All hands-on playtesting is left to the user.

# Evening supplies, lighting and ledger — 2026-11-16

Evening orders now debit once and deliver only after the next morning checkpoint commits. Quantity zero explicitly orders nothing. Failed writes retain the paid order for retry; restoring a morning rolls back both the order and its debit. Suppliers accept orders only in evening, with R for pack, F/T for quantity and E to confirm. X can end an unserved visit before stock pickup, preventing a stock shortage from blocking closing. Farming and foraging remain deferred.

The shop fades into night over four seconds at evening, retaining pendant light. Explicit morning advancement adds a 2.5-second fade and restores daylight. A flat ledger, written number grid, ceramic payment dish, change mat and PAID stamp replace the raised keys and floating ticket board. Existing keyboard checkout, free movement, mouse look and guarded payment remain available.

- `Tools/check.ps1`: **29 passed, 0 automation warnings**. Report: `Saved/Automation/Interaction-20261116-111226-354`. New coverage includes evening-only ordering, quantities, zero orders, delayed ingredients, immutable paid quantities, insufficient capacity, save-write failure and retry, repeated restoration without duplicated money or ingredients, phase lighting and unserved visits without invented payments or lost stock. Existing save corruption, ownership and exact-change regressions remain passing.
- Rendered keyboard workflow: **passed without warnings**, selling both products, closing, paying for an evening order and receiving it in morning two. Report: `Saved/Automation/LedgerRenderedRelease`; inspected captures: `Saved/ArtReview/ledger-till.png`, `ledger-evening.png` and `ledger-morning.png`. Evening shows 30 bits takings; the 12-bit order leaves 18 bits on morning two. Visual review caught and corrected the FBX countertop mirroring, and verified flat ledger/control alignment, dark exterior windows and restored morning daylight.
- Final asset validator: **34 meshes, 87 collision hulls, four sections / 32 stock positions, two window openings and four pendants; 0 errors and 0 warnings**. Log: `Saved/Logs/LedgerAssetsFinal.log`. The removed upright ticket accounts for one fewer hull. The approved map layout remains unchanged.
- Windows Development package: **passed**, `builds/unreal-evening-ledger/Windows/FantasyShopkeeper.exe`. Log: `Saved/Logs/LedgerPackage.log`. Automatic startup and reopening passed using isolated `Saved/LedgerPackageSmoke` data; the 2,336-byte checkpoint retained its hash and modification time. Logs: `LedgerPackageLaunch.log` and `LedgerPackageRestore.log`, without warnings or errors.
- The seven maintained Python tools parse successfully. Selected-mesh regeneration/reimport uses `SHOP_ART_ASSETS`, updates no map or Blueprint assets, and adds no scripts. Final Unreal import completed without warnings/errors; Blender emitted existing API deprecation and sandbox thumbnail-cache notices while successfully exporting the selected FBX files and source scene.

Morning save schema remains compatible: pending orders belong to the unsaved day, while the saved morning contains delivered ingredients and their already-debited cash. No mid-day save or delivery animation is claimed. The player performs manual playtesting; automated checks do not assess subjective handling or sound mix.

The entries below describe earlier checkpoints and are retained as historical evidence.

# Unreal shop days and checkpoints — 2026-11-16

Implemented preparation, explicit opening, a configurable finite visitor allocation, closing that waits for the active customer, evening totals and explicit next-morning advancement. O opens/closes; N advances with empty hands. Morning snapshots replace cash, ingredients and physical items together, including shelf gaps, loose stock and brewing output. Immediate supply purchases remain unchanged; no daily expenses or delayed deliveries are introduced.

- `Tools/check.ps1`: **26 tests passed, 0 automation warnings**. Final report: `unreal/FantasyShopkeeper/Saved/Automation/Interaction-20261116-081559-327`. Includes repeated transitions, active-customer closing, finite visits, whole-economy restoration, subsequent purchases, stale checkout rejection, fresh-state startup, exact shelf positions, loose/output stock, malformed versions/products/quantities/destinations, duplicate slot records, truncated/corrupted files, invalid-file preservation, held-stock blocking, a locked destination and restoration attempts during brewing.
- Rendered keyboard workflow: **passed without warnings**, completing three walking visits and advancing to morning two. Evening shows 42 bits in takings, 24 spent on supplies and 18 closing cash. Inspected HUD captures: `Saved/ArtReview/day-evening.png` and `day-morning.png`; checkout capture: `day-till.png`. Report: `Saved/Automation/DayRendered`.
- Windows Development packaging: **passed**. New build: `builds/unreal-day-loop/Windows/FantasyShopkeeper.exe`. The prior `unreal-interaction` build remains intact. Automatic startup and reopening used an isolated `Saved/PackageSmoke` directory; reopening preserved the checkpoint bytes and modification time. Logs: `Saved/Logs/DayPackageLaunch.log` and `DayPackageRestore.log`.
- The user confirmed the four-section layout as intentional: four apothecary sections provide 32 stock positions, with the retail tables and their stock sections removed. The current map is now the approved layout. Customer flow follows a configurable counter-local offset, verified through the walking workflow. The asset validator and regeneration tool now match this layout; the earlier ten-section assertion in `Saved/Logs/DayAssets.log` is superseded by the follow-up verification below.
- Four-section follow-up: the asset commandlet passed with **0 errors and 0 warnings**, validating 34 meshes, 88 collision hulls, 32 supported stock positions, two window openings and four pendants. Log: `Saved/Logs/FourSectionAssets.log`. The full build and gameplay rerun also passed all **26 tests without automation warnings**; report: `Saved/Automation/Interaction-20261116-085107-109`. The existing expansion installer no longer recreates the retired retail tables or sections; its Python syntax was checked without running an asset rewrite.
- No maintained scripts were added. The existing asset checker now checks the counter-relative waypoint configuration. Unreal emitted its existing compiler-version and engine-header deprecation notices during builds; these are distinct from the warning-free gameplay automation reports.

Morning files have a version and payload checksum. Full value validation precedes staged restoration; Windows file replacement preserves the old checkpoint on write failure. Mid-day saves and map-identity migration are not implemented. Invalid saves are preserved and block further writes; moving the file aside while the game is closed explicitly starts fresh. Automated checks do not touch the player's checkpoint. Human playtesting remains with the user.

---

# Fen encounter validation — 2026-11-08

Fen is the existing second visitor on both days. Their request and outcome do not add visits, change ordinary basket demand, reserve stock or discount purchases. Reading is opt-in through a ledger note; the question can be deferred until bedtime.

- Encounter regression: **265 assertions, 0 failures** headlessly. Covers accepted/deferred/declined/ignored promises, actual payment, wrong customer/day, repeat callbacks and reward claims, a full delivery tray, ordinary economics, no-stock/price-rejection/early-close outcomes, older save compatibility, invalid story data, fresh-scene resume and rolling back a claimed gift with the morning snapshot.
- Graphical encounter run: **272 assertions, 0 failures**, including seven inspected screenshots of the optional ledger entry, conversation, largest basket layout, lamp and reward states.
- Ordinary decline/ignore/defer paths retain the existing **224-bit** two-day ending and no story debt. The only additional merchandise on fulfillment is one tonic, created once into a free delivery slot after an explicit claim.
- A paid starlight purchase makes Fen walk to the doorway and test the lamp. The lamp is decorative and cannot duplicate sold stock. Missed promises receive the bakery-lantern fallback with no fine.
- Source and exported-resource-pack checks include the encounter suite alongside shared actions, checkout and day-loop tests. Test saves use a separate file and are cleaned up; player saves are untouched.

The promise is stored in the day-two morning snapshot. Mid-day fulfillment and reward claims are deliberately not autosaved: restarting restores the complete morning world, including story state, stock and cash. Saves from before this encounter default to no promise. A pre-existing day-two save remains ordinary trade; start fresh or resume day one to experience the offer.

Human feedback on Fen's voice, the optional delivery and the value of the consequence is still outstanding. Native Windows execution remains unverified on this Linux executor. This tests one encounter's implementation, not the whole story strategy. The broader examples and intended evidence are in story-strategy.md.

---

# Two-day loop validation — 2026-11-08

The day-loop regression covers both full trading days, cash reconciliation, explicit opening, safe closing, supplier orders, retained stock, price rejection, customer payment adjustment, recovery credit, repayments and morning persistence.

- Headless day-loop regression: **115 assertions, 0 failures**. It exercises duplicate close/order/advance requests, changing tray capacity after ordering, carried bottles at sleep, restore into a fresh scene, repeated restores, invalid startup saves, invalid stock data, failed writes, and preservation of an existing checkpoint.
- Default manual path: 24 starting cash + 280 takings − 24 daily expenses − 56 ordered stock = **224 cash**, no debt.
- Recovery path: after a no-sales first day, the 112-bit loan permits a paid delivery and day-two trading. Repaying the full 118-bit debt leaves **78 cash and six retained bottles**, proving recovery without forced reset.
- Graphical day-loop regression: **120 assertions, 0 failures**, including five screenshots inspected for the planning, loan and resume screens.
- Windows/Linux exports pass. Checkout (107) and day-loop (115) checks also pass against the exported Linux resource pack. The standalone Linux release creates a morning checkpoint on first launch and resumes it without rewriting it on the second launch.
- Existing shared item actions (30), checkout (107), and scene checks (36 headless; walking is a separate graphical check) remain required by the runner.
- Test scenes disable ordinary player persistence. Save regression files use a separate test path and are removed after the run. Smoke/capture startup also disables persistence, keeping builds from touching player saves.

The checkpoint saves only the morning's starting state, after deliveries; restarting intentionally discards the current day's later actions. Stored stock is validated completely before application. Orders are consumed before saving the new world, so restoring does not replay them. Existing corrupt saves are preserved for an explicit fresh-start decision.

One visitor at a time remains a deliberate limit. Supplier costs 5/9/14, daily expense 12, starting cash 24, and loan terms 112 received/118 due are tuning values. The initial six bottles are already paid for. Customers reject shelf prices above twice the initial price; this is a simple prototype rule, not a demand simulation. Negative cash after a poor final day is shown as a deficit, without forced bankruptcy. Workers, wages, queues and encounters are not implemented.

Native Windows execution and the feel of the new planning screens still require human playtesting. See README.md for controls and reproduction commands.

---

# First customers validation — 2026-11-08

The user confirmed delivery movement and stocking work well. The new customer/checkout slice is tested separately from that physical-shop checkpoint.

- Checkout regression: **107 assertions, 0 failures** headlessly. Covers physical shelf-to-basket ownership, invalid totals, under/overchange, alternate denominations, correction, click/hold/release, focus loss, returning to the ledger, pause/resume, duplicate confirmation, the fixed three-visit allocation, no waiting timeout, delivery stock remaining unavailable, and competing player/customer pickup with another-shelf retry.
- Graphical checkout regression on Xvfb/Mesa with dummy audio: **111 assertions, 0 failures**, including the complete three-visit flow plus four screenshot captures. Screenshots cover invitation, a customer at the counter, the basket ledger, and the change tray.
- Existing shared actions: 30 assertions. Existing scene regression: 36 headless assertions (walking explicitly skipped without a display); 74 assertions in the graphical continuous walking mode.
- `bash tools/check-godot-linux.sh` includes checkout checks. Windows/Linux export packaging succeeds. The standalone Linux release passes startup smoke checks outside the repository. All 107 checkout assertions also pass against the exported Linux resource pack using the pinned editor engine with `--main-pack`; the release executable itself does not support the editor script runner. The build script includes this exported-pack check.

Customer paths follow the room's clear aisles; they are a single-visitor prototype rather than general navigation or queueing. Prices are fixed (12/23/35 bits). Missing products produce immediate remarks; no persistent missing-stock report is added. Customers buy the available portion of a requested basket and leave without a sale if nothing is available. Physical sold items remain with the customer until departure. Takings are recorded once at confirmation; full daily accounting is not implemented.

For play instructions and reproduction commands, see README.md. New checkout audio and mouse feel still need human validation; automated graphical runs use dummy audio. Native Windows and PowerShell execution remain unverified on this Linux host. This historical checkpoint predates the two-day loop above. Queues, workers and encounters remain outstanding.

---

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

These historical checks establish the physical-shop checkpoint. The first customer/checkout slice above was added afterwards; daily finances, persistence, workers and the optional encounter remain future work.
