# Fantasy Shopkeeper

A cosy magical shop where ordinary trade supports growth and optional encounters create additional opportunities.

## Language

**Product**:
A kind of merchandise with a shared identity and shelf price, such as dawn tonic.
_Avoid_: Item type, SKU

**Item**:
One physical unit of a product that can be carried, stocked, or sold.
_Avoid_: Product when referring to a single unit

**Stock**:
The items the shop owns, whether awaiting shelving or available for sale.

**Shelf slot**:
A designated place for one item of a specified product, available to customers once stocked.

**Delivery**:
Stock ordered in the evening and received the following morning.

**Restocker**:
A hired helper who carries items from the delivery area to their shelf slots.
_Avoid_: Automation when referring specifically to this worker

**Trading day**:
A fixed set of customer opportunities between opening and closing, with progress independent of the player's arithmetic speed.

**Preparation**:
The unlimited morning period for receiving deliveries and arranging the shop before opening.

**Evening planning**:
The unlimited period after serving the day's customers for ordering stock and arranging labour.

**Basket total**:
The price of all items in one customer's purchase, calculated by the shopkeeper.

**Change tray**:
The coins assembled to return the difference between the customer's payment and basket total.

**Recovery loan**:
Credit intended to fund another one or two days of basic stock when the shop cannot otherwise continue trading.

**Special request**:
An optional customer opportunity with a later outcome and possible bonus reward, independent of ordinary progression.

## Production design terms

These terms describe the proposed brewing game; the systems are not all implemented in the Godot prototype.

**Ingredient**:
A raw material consumed in a recipe, measured in explicit units or portions.

**Recipe**:
Defined ingredient quantities and preparation steps that produce a known product or authored variation.

**Batch**:
One preparation operation with a defined material cost and number of physical output items.

**Bespoke order**:
A customer agreement specifying the needed product or effect, quantity, price and collection terms. It does not automatically reserve physical stock.
