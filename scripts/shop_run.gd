class_name ShopRun
extends RefCounted

const BASE_PRICES := {"tonic": 12, "ink": 23, "light": 35}
const COSTS := {"tonic": 5, "ink": 9, "light": 14}
const DAILY_COST := 12
const LOAN_CASH := 112
const LOAN_DUE := 118
const VISITS_PER_DAY := 3
var day := 1
var phase := "preparation"
var cash := 24
var debt := 0
var loan_taken := false
var prices: Dictionary = BASE_PRICES.duplicate()
var visits_started := 0
var visits_resolved := 0
var sales := 0
var revenue := 0
var expenses := 0
var stock_spend := 0
var repayments := 0
var order_paid := false
var pending_order := {"tonic": 0, "ink": 0, "light": 0}
var booked: Array[int] = []
var previous_day: Dictionary = {}

func open_shop() -> bool:
	if phase != "preparation":
		return false
	phase = "trading"
	return true

func start_visit() -> bool:
	if phase != "trading" or visits_started != visits_resolved or visits_started >= VISITS_PER_DAY:
		return false
	visits_started += 1
	return true

func book_sale(amount: int) -> bool:
	if phase not in ["trading", "closing"] or visits_started <= visits_resolved or booked.has(visits_started) or amount < 0:
		return false
	booked.append(visits_started)
	revenue += amount
	cash += amount
	sales += 1
	return true

func resolve_visit() -> bool:
	if visits_resolved >= visits_started:
		return false
	visits_resolved += 1
	if phase == "closing":
		_finish_close()
	return true

func close_shop() -> bool:
	if phase != "trading":
		return false
	phase = "closing"
	if visits_started == visits_resolved:
		_finish_close()
	return true

func _finish_close() -> void:
	phase = "evening"
	expenses = DAILY_COST
	cash -= DAILY_COST

func set_price(product: String, amount: int) -> bool:
	if phase not in ["preparation", "evening"] or not prices.has(product) or amount < 1 or amount > 999:
		return false
	prices[product] = amount
	return true

func order_cost(quantities: Dictionary) -> int:
	var amount := 0
	for product in COSTS:
		amount += int(quantities.get(product, 0)) * COSTS[product]
	return amount

func place_order(quantities: Dictionary, capacity: int) -> Dictionary:
	if phase != "evening" or day >= 2 or order_paid:
		return _result(false, "There is no unpaid delivery to order tonight.")
	var count := 0
	for product in COSTS:
		var quantity = quantities.get(product, -1)
		if not _whole(quantity) or quantity < 0 or quantity > 8:
			return _result(false, "Choose between 0 and 8 bottles of each product.")
		count += int(quantity)
	if count > capacity:
		return _result(false, "Only %d delivery spaces are free. Unsold delivery stock stays here." % capacity)
	var cost := order_cost(quantities)
	if cost > cash:
		return _result(false, "This order costs %d bits; you have %d. Reduce it or consider the recovery loan." % [cost, cash])
	cash -= cost
	stock_spend += cost
	for product in COSTS:
		pending_order[product] = int(quantities[product])
	order_paid = true
	return _result(true, "Order paid once. Delivery arrives next morning.")

func borrow() -> bool:
	if phase != "evening" or day >= 2 or loan_taken or cash >= 68:
		return false
	loan_taken = true
	cash += LOAN_CASH
	debt += LOAN_DUE
	return true

func repay(amount: int) -> bool:
	if phase != "evening" or amount <= 0 or amount > cash or amount > debt:
		return false
	cash -= amount
	debt -= amount
	repayments += amount
	return true

func advance_morning() -> Dictionary:
	if phase != "evening" or day >= 2 or not order_paid:
		return _result(false, "Confirm and pay the order first, even if ordering zero bottles.")
	var shipment := pending_order.duplicate()
	previous_day = {"day": day, "sales": sales, "revenue": revenue, "expenses": expenses, "stock_spend": stock_spend, "repayments": repayments}
	day += 1
	phase = "preparation"
	visits_started = 0
	visits_resolved = 0
	sales = 0
	revenue = 0
	expenses = 0
	stock_spend = 0
	repayments = 0
	booked.clear()
	order_paid = false
	pending_order = {"tonic": 0, "ink": 0, "light": 0}
	return {"ok": true, "message": "Morning delivery received.", "shipment": shipment}

func finish() -> bool:
	if phase != "evening" or day != 2:
		return false
	phase = "complete"
	return true

# Only morning checkpoints are persisted; a reload never resumes half a sale.
func morning_state() -> Dictionary:
	return {"day": day, "phase": phase, "cash": cash, "debt": debt, "loan_taken": loan_taken, "prices": prices.duplicate(), "previous_day": previous_day.duplicate()}

static func from_morning(data: Dictionary) -> ShopRun:
	if data.get("phase") != "preparation" or not _whole(data.get("day")) or int(data.day) not in [1, 2]:
		return null
	if not _whole(data.get("cash")) or data.cash < -100000 or data.cash > 1000000:
		return null
	if not _whole(data.get("debt")) or data.debt < 0 or data.debt > LOAN_DUE or not data.get("loan_taken") is bool:
		return null
	if data.debt > 0 and not data.loan_taken:
		return null
	if not data.get("prices") is Dictionary or not data.get("previous_day") is Dictionary:
		return null
	for product in BASE_PRICES:
		if not _whole(data.prices.get(product)) or data.prices[product] < 1 or data.prices[product] > 999:
			return null
	var run := ShopRun.new()
	run.day = int(data.day)
	run.cash = int(data.cash)
	run.debt = int(data.debt)
	run.loan_taken = data.loan_taken
	for product in BASE_PRICES:
		run.prices[product] = int(data.prices[product])
	run.previous_day = data.previous_day.duplicate()
	return run

static func _whole(value: Variant) -> bool:
	return value is int or (value is float and is_finite(value) and value == floor(value))

func _result(ok: bool, message: String) -> Dictionary:
	return {"ok": ok, "message": message}
