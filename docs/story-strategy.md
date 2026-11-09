# Story strategy: a shop with a place in the community

Working direction — 2026-11-08. Fen’s first two-day encounter is now implemented; Nessa and Ivo remain working examples for the broader direction. The user has playtested and enjoyed stocking, manual checkout and the two-day loop. Our next useful exploration is the fantasy shop's personality and relationships; the retail foundation has enough evidence to support that work.

The [production GDD](game-design-document.md) now connects this narrative direction to potion-making and bespoke work. Fen remains an implemented retail/story reference; the crafting interpretation is proposed future work.

## The experience we want

The player should gradually recognise who comes through the door, understand a little of their lives, and see their own shop becoming useful to those people. Fantasy changes what people need, what products can do, and what happens after a sale. A bottle can be both an item worth 35 bits and the reason a familiar customer comes back with something to tell you.

Ordinary trade remains a complete way to play. Story participation offers particular relationships, small changes and opportunities. It must not become the only sensible source of income, staff, essential products or progression. The town continues to have a life when the player declines to help.

The longer-term shape is a set of short, overlapping arcs among recurring customers. They can accumulate into a sense of local history without requiring a mandatory main quest, a world-saving role, or exploration outside the shop. More ambitious overarching plots can be considered after this smaller structure has a voice.

## Principles for content and delivery

- **Put retail decisions at the centre.** Stocking, sourcing, reserving, pricing, recommending and making room in the shop can carry a story choice. Do not make every story a request for a free item. A normal paid purchase can have an interesting consequence.
- **Give characters lives beyond their requests.** A regular needs an ordinary reason to shop, an identifiable concern or habit, and something that can change over several visits. Some visits should be ordinary purchases with no request at all.
- **Let consequences return through the shop.** A changed basket, a remembered promise, a small object, a remark from another regular or a new supplier offer can show what happened. Avoid resolving everything in a reward popup.
- **Make engagement voluntary in practice.** Offer one short conversational opening, then let the player continue trading, hear more, defer, accept or decline. Do not repeatedly reopen an ignored conversation. Reading or thinking must not cost customer opportunities.
- **Disclose commitments.** Before accepting, show the product, quantity, collection visit and payment or donation terms. Do not silently turn a purchase into a gift, lock stock, change a price or create debt. Promising something need not guarantee that it will happen, but the possible consequence should be understandable.
- **Keep consequences proportionate.** An unfulfilled promise may change a later line or close that particular bonus opportunity. It should not trigger economic collapse, hidden reputation penalties or repeated guilt. Other characters can solve problems too.
- **Make fantasy tangible.** Give products specific uses and limits, and carry those uses into the outcome. Prefer a recognisable customer and one visible change over a paragraph explaining unfamiliar lore.

For the next slice, one optional opening among the day's three visitors is enough. Keep a visible “continue ordinary trade” route. Longer conversation happens on demand; arithmetic and checkout retain their own space. A brief note of an accepted promise is useful, but a quest log, map markers, story currency and relationship meters are not required.

## Three connected examples

These share a local Lantern Walk, an evening when neighbours walk home together under borrowed lights. The event happens with or without the player's help. Each example stands alone; none requires completing another to keep trading.

### 1. Fen, the lamplighter — a commercial promise

Fen is buying the usual stock and admits that the communal lantern has begun attracting moths “with opinions”. They ask whether the shop could have one bottle of starlight available on tomorrow's visit. They will buy it at the shop's displayed price, subject to the normal price rules.

The player may promise to try, defer until evening planning, or decline. A promise is a reminder, not an automatic stock reservation. Ordering and arranging tomorrow's stock remain the player's work. On the day-two return, completing an actual starlight purchase lets Fen test it in a small lamp by the shop entrance: “There. Finally, a light that doesn’t send directions to moths.” The lamp changes visibly during that visit, and Fen offers one modest merchandise bonus. The two-day slice gets its payoff immediately; a later account of the walk belongs to a future arc.

If no promise was made, Fen remains an ordinary customer. If a promise was made but the product cannot be supplied, Fen says they borrowed the bakery's lantern instead: “Payment was in buns. Complicated exchange rate.” No fine or loss of future ordinary custom follows. The player can still be curious about the walk.

This is the recommended first encounter: it connects a remembered person and product use across the existing two days with little new machinery.

### 2. Nessa, the printer — what belongs in the shop

Nessa buys whisper ink for route cards and asks whether a small Lantern Walk notice can stand on the counter. The player can display it, ask to see it later, or keep the counter clear. This is a choice about the shop's appearance and involvement, rather than donating stock.

On a later visit, a courier mentions seeing the notice; without it, they heard through the bakery. Nessa's next purchase and conversation reflect how the cards turned out. Participation might lead to a later offer of custom shelf signs at a disclosed price, rather than a permanent profit multiplier.

This is a later example, not a requirement for the first build. It would add one removable display object and a small follow-up, not a general decorating system.

### 3. Ivo, the courier — a regular who remembers

Ivo buys dawn tonic for an early route. There is no special request. On a later visit, a short optional remark notices Fen's trial lamp outside the shop, or mentions the borrowed bakery lantern, depending on what happened. After the walk, a further ordinary visit could recall the event itself. Ivo might then mention an eccentric customer on the route who could become a future regular.

There is no reward for listening and no task to accept. Its purpose is to make a normal sale feel connected to life beyond the door. A recurring cast needs these uneventful visits as well as explicit opportunities.

## How the examples become a strategy

An arc should usually need two to four visits: establish a person through trade, offer an opportunity where appropriate, show an outcome, then let that person return to ordinary shopping. Shared events allow a few authored references between arcs. Avoid a chain in which every declined request blocks the next character's story.

Use a small cast before adding breadth. Each regular should have a recognisable voice, an ordinary shopping pattern, a personal concern and a reason to return. Warm, specific humour suits this shop; interchangeable fantasy errands and constant crises would weaken it.

For each proposed arc, write a short card containing: the character's ordinary trade; the player-facing opening; the retail choice; the commitment and terms; the fulfilled, declined and unfulfilled outcomes; the later visible or conversational change; and the economic effect. If the card needs a new subsystem, identify that cost explicitly. Do not build a general quest engine before the first arc needs one.

## First playable slice and evidence to collect

The first build implements Fen's day-one offer and day-two return, with one small visible consequence and a modest stock bonus. Use existing visits and normal checkout. Do not add an extra timed customer, a forced conversation, an automatic discount, a new product catalogue or mandatory stock reservation. At default prices the ordinary route must retain its existing customer allocation and ability to fund the shop. The bonus is one tonic bottle; tune it so the story is attractive without making participation the economic default.

A fulfilled purchase must use real stock and complete payment. Accepted-promise state must survive the transition to the saved day-two morning. Outcome and bonus claims must resolve once; a full delivery tray must leave a bonus safely pending. Restarting the morning must roll back the story and stock together. A deferred or declined offer must leave ordinary trade intact. These conditions are covered by encounter regression checks. Narrative tone and the feel of participation still need a human playtest.

In the playtest, look for whether the player remembers Fen and their concern, notices or asks about the return, connects stocking to the outcome, and wants to know more about a regular. Also watch whether they can ignore the offer comfortably and whether conversation interferes with the work they already enjoy. Economic viability and inventory safety remain automated checks.

One successful encounter demonstrates this particular delivery and tone. It cannot establish the whole narrative strategy; one weak encounter cannot disprove it either. Before expanding into a campaign, try at least one different form—such as Nessa's display choice or Ivo's ordinary return—and judge the cumulative effect. The restocker remains planned, but story identity is the next priority.
