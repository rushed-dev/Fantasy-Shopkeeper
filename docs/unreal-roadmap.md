# Unreal setup and implementation roadmap

2026-11-15 · Unreal implementation checkpoint and remaining production roadmap

This roadmap follows the [production GDD](game-design-document.md) and [engine evaluation](engine-decision.md). The existing Godot game remains the playable reference. The [prototype checklist](implementation-plan.md) records that implementation's progress; its completed items do not mean those features exist in Unreal.

## Current Unreal checkpoint

The shared repository now contains the Unreal 5.8.4 project, native interaction and item ownership, safe surface set-down, two ingredient-checked recipes, walking customers, manual change, repeating visits and evening orders with next-morning delivery. Dawn Tonic and Moonlit Ink have separate data assets and physical bottle presentations. An original procedural kit dresses the interior. The approved stock layout has four eight-slot apothecary sections (32 positions). Thirty-one automated regressions cover the gameplay boundaries and a keyboard workflow through both products, evening ordering and morning delivery. The user has playtested the day loop; the latest ledger and lighting changes have automated rendering coverage.

Recipe prices, tender, supplier quantities and storage capacity are prototype tuning decisions. Unreal now has explicit preparation, finite trading, closing and evening, with versioned morning checkpoints for cash, ingredients and physical stock. Reopening rolls back to the saved morning. Phase-based evening lighting, a morning fade and scheduled deliveries are implemented. The entrance sign opens/closes with E. Customers put goods and payment on the counter automatically; manual total entry is removed. Multiple evening orders accumulate into one guarded delivery, with three consecutive trading days covered by regression. A physical order book shows planning figures and has E-operated tabs; resting fades through night with a short procedural soundscape. Morning checkpoints include a sealed entrance delivery, which must be unpacked once before opening. Mid-day save-and-quit and the story slice remain subsequent milestones. The older Godot implementation already explores those systems and supplies scenario references for the Unreal port.

## Recommended engine

Use **Unreal Engine 5.8.4**, installed through Epic Games Launcher, for the evaluation and first production slice. Epic has published its [5.8.4 hotfix announcement](https://forums.unrealengine.com/t/5-8-4-hotfix-released/2842775). This is a current released UE5 baseline, not a claim that this project has been tested on it.

Your Blueprint experience and Ryzen 7 9800X3D / 32 GB / RTX 5070 Ti support the contained-shop evaluation. For agent-assisted production, the updated recommendation is C++ gameplay classes with Blueprint children for presentation and tuning. Start with the first-person template, one contained map, Enhanced Input, UMG and simple snap placement. Try Lumen for the selected lighting direction and measure the result. Nanite and advanced effects should earn their place through a visible benefit.

Check required asset packs and plugins against 5.8 before buying or depending on them. Prefer a small dependency list. If an essential dependency only supports 5.7, evaluate its latest released hotfix as a fallback before creating production content. Record the exact engine and plugin versions and keep them fixed through the slice; evaluate upgrades separately with a packaged-build check.

## Recommended version control

Use **GitHub Desktop + Git LFS + a private GitHub repository**. Git is the version control system, Desktop provides the everyday interface, LFS stores large binary versions, and GitHub holds the remote copy. This fits solo development without running a Perforce server.

The Unreal project lives under `unreal/FantasyShopkeeper` in this repository alongside the Godot reference. The repository's root Git ignores and LFS rules cover the Unreal project and its source art.

Before the first Unreal content commit:

1. Install GitHub Desktop and verify Git LFS is available in the shell used for setup.
2. Create the Unreal project and repository, then configure Unreal ignores and LFS before importing packs.
3. Track at least `*.uasset` and `*.umap` through LFS. Add large source-art/audio formats actually used by the project, such as `*.blend`, `*.psd`, `*.fbx` and `*.wav`.
4. Commit `.gitattributes` and `.gitignore`. Commit the `.uproject`, `Config`, `Content`, any `Source`, and required project plugin files. Exclude generated caches, `Intermediate`, `Saved`, ordinary rebuildable `Binaries`, IDE output and packaged builds. Handle third-party binary-only plugins according to their distribution requirements.
5. Push a small real asset, clone into another local folder and open/package that clone. Verify the assets downloaded as usable files and that nothing required exists only in the original working directory.

The [GitHub LFS configuration guide](https://docs.github.com/en/repositories/working-with-files/managing-large-files/configuring-git-large-file-storage) explains extension tracking and committing `.gitattributes`. Apply those rules before adding large assets to history.

Daily routine: pull before opening the editor, complete one coherent change, save, review the changed files, commit with a useful description and push. Use `main` for working builds and short branches for risky experiments. Keep Blueprint responsibilities small; binary asset conflicts usually require choosing or recreating an edit rather than combining text changes. Avoid editing the same project simultaneously on two machines.

If artists or other developers join and asset locking becomes important, evaluate **Anchorpoint on the same Git/LFS repository**. Its [quick-start documentation](https://docs.anchorpoint.app/getting-started/quick-start/) describes its game-development workflow and locking. Agree on one locking workflow across collaborators; Anchorpoint-specific locks should not be assumed to protect edits made with arbitrary Git clients.

GitHub LFS has storage and transfer allowances, with usage-based billing beyond the included allowance. Check the account's [current LFS billing and budget settings](https://docs.github.com/en/billing/concepts/product-billing/git-lfs) before importing large packs. Import selected assets rather than entire libraries. Keep release packages outside source history and maintain a separate backup of source art and the repository, including LFS objects.

## Milestones

Work in the order below. Finish and play the completion check before expanding scope. The initial evaluation has a three-to-five focused-day budget; later milestones need estimates based on measured progress and available weekly hours.

### 0. Establish a recoverable project

- [ ] Preserve a working Godot build and record its current behaviour.
- [x] Install/pin Unreal, create the project alongside the reference and configure Git/LFS.
- [ ] Add input, pause and look sensitivity; create a small greybox shop.
- [ ] Package a Windows build and verify a fresh clone can produce it.

**Complete when:** a clean checkout opens and packages, and the executable runs outside the editor. Resolve setup failures before building game systems.

### 1. Evaluate look, feel and authoring cost

- [x] Dress one corner with a counter, shelf, brewing bench, window and coherent original assets.
- [x] Pick up and snap-place a bottle, turn a fixed ingredient input into one bottle, then complete one simple sale.
- [ ] Compare morning/evening lighting, readable labels, glass/liquid treatment and a small magical effect.
- [ ] Record packaged frame times, memory, shader waits, edit/play time and package time. Choose a representative player hardware target; the development GPU alone cannot establish minimum specifications.

**Complete when:** the corner meets the visual direction, handling is pleasant, packaging works and the authoring loop is sustainable. Aim provisionally for 60 fps at 1080p on the chosen player target. At the end of the evaluation budget, decide whether to proceed, simplify one specific obstacle or revisit Godot Forward+.

### 2. Prove the brewing-to-sale loop

- [x] Introduce stable product/recipe IDs and separate definitions from runtime ingredient quantities and item ownership.
- [ ] Implement one bench, a few ingredients and two authored recipes with readable steps and recoverable corrections.
- [x] Create physical output once, carry it, stock it and let one customer buy it.
- [x] Restore manual totals and change using 1/10/100 denominations, including correction and leaving/returning to the counter.
- [ ] Add one shared action boundary for consuming ingredients, moving items and completing a sale. Keep UI and visual effects from independently changing economic state.

**Complete when:** a player makes and sells both products repeatedly without duplicated output, lost ownership or duplicate payment. Playtest repeated batches: change uninteresting actions before adding recipes.

### 3. Make a dependable repeating day

- [x] Add explicit preparation, trading, closing and evening phases with fixed customer opportunities and unlimited thinking time.
- [x] Add evening supplier orders and guarded next-morning deliveries, including an explicit zero order.
- [ ] Add shelf prices, daily expenses and disclosed recovery finance.
- [x] Implement versioned morning checkpoints for cash, ingredients, shelf stock, loose items and brewing output.
- [ ] Add production save-and-quit for pending work and the current phase.
- [ ] Preserve ingredient stock, batch progress, physical items, active baskets, payment/change, finances and pending deliveries together.
- [ ] Exercise interrupted brewing, pending checkout, repeated close/day-advance actions and reloads around deliveries. Preserve invalid saves and report the problem.

**Complete when:** several consecutive days work in a packaged build, ordinary trade pays its costs, and reloads neither repeat nor erase committed economic actions. Do not claim mid-day saving works until those cases pass.

### 4. Finish a representative story slice

- [ ] Rework Fen's need into one optional recipe/product choice and a later paid collection through normal checkout.
- [ ] Support accepting, deferring, declining and missing the request; state purchase/donation/reservation terms explicitly.
- [ ] Show a visible consequence and grant any reward once. Persist the story with inventory and money.
- [ ] Add a third recipe and a second short customer encounter to measure content-authoring effort.
- [ ] Playtest with new players, including someone who ignores stories. Record confusion, repetition, performance and actual session length.

**Complete when:** the slice feels like a small version of the intended game, voluntary stories work, ordinary commerce remains viable, and adding content does not require rewriting the systems. Re-estimate the remaining project here.

### 5. Test progression and paid help

- [ ] Add one storage improvement, one batch improvement and one mundane restocker.
- [ ] Have the worker use the same ownership/action rules as the player, with disclosed wages, safe retries and blocked-route handling.
- [ ] Check simultaneous attempts to move the same item or fill the same slot.
- [ ] Compare manual and assisted play and tune costs around comfort and useful freedom.

**Complete when:** ordinary trade can fund the helper, assistance is worth buying, and the manual route stays enjoyable. Defer further automation until this is demonstrated.

### 6. Produce the bounded chapter

- [ ] Use the GDD's provisional caps: six recurring customers, three short arcs, six to eight recipes and eight to twelve ingredients.
- [ ] Complete a small upgrade set and one coherent chapter in the shop/workroom.
- [ ] Track asset sources, generation tools, dialogue, audio and usage rights as content is added. AI-generated artwork is permitted as of 2026-11-15. Verify applicable release disclosures during release preparation.
- [ ] Measure content cost and pacing continuously; reduce scope if either exceeds the solo budget.

**Complete when:** the chapter is playable from a fresh save through its ending, optional-story paths remain viable, and content/provenance records are complete. These counts remain planning caps rather than advertised promises.

### 7. Prepare a release candidate

- [ ] Finish readable UI, text scaling, remapping, sensitivity and any agreed arithmetic assistance.
- [ ] Validate save compatibility/recovery and complete end-to-end packaged Windows playthroughs.
- [ ] Test performance and scalability on representative player hardware, then establish the published requirements.
- [ ] Finish audio, feedback, credits, licence notices, accurate store disclosures and screenshots from the game.
- [ ] Have external testers repeat the ordinary-trade and story routes and resolve release-blocking defects.

**Complete when:** a reproducible build completes the chapter without blocking defects, corrupting saves or violating the ownership/economy rules, and the supported hardware can run it acceptably.

## Implementation habits

Use focused C++ gameplay actors/components, a shared interaction interface, separate recipe/product data and small UI widgets. Use Blueprint children for meshes, materials, effects and tuning. Interaction, item ownership, brewing, customer visits, supplies and checkout now follow this approach, with editor-Python tools for repeatable asset installation and map checks.

Use the existing Godot regressions as scenario specifications, not evidence that Unreal is correct. Add focused Unreal checks for ownership, transaction retries and persistence as those systems arrive, alongside hands-on checks for handling, readability and fun. Package at every milestone.

For each work session, choose one observable outcome, implement it, verify it in play and push a working checkpoint. The immediate target after setup is: **make one bottle, place it on a shelf and sell it in a packaged build**.

### Ingredient storage reinvestment

Implemented one physical evening purchase: 24 bits for extra ingredient drawers, doubling each ingredient capacity from 30 to 60. It uses guarded economy transactions and the next morning checkpoint, with separate upgrade spending in the order book. Save schema 3 restores the upgrade alongside cash, ingredients and paid deliveries; schema 1/2 retains base capacity. This is a single storage tier; batch brewing and worker automation remain future work.
