# Unreal MVP roadmap

2026-11-16 — bounded MVP implementation, implemented; automated verification recorded below, ready for player review.

## Agreed scope

One shop, a three-day Lantern Walk chapter, three recurring regulars, the two existing recipes, manual change counting, next-morning ingredient deliveries and the existing storage/batch upgrades. The user authorised implementation through the finished MVP before reviewing and tidying. The wider [GDD](game-design-document.md) remains a future production direction, not a requirement to expand this slice.

## Implemented milestones

1. **Menus and onboarding.** Startup/resume and F1 pause menu; how-to-play, sensitivity, volume, High/Epic graphics, journal, credits, confirmed restart/new shop/quit. Menus disclose morning-only saving. Phase-specific prompts teach preparation, stocking, checkout, ordering and delivery.
2. **Physical work.** Brewing shows batch size, deterministic ingredient costs and available quantities even when blocked. Existing tactile goods/payment, manual change, three-person queue and physical controls remain authoritative.
3. **Recurring cast.** Ivo the courier buys Dawn Tonic, Fen the lamplighter buys Moonlit Ink, and Nessa the printer buys Dawn Tonic. Distinct small work badges and authored return lines use the existing mannequin presentations. There is no voice requirement or patience penalty.
4. **Optional story.** Fen's day-one note asks for one Moonlit Ink at the normal 18-bit price on day two. Accept, defer or decline through the journal; a real paid collection lights a lantern by the entrance. Missing the request causes no fine. Story, cash and inventory roll back together at a morning restart. No third recipe was needed.
5. **Progression and recovery.** Existing upgrade balance supports three days and continued trade. Supplier credit provides 6 Moonleaf + 6 Water next morning when short of supplies and holding less than 12 bits: owe 18 total, no interest/deadline, one outstanding loan. Order/repay at the physical supplier book. The chapter summary opens after evening three and leaves continued trading available.
6. **Verification and review build.** Local build, focused transaction/persistence tests, actual-map menu/story/queue workflow, rendered reviews, asset checks, Windows packaging and isolated packaged-save checks. Exact results are recorded in [validation](validation.md). Manual feel, pacing and outside playtesting are the user's review step.

## Persistence contract

Version 5 morning checkpoints add the lantern story and supplier account to the existing cash, ingredients, upgrades, bottles and unopened delivery. Versions 1–4 remain supported. The game does not save mid-day on quit. Settings persist separately. New game backs up the previous morning; invalid saves are preserved. No player's checkpoint is used by automation.

## Outside this MVP

Workers, farming/foraging, additional rooms, multi-item baskets, changing prices, taxes/rent, further recipes or upgrade tiers, deeper crafting and full mid-day save-and-quit. Wider story arcs and the commercial chapter's six-character content envelope remain later production decisions.

## Review gates

- Can a new player finish day one without control explanations?
- Does repeated brewing/stocking/change counting remain enjoyable?
- Are the three regulars recognisable and Fen's terms clear?
- Can the request be ignored comfortably, and can bad ordering recover?
- Is the chapter ending enough to make the player want another morning?

Fix confusion and defects found here before adding systems or content. No public release or store submission is part of this milestone.

## Working setup

Production: `unreal/FantasyShopkeeper/FantasyShopkeeper.uproject`, Unreal Engine 5.8.4. Use native C++ gameplay with Blueprint presentation, the existing Git/LFS repository and the seven maintained Python tools. Build/test/package through `Tools/check.ps1`; see [local tooling](local-tooling.md). The Godot project remains a behavioural reference. Keep manual playtesting with the user and make reviewed, focused commits.
