extends "checkout_test.gd"

const TEST_SAVE := "user://day-loop-regression-morning.json"

func _run() -> void:
	_model_rules()
	await _two_days()
	await _recovery_and_saves()
	await _price_rejection()
	await _closing_customer()
	print("DAY LOOP: %d assertions, %d failures" % [assertions, failures])
	quit(1 if failures else 0)

func _model_rules() -> void:
	var state := ShopRun.new()
	_check(not state.start_visit(), "Cannot trade during preparation")
	_check(state.set_price("ink", 25) and not state.set_price("ink", 0), "Prices editable with positive bounds during preparation")
	_check(state.open_shop() and not state.open_shop(), "Explicit opening happens once")
	_check(not state.set_price("ink", 30), "Shelf prices lock during trading")
	_check(state.start_visit() and not state.start_visit(), "Only one unresolved visit at a time")
	_check(state.close_shop() and state.phase == "closing" and state.cash == 24, "Close waits for current customer without early charge")
	_check(not state.start_visit(), "Closing stops further arrivals")
	_check(state.book_sale(12) and not state.book_sale(12), "Closing sale credits revenue only once")
	_check(state.resolve_visit() and state.phase == "evening" and state.cash == 24, "Departure settles one daily expense")
	_check(not state.close_shop() and not state.resolve_visit() and state.cash == 24, "Repeated close/departure cannot charge again")
	_check(not state.advance_morning().ok, "Morning cannot begin before order confirmation")
	var quantities := {"tonic": 2, "ink": 2, "light": 2}
	_check(not state.place_order(quantities, 8).ok and state.cash == 24, "Unaffordable order leaves funds unchanged")
	_check(state.borrow() and state.cash == 136 and state.debt == 118, "Recovery terms match cash and fixed debt")
	_check(not state.borrow(), "Loan cannot be duplicated")
	_check(not state.place_order(quantities, 2).ok and not state.order_paid, "Delivery capacity checked before charging")
	_check(not state.place_order({"tonic": -1, "ink": 0, "light": 0}, 8).ok, "Negative order rejected")
	_check(not state.place_order({"tonic": 1.5, "ink": 0, "light": 0}, 8).ok, "Fractional bottles rejected")
	_check(state.place_order(quantities, 8).ok and state.cash == 80, "Supplier order charged at 5/9/14 costs")
	_check(not state.place_order(quantities, 8).ok and state.cash == 80, "Repeated order does not debit twice")
	_check(state.repay(10) and state.cash == 70 and state.debt == 108, "Voluntary repayment reduces cash and debt equally")
	_check(not state.repay(71) and state.debt == 108, "Cannot repay more cash than available")
	var morning := state.advance_morning()
	_check(morning.ok and morning.shipment == quantities and state.day == 2 and state.phase == "preparation", "One transition produces one paid shipment")
	_check(not state.advance_morning().ok and state.cash == 70, "Repeated advance cannot duplicate delivery or money")
	var restored := ShopRun.from_morning(state.morning_state())
	_check(restored != null and restored.debt == 108 and restored.cash == 70 and restored.prices.ink == 25, "Morning restores funds, prices and debt")
	_check(restored.loan_taken and restored.visits_started == 0 and not restored.order_paid, "Morning has no pending charges or replayable order")
	var bad := state.morning_state()
	bad.prices.ink = -2
	_check(ShopRun.from_morning(bad) == null, "Invalid morning model rejected before use")

func _new_shop() -> Node:
	var shop = load("res://scenes/shop.tscn").instantiate()
	shop.persistence_enabled = false
	root.add_child(shop)
	shop._start()
	return shop

func _serve_all(shop: Node) -> void:
	shop._open_checkout()
	shop.day_panel.buttons.open.pressed.emit()
	_check(shop.run.phase == "trading", "Planning panel explicitly opens shop")
	for visit in range(3):
		_check(shop.invite_customer(), "Invite allocated customer")
		shop.customer.speed = 1000
		await _wait_for_counter(shop)
		shop._open_checkout()
		var sale: CounterSale = shop.customer.sale
		_check(sale.submit_total(str(sale.total())).ok, "Customer payment covers current shelf prices")
		var change := sale.payment - sale.total()
		for value: int in [100, 10, 1]:
			while change >= value:
				sale.add_coin(value)
				change -= value
		_check(shop.complete_sale(), "Sale commits within current day's finances")
		await _wait_for_departure(shop)
	shop._open_checkout()
	shop.checkout.close_day.pressed.emit()
	_check(shop.run.phase == "evening" and shop.day_panel.visible, "Closing opens evening planning")

func _two_days() -> void:
	var shop := _new_shop()
	_check(not shop.invite_customer() and shop.run.phase == "preparation", "Preparation has unlimited time and no arrivals")
	_stock(shop)
	await _serve_all(shop)
	_check(shop.run.cash == 152 and shop.run.revenue == 140 and shop.run.expenses == 12, "Day one cash reconciles starting cash, sales and expense")
	await _day_snapshot("evening-plan")
	for product in shop.day_panel.order_fields:
		shop.day_panel.order_fields[product].value = 2
	shop._pause()
	_check(paused and not shop.day_panel.visible, "Pause suspends evening planning")
	shop._start()
	_check(shop.day_panel.visible and shop.day_panel.quantities().tonic == 2 and shop.run.cash == 152, "Resume preserves unpaid order draft without spending cash")
	shop.day_panel.buttons.buy.pressed.emit()
	_check(shop.run.order_paid and shop.run.cash == 96, "Evening panel buys a six-bottle delivery")
	await _day_snapshot("paid-order")
	shop.day_panel.buttons.advance.pressed.emit()
	_check(shop.run.day == 2 and shop.run.phase == "preparation" and shop.run.cash == 96, "Advance resets day counters without replaying expense")
	var count := _count_stock(shop)
	_check(count == 6 and shop.visits_invited == 0 and shop.run.revenue == 0, "Shipment appears once and new visit allocation resets")
	shop._advance_morning()
	_check(_count_stock(shop) == count and shop.run.day == 2, "Repeated sleep cannot deliver twice")
	await _day_snapshot("day-two-morning")
	_stock(shop)
	await _serve_all(shop)
	_check(shop.run.cash == 224 and shop.run.debt == 0, "Ordinary two-day trading remains profitable without borrowing")
	shop.day_panel.buttons.finish.pressed.emit()
	_check(shop.run.phase == "complete" and not shop.invite_customer(), "Two-day experiment ends explicitly without extra arrivals")
	shop.free()
	await process_frame

func _count_stock(shop: Node) -> int:
	var count := 0
	for slot: StockSlot in shop.slots:
		if is_instance_valid(slot.item):
			count += 1
	return count

func _recovery_and_saves() -> void:
	if FileAccess.file_exists(TEST_SAVE):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(TEST_SAVE))
	var shop := _new_shop()
	shop.open_trading()
	shop._day_action("close")
	_check(shop.run.cash == 12, "Poor first day still charges disclosed expense")
	await _day_snapshot("recovery-loan")
	shop._day_action("borrow")
	_check(shop.run.cash == 124 and shop.run.debt == 118, "Recovery funds are sufficient for another basic delivery")
	# Existing delivery stock uses six spaces. Ordering six more is refused.
	for product in shop.day_panel.order_fields:
		shop.day_panel.order_fields[product].value = 2
	shop._day_action("buy")
	_check(not shop.run.order_paid and shop.run.cash == 124, "Unsold delivery stock limits a new order")
	_stock(shop)
	shop._open_checkout()
	shop._day_action("buy")
	_check(shop.run.order_paid and shop.run.cash == 68, "Recovery allows restocking after freeing delivery tray")
	var held_slot: StockSlot = shop.shelf_slots[0]
	var held := held_slot.item
	held_slot.item = null
	held.reparent(shop.player.carry_anchor, false)
	shop.player.held_item = held
	_check("carrying" in shop._advance_morning() and shop.run.day == 1, "Sleeping cannot lose a carried bottle")
	shop.player.held_item = null
	held.reparent(held_slot, false)
	held_slot.item = held
	# Revalidate capacity again at delivery, not just when buying.
	var shelf: StockSlot = shop.shelf_slots[0]
	var delivery: StockSlot = shop.slots[6]
	var item := shelf.item
	shelf.item = null
	item.reparent(delivery, false)
	delivery.item = item
	for i in range(2):
		var another: StockSlot = shop.shelf_slots[i + 1]
		var target: StockSlot = shop.slots[i + 7]
		var moved := another.item
		another.item = null
		moved.reparent(target, false)
		target.item = moved
	var blocked: String = shop._advance_morning()
	_check(shop.run.day == 1 and "delivery spaces" in blocked and shop.run.order_paid, "Moved stock cannot silently consume paid delivery")
	_stock(shop)
	shop.persistence_enabled = true
	shop.save_path = TEST_SAVE
	shop._advance_morning()
	_check(shop.run.day == 2 and FileAccess.file_exists(TEST_SAVE) and _count_stock(shop) == 12, "Morning snapshot includes retained stock and new delivery once")
	var saved_cash: int = shop.run.cash
	var saved_text := FileAccess.get_file_as_string(TEST_SAVE)
	shop.free()
	await process_frame
	var resumed = load("res://scenes/shop.tscn").instantiate()
	resumed.save_path = TEST_SAVE
	root.add_child(resumed)
	await _day_snapshot("resumed-morning")
	_check(resumed.run.day == 2 and resumed.run.cash == saved_cash and resumed.run.debt == 118, "Fresh scene resumes saved morning finances")
	_check(_count_stock(resumed) == 12 and resumed.run.loan_taken and resumed.visits_invited == 0, "Reload restores physical stock without duplicate delivery, loan or visits")
	resumed._start()
	await _serve_all(resumed)
	resumed._day_action("repay")
	_check(resumed.run.debt == 0 and resumed.run.cash == 78 and _count_stock(resumed) == 6, "Recovered second-day trade can repay fixed loan and retain working stock")
	resumed._load_morning()
	_check(_count_stock(resumed) == 12 and resumed.run.cash == saved_cash, "Repeated load replaces rather than adds stock")
	_check(not MorningSave.write_file("user://missing-test-directory/morning.json", {}), "Unwritable destination reports save failure")
	_check(FileAccess.get_file_as_string(TEST_SAVE) == saved_text, "Failed separate save leaves original checkpoint intact")
	var invalid: Dictionary = JSON.parse_string(saved_text)
	invalid.stock[0].product = "unknown-product"
	MorningSave.write_file(TEST_SAVE, invalid)
	var before := _count_stock(resumed)
	_check(not resumed._load_morning() and _count_stock(resumed) == before and resumed.run.cash == saved_cash, "Invalid checkpoint rejected without partial application")
	_check(FileAccess.get_file_as_string(TEST_SAVE) != saved_text, "Invalid file preserved for explicit fresh-start decision")
	resumed.free()
	var corrupt_text := FileAccess.get_file_as_string(TEST_SAVE)
	var rejected = load("res://scenes/shop.tscn").instantiate()
	rejected.save_path = TEST_SAVE
	root.add_child(rejected)
	_check(rejected.run.day == 1 and _count_stock(rejected) == 6, "Invalid startup save does not partly restore another morning")
	_check(FileAccess.get_file_as_string(TEST_SAVE) == corrupt_text, "Startup does not overwrite an invalid existing save")
	_check(not rejected._save_morning() and FileAccess.get_file_as_string(TEST_SAVE) == corrupt_text, "Later autosave cannot replace an invalid checkpoint without fresh-start consent")
	rejected.free()
	DirAccess.remove_absolute(ProjectSettings.globalize_path(TEST_SAVE))
	await process_frame

func _price_rejection() -> void:
	var shop := _new_shop()
	_stock(shop)
	shop._set_shelf_price("tonic", 999)
	shop.open_trading()
	shop.invite_customer()
	shop.customer.speed = 1000
	var remarks: Array[String] = []
	shop.customer.remarked.connect(func(message: String) -> void: remarks.append(message))
	await _wait_for_departure(shop)
	_check(shop.run.revenue == 0 and _count_stock(shop) == 6, "Unreasonable price loses purchase without consuming item")
	_check(remarks.any(func(message: String) -> bool: return "price" in message), "Price rejection is explicit to the player")
	shop.free()
	await process_frame
	var premium := _new_shop()
	_stock(premium)
	premium._set_shelf_price("tonic", 24)
	premium.open_trading()
	premium.invite_customer()
	premium.customer.speed = 1000
	await _wait_for_counter(premium)
	_check(premium.customer.sale.total() == 24 and premium.customer.sale.payment >= 24, "Accepted higher price receives enough payment")
	premium.free()
	await process_frame

func _day_snapshot(name: String) -> void:
	if not "--capture-days" in OS.get_cmdline_user_args():
		return
	await process_frame
	await RenderingServer.frame_post_draw
	var folder := ProjectSettings.globalize_path("res://builds/screenshots")
	DirAccess.make_dir_recursive_absolute(folder)
	_check(root.get_texture().get_image().save_png(folder.path_join(name + ".png")) == OK, "Save day-loop screenshot")


func _closing_customer() -> void:
	var shop := _new_shop()
	_stock(shop)
	shop.open_trading()
	shop.invite_customer()
	shop.customer.speed = 1000
	await _wait_for_counter(shop)
	shop._open_checkout()
	shop.checkout.close_day.pressed.emit()
	_check(shop.run.phase == "closing" and shop.run.expenses == 0 and not shop.invite_customer(), "Close button allows current customer to finish and stops new arrivals")
	var sale: CounterSale = shop.customer.sale
	sale.submit_total(str(sale.total()))
	for i in range(sale.payment - sale.total()):
		sale.add_coin(1)
	_check(shop.complete_sale(), "Pending customer can still pay during closing")
	await _wait_for_departure(shop)
	_check(shop.run.phase == "evening" and shop.run.cash == 24 and shop.run.expenses == 12, "World departure charges exactly one expense after closing sale")
	shop.free()
	await process_frame
