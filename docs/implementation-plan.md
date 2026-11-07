# Implementation checklist

The approved experiment has five focused sessions. These are playable milestones, not calendar promises. Review the effort cap if a milestone requires more work. The full target is in prototype-spec.md.

## 1. Physical shop and shared actions (current)

Cloud validation on 2026-11-07 fixed the player constructor and false smoke success. Shared actions pass 30 assertions; the graphical route passes 74, including continuous walking, aiming, all six transfers, collision and pause/resume. Windows/Linux release archives are packaged; the extracted Linux release passes smoke validation. See validation.md. Technical implementation is ready for playtesting; native Windows and subjective usability acceptance remain outstanding.

- [x] Set up Godot and a Windows launch path (Windows engine launcher not re-executed on Linux).
- [x] Build a navigable, modest fantasy shop with a delivery area and labelled shelves.
- [x] Pick up and place actual products, one at a time, with clear prompts and occupied/incompatible feedback.
- [x] Centralise item actions so future workers use the same reach, visibility, carry, slot, and stock checks.
- [x] Show at least one glowing item.
- [x] Verify successful transfers and failure cases through the public action interface using two actors (30 assertions; clean cloud rerun passed).
- [x] Run a visual smoke test and package a playable first milestone (native Windows playtest pending).

## 2. Customer and checkout

- [ ] Customer browses, obtains one to three available products, queues, and visits the counter.
- [ ] Missing stock and unreasonable prices cause explicit lost sales; waiting causes no penalty.
- [ ] Enter basket total, receive payment, count change using 1/10/100 pouches.
- [ ] Click/hold repeat, tray removal, keyboard equivalents, confirmation and correction.
- [ ] Leaving and returning to the counter preserves the pending transaction.
- [ ] Verify no duplicate charges or stock loss on retries.

## 3. Days, stock, finances, persistence

- [ ] Unlimited preparation/evening with explicit open/advance actions.
- [ ] Fixed customer allocation and safe closing after remaining service.
- [ ] Supplier ordering, morning delivery, shelf pricing and disclosed daily costs.
- [ ] Basic recovery loan with explicit debt and repayment; tune recovery through one or two days of stock.
- [ ] Morning save/resume, fresh start, and unrestricted pause.
- [ ] Verify daily transitions and saves do not duplicate stock, charges, wages, or debt.

## 4. Delegation

- [ ] Hire and prepay the mundane restocker during evening planning.
- [ ] Worker navigates to stock and shelf and uses the shared item action interface.
- [ ] Competing player/worker actions remain safe; worker retries if a product or slot changes.
- [ ] Play two days and compare manual work with assisted work.

## 5. Optional encounter and review

- [ ] Optional day-one request, day-two return, brief consequence, and bonus stock.
- [ ] Verify ordinary trade alone funds the helper; declining remains viable.
- [ ] Finish enough feedback and sound to assess counting and handling.
- [ ] Package a Windows build and conduct the agreed playtest.
- [ ] Record evidence against continuation criteria and decide whether to extend the project.

## Early follow-up if continuing

Prioritise crate handling, then act on playtest evidence. Preserve the ideas for different workers, magical machinery, maintenance automation, richer lenders, and missing-stock reports without implementing them speculatively.
