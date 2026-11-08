# Fantasy Shopkeeper: two-day prototype

Status: scope approved in the grilling session on 2026-11-07. The recovery loan supersedes the proposed bankruptcy/replay policy. This document describes the target prototype; see implementation-plan.md for what is actually built.

## Purpose and decision to make

Build on the enjoyable retail foundation to explore a cosy fantasy shop with memorable returning customers and optional stories. Ordinary trading must remain a complete, viable playstyle. Story consequences and bonus opportunities should grow from shopkeeping. Delegation remains a planned progression step, with mundane workers economically useful.

The user playtested and enjoyed stocking, checkout and the two-day loop on 2026-11-08. The next priority is the shop's story identity: recurring customers, voluntary opportunities and consequences that return through ordinary trade. See [story-strategy.md](story-strategy.md) for the working direction and connected examples. A representative story slice now precedes the restocker; delegation remains planned. This changes the iteration order without authorising a larger campaign or making story engagement necessary for ordinary progression.

The first experiment is capped at five focused development sessions, followed by a review. This is an effort limit, not a delivery guarantee. Target roughly ten minutes of active trading across two days; preparation, arithmetic, pauses, and evening decisions may take as long as the player needs.

## Experience

- Single player, Windows, keyboard and mouse, first person, simple 3D assets.
- Cosy, funny, relaxed. Recoverable trouble and eccentric customers; challenge is not the principal attraction.
- Grow from doing every job yourself to choosing stock, advising special customers, and delegating routine work.
- Mundane workers remain competitive throughout the intended full game. Magical solutions offer different benefits, costs, and maintenance demands. Later automation may address those demands.
- Original setting, characters, item descriptions, art, sound, and interface. Fantasy retail and decimal coin mechanics are the inspiration; licensed franchise expression is not part of the asset plan.

## Daily cycle

1. Morning: receive last evening's orders, stock shelves, set prices, and prepare without a timer. The first morning starts with a small delivery and enough funds to trade.
2. Open trading: a fixed allocation of potential customers visits. Day progress follows resolved visits, including lost sales, rather than real elapsed time. Slow arithmetic therefore does not reduce earning opportunities.
3. Closing: stop new arrivals and finish serving remaining customers with unlimited time.
4. Evening: review finances, buy next morning's stock, arrange the next day's restocker, and advance when ready. No evening time limit.

Customers may comment on queue length but do not abandon purchases, reduce payment, or damage reputation because of waiting. Opening the shop is an explicit action. Pausing is always available.

## Stock and interactions

- A few distinguishable products; small customer baskets (initially one to three items).
- Individual physical products move from delivery crates to designated shelf positions. Initially carry one item at a time and snap into valid positions. Multi-item crate handling is an early follow-up after the first playtest.
- Player and workers use exactly the same item action interface and rules: actual stock, reachable locations, carry capacity, occupied slots, and compatible products. Workers cannot silently teleport stock or bypass these rules.
- Abilities may later change explicit capabilities for either actor. A worker's identity alone does not grant privileges.
- Customers want particular products. Unavailable shelf stock causes a visible reaction and lost sale, even if a delivery crate still contains that product.
- Initially the player must notice what customers wanted. No persistent missing-stock report. Future hires or mechanics may track it.
- At least one product visibly behaves magically in ordinary retail, such as a glowing vessel illuminating nearby objects.

## Counter and currency

- A close counter view contains a sales ledger, three coin pouches, and a change tray.
- Player adds item prices and enters the basket total; prices stay visible. Wrong totals permit correction with no penalty.
- Customer supplies payment; player calculates and dispenses change.
- Three denominations worth 1, 10, and 100 base units. Working names: bits, marks, crowns. All later grades remain ten times the previous grade.
- Click gives one coin. Holding gives a steady repeat after a short initial delay and stops on release. Audible and visible feedback make counting learnable. Keyboard equivalents are supported.
- Clicking a tray coin removes it. A separate action commits the sale. Any combination with the correct total is valid, including ten low-value coins instead of one higher-value coin.
- Wrong change permits retry. Pouches have unlimited change in this experiment; cash-reserve management is deferred.
- The player can leave the counter and return without losing the pending transaction.

## Economy and progression

- Fixed supplier costs, player-set shelf prices, a small daily shop expense, and restocker wages.
- Customers may reject unreasonable prices. Missing shelf stock loses sales. Waiting time never does.
- Stock purchases and labour compete for funds. Manual work saves the wage; automation creates time for serving customers.
- Hire a mundane local restocker on evening one and experience their work on day two. The wage is disclosed and prepaid for the next day, alongside planning stock purchases.
- Simple recovery loan: when finances cannot fund continued trading, offer enough credit for roughly one or two days of basic stock. Show the borrowed amount, outstanding debt, and repayment terms before accepting. Debt is the consequence, rather than forcing a reset.
- Loan amount, modest financing cost, and repayment pacing are tuning decisions, not yet player-approved figures. Implement a transparent, bounded basic loan without compounding late penalties or a mandatory narrative encounter; verify that it actually permits recovery. Do not silently add a lender faction system.
- Future lenders may include nobles, thieves' guilds, and dragons, making borrowing a route to wider-world interactions.

## Optional encounter

The recommended first slice is Fen’s optional day-one request and day-two return, described in story-strategy.md. It is not implemented yet. One customer makes an optional request on day one and returns on day two. Fulfilling it produces a short story consequence and a small merchandise reward, potentially glowing stock. Declining preserves viable ordinary progression. Failed accepted requests may have clearly understood, bounded consequences.

Magical automation rewards, branching quest systems, and additional encounter content are later work. The prototype's helper is mundane and affordable through ordinary trading alone.

## Persistence and recovery

Save at the start of each morning, after deliveries and day setup. Resume from that morning's state. Allow a fresh experiment. Mid-transaction persistence is deferred. Save and reload must not duplicate deliveries, charges, loans, or hired labour.

## Validation and continuation decision

Play personally, then observe two or three other people without coaching every interaction. Evaluate:

1. Are stocking and serving satisfying during an ordinary stretch?
2. Does the restocker noticeably improve the player's day, and do they want another upgrade?
3. Is the special request worth considering while skipping it remains enjoyable?
4. Can slower players take their time without losing customers to waiting or receiving fewer daily opportunities?
5. Can poor finances be recovered using the basic loan without an escalating punishment spiral?

If the first two fail, revise the retail loop before adding content. If only the encounter fails, revise that encounter. The highest useful automated seam is the shared item-action boundary: two actors competing for a physical item must never duplicate or lose it, and must face the same validations. Later cover completed sales, day transitions, recovery loans, and morning save/resume at their public boundaries.

## Deferred

Multi-item crate carrying; further staff duties; missing-stock reports; staff quirks such as maximising the number of change coins; magical automation and maintenance chains; world-linked lenders; large item catalogues; persistent reputation; complex branching stories; elaborate economy; mid-transaction saves; multiplayer; other platforms.

## Reference checks

- https://www.copyright.gov/register/tx-games.html (mechanics versus expressive content; US guidance)
- https://www.gov.uk/intellectual-property-an-overview (UK IP categories)
- https://www.dndbeyond.com/sources/dnd/br-2024/equipment (reference only; our denomination ladder differs)
- https://docs.godotengine.org/en/stable/tutorials/export/exporting_for_windows.html
