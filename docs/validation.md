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
