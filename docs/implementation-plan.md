# Implementation checklist

The approved experiment has five focused sessions. These are playable milestones, not calendar promises. Review the effort cap if a milestone requires more work. The full target is in prototype-spec.md.

## 1. Physical shop and shared actions

Cloud validation on 2026-11-07 fixed the player constructor and false smoke success. Shared actions pass 30 assertions; the graphical route passes 74, including continuous walking, aiming, all six transfers, collision and pause/resume. Windows/Linux release archives are packaged; the extracted Linux release passes smoke validation. See validation.md. The user confirmed on 2026-11-08 that delivery movement and stocking work well.

- [x] Set up Godot and a Windows launch path (Windows engine launcher not re-executed on Linux).
- [x] Build a navigable, modest fantasy shop with a delivery area and labelled shelves.
- [x] Pick up and place actual products, one at a time, with clear prompts and occupied/incompatible feedback.
- [x] Centralise item actions so future workers use the same reach, visibility, carry, slot, and stock checks.
- [x] Show at least one glowing item.
- [x] Verify successful transfers and failure cases through the public action interface using two actors (30 assertions; clean cloud rerun passed).
- [x] Run a visual smoke test and package a playable first milestone (native Windows playtest pending).

## 2. Customer and checkout

First playable slice added on 2026-11-08: three explicitly invited visits, one customer at a time, with baskets of one/two/three physical items. Fixed shelf prices are visible; the ledger requires manual totals and change. Stock races retry another matching shelf; missing shelf stock receives explicit feedback. No timeouts or penalties for waiting. Price rejection is now included in milestone 3; general queueing remains outstanding.

- [ ] Customer browses, obtains one to three available products, queues, and visits the counter. (Browsing/baskets/counter implemented; queue deferred.)
- [x] Missing stock and unreasonable prices cause explicit lost sales; waiting causes no penalty. (Missing stock, adjustable prices, rejection and waiting implemented.)
- [x] Enter basket total, receive payment, count change using 1/10/100 pouches.
- [x] Click/hold repeat, tray removal, keyboard equivalents, confirmation and correction.
- [x] Leaving and returning to the counter preserves the pending transaction.
- [x] Verify no duplicate charges or stock loss on retries.

## 3. Days, stock, finances, persistence

Added on 2026-11-08. Two explicit preparation/trading/closing/evening cycles; three fixed opportunities per day; closing waits for the active customer. Supplier ordering, shelf prices, single-charge expenses, a bounded recovery loan, voluntary repayment and morning snapshots are implemented. Ordinary manual trading ends with 224 bits from 24 starting cash after paying for day-two stock and both daily expenses. A recovery scenario can clear the 118-bit loan and retain 78 cash plus six bottles after day two. These are tuning values. The user has playtested this loop successfully. Story strategy and a representative encounter now precede the restocker.

- [x] Unlimited preparation/evening with explicit open/advance actions.
- [x] Fixed customer allocation and safe closing after remaining service.
- [x] Supplier ordering, morning delivery, shelf pricing and disclosed daily costs.
- [x] Basic recovery loan with explicit debt and repayment; tune recovery through one or two days of stock.
- [x] Morning save/resume, fresh start, and unrestricted pause.
- [x] Verify daily transitions and saves do not duplicate stock, charges, or debt. (Wage persistence awaits delegation.)

## 4. Delegation

Still planned; follows the first story slice under the revised iteration order.

- [ ] Hire and prepay the mundane restocker during evening planning.
- [ ] Worker navigates to stock and shelf and uses the shared item action interface.
- [ ] Competing player/worker actions remain safe; worker retries if a product or slot changes.
- [ ] Play two days and compare manual work with assisted work.

## 5. Optional encounter and review (current focus)

The short [story strategy](story-strategy.md) is written, with three connected examples: a paid product promise, a counter-display choice, and a returning regular without a quest. The next implementation is Fen's starlight request, using existing visits and ordinary checkout. No encounter code has been added by this strategy step.

- [x] Define story principles, optional participation, connected examples and first-slice acceptance checks.

- [ ] Optional day-one request, day-two return, brief consequence, and bonus stock.
- [ ] Verify ordinary trade alone funds the helper; declining remains viable.
- [ ] Finish enough feedback and sound to assess counting and handling.
- [ ] Package a Windows build and conduct the agreed playtest.
- [ ] Record evidence against continuation criteria and decide whether to extend the project.

## Early follow-up if continuing

Prioritise crate handling, then act on playtest evidence. Preserve the ideas for different workers, magical machinery, maintenance automation, richer lenders, and missing-stock reports without implementing them speculatively.
