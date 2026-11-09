# Unreal setup and implementation roadmap

2026-11-09 · Recommended plan; Unreal implementation has not started

This roadmap follows the [production GDD](game-design-document.md) and [engine evaluation](engine-decision.md). The existing Godot game remains the playable reference. The [prototype checklist](implementation-plan.md) records that implementation's progress; its completed items do not mean those features exist in Unreal.

## Recommended engine

Use **Unreal Engine 5.8.4**, installed through Epic Games Launcher, for the evaluation and first production slice. Epic has published its [5.8.4 hotfix announcement](https://forums.unrealengine.com/t/5-8-4-hotfix-released/2842775). This is a current released UE5 baseline, not a claim that this project has been tested on it.

Your Blueprint experience and Ryzen 7 9800X3D / 32 GB / RTX 5070 Ti make a Blueprint-first evaluation appropriate. Start with the first-person template, one contained map, Enhanced Input, UMG and simple snap placement. Try Lumen for the selected lighting direction and measure the result. Nanite and advanced effects should earn their place through a visible benefit.

Check required asset packs and plugins against 5.8 before buying or depending on them. Prefer a small dependency list. If an essential dependency only supports 5.7, evaluate its latest released hotfix as a fallback before creating production content. Record the exact engine and plugin versions and keep them fixed through the slice; evaluate upgrades separately with a packaged-build check.

## Recommended version control

Use **GitHub Desktop + Git LFS + a private GitHub repository**. Git is the version control system, Desktop provides the everyday interface, LFS stores large binary versions, and GitHub holds the remote copy. This fits solo development without running a Perforce server.

Create a separate Unreal repository when starting the evaluation; keep this Godot repository and its history intact. This is a proposed setup, not a repository already created by this document.

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
- [ ] Install/pin Unreal, create the separate candidate project and configure Git/LFS.
- [ ] Add input, pause and look sensitivity; create a small greybox shop.
- [ ] Package a Windows build and verify a fresh clone can produce it.

**Complete when:** a clean checkout opens and packages, and the executable runs outside the editor. Resolve setup failures before building game systems.

### 1. Evaluate look, feel and authoring cost

- [ ] Dress one corner with a counter, shelf, brewing bench, window and a few coherent licensed assets.
- [ ] Pick up and snap-place a bottle, turn a fixed ingredient input into one bottle, then complete one simple sale.
- [ ] Compare morning/evening lighting, readable labels, glass/liquid treatment and a small magical effect.
- [ ] Record packaged frame times, memory, shader waits, edit/play time and package time. Choose a representative player hardware target; the development GPU alone cannot establish minimum specifications.

**Complete when:** the corner meets the visual direction, handling is pleasant, packaging works and the authoring loop is sustainable. Aim provisionally for 60 fps at 1080p on the chosen player target. At the end of the evaluation budget, decide whether to proceed, simplify one specific obstacle or revisit Godot Forward+.

### 2. Prove the brewing-to-sale loop

- [ ] Introduce stable product, recipe and item IDs; separate definitions from runtime quantities and locations.
- [ ] Implement one bench, a few ingredients and two authored recipes with readable steps and recoverable corrections.
- [ ] Create physical output once, carry it, stock it and let one customer buy it.
- [ ] Restore manual totals and change using 1/10/100 denominations, including correction and leaving/returning to the counter.
- [ ] Add one shared action boundary for consuming ingredients, moving items and completing a sale. Keep UI and visual effects from independently changing economic state.

**Complete when:** a player makes and sells both products repeatedly without duplicated output, lost ownership or duplicate payment. Playtest repeated batches: change uninteresting actions before adding recipes.

### 3. Make a dependable repeating day

- [ ] Add explicit preparation, trading, closing and evening phases with fixed customer opportunities and unlimited thinking time.
- [ ] Add supplier purchases, next-morning deliveries, shelf prices, daily expenses and disclosed recovery finance.
- [ ] Implement versioned morning checkpoints, then production save-and-quit for pending work and the current phase.
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
- [ ] Track human-made art, dialogue, audio and licences as content is added; resolve the existing content-policy questions before commissioning or producing release material.
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

Start Blueprint-first with focused actors/components, a shared interaction interface, separate recipe/product data and small UI widgets. Keep the level Blueprint for level-specific presentation. Introduce C++ only for a concrete maintainability, testing, tooling or performance need.

Use the existing Godot regressions as scenario specifications, not evidence that Unreal is correct. Add focused Unreal checks for ownership, transaction retries and persistence as those systems arrive, alongside hands-on checks for handling, readability and fun. Package at every milestone.

For each work session, choose one observable outcome, implement it, verify it in play and push a working checkpoint. The immediate target after setup is: **make one bottle, place it on a shelf and sell it in a packaged build**.
