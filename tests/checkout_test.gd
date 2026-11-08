extends SceneTree

var assertions := 0
var failures := 0

func _initialize() -> void:
	_run.call_deferred()

func _check(ok: bool, message: String) -> void:
	assertions += 1
	if not ok:
		failures += 1
		push_error(message)

func _run() -> void:
	await _sale_rules()
	await _shop_flow()
	await _competing_pickup()
	await _empty_shelves()
	print("CHECKOUT: %d assertions, %d failures" % [assertions, failures])
	quit(1 if failures else 0)

func _sale_rules() -> void:
	var shelf := StockSlot.new()
	shelf.is_shelf = true
	root.add_child(shelf)
	var basket := Node3D.new()
	root.add_child(basket)
	var item := StockItem.new().configure("tonic", "Dawn tonic", Color.GREEN)
	shelf.seed(item)
	var sale := CounterSale.new(20)
	_check(sale.take(shelf, 10, basket), "Take physical shelf item")
	_check(shelf.item == null and item.get_parent() == basket, "Basket takes sole ownership")
	_check(not sale.take(shelf, 10, basket) and sale.items.size() == 1, "Competing pickup cannot duplicate item")
	_check(not sale.confirm().ok, "Cannot pay before agreeing total")
	_check(not sale.add_coin(1), "Cannot dispense before payment")
	for text in ["", "10.0", "ten", "-10", "11"]:
		_check(not sale.submit_total(text).ok, "Reject invalid or incorrect total: " + text)
	_check(sale.submit_total("10").ok, "Accept correct total")
	_check(not sale.confirm().ok and not sale.completed, "Underchange leaves sale open")
	_check(not sale.add_coin(5) and not sale.remove_coin(100), "Only valid coins can enter or leave tray")
	for i in range(11):
		sale.add_coin(1)
	_check(not sale.confirm().ok and sale.coins[1] == 11, "Overchange preserves coins for correction")
	sale.remove_coin(1)
	_check(sale.confirm().ok, "Ten bits accepted instead of one mark")
	_check(not sale.confirm().ok and not sale.add_coin(100), "Completed transaction cannot charge or dispense again")
	_check(sale.items.size() == 1 and item.get_parent() == basket, "Retries conserve physical item")
	shelf.free()
	basket.free()
	await process_frame

func _stock(shop: Node) -> void:
	# Fixture preparation; actual player transfers remain covered by shop_scene_test.
	for source: StockSlot in shop.slots:
		if source.is_shelf or source.item == null:
			continue
		for shelf: StockSlot in shop.shelf_slots:
			if shelf.item == null and shelf.accepts(source.item):
				var item := source.item
				source.item = null
				item.reparent(shelf, false)
				shelf.item = item
				item.transform = Transform3D.IDENTITY
				break
	shop._update_stock_count()

func _wait_for_counter(shop: Node) -> void:
	for i in range(1000):
		if shop.customer == null or shop.customer.state == "waiting":
			break
		await process_frame
	_check(shop.customer != null and shop.customer.state == "waiting", "Customer browses and reaches counter")

func _wait_for_departure(shop: Node) -> void:
	for i in range(1000):
		if shop.customer == null:
			break
		await process_frame
	_check(shop.customer == null, "Customer leaves and visit becomes available")

func _shop_flow() -> void:
	var shop = load("res://scenes/shop.tscn").instantiate()
	shop.persistence_enabled = false
	root.add_child(shop)
	shop._start()
	shop.open_trading()
	_stock(shop)
	# Verify the real counter can be aimed at from the customer-facing side.
	shop.player.position = Vector3(2.75, 0.05, 2.6)
	shop.player.camera.look_at(shop.counter_body.global_position)
	await physics_frame
	await physics_frame
	_check(shop.player.aimed_body() == shop.counter_body, "Counter ledger is reachable through player aiming")
	var interact := InputEventAction.new()
	interact.action = "interact"
	interact.pressed = true
	shop._unhandled_input(interact)
	_check(shop.checkout.visible and shop.checkout.invite.visible, "E opens ledger and invitation")
	await _snapshot("counter-invitation")
	var expected_till := 0
	for visit in range(3):
		shop.checkout.invite.pressed.emit()
		_check(shop.customer != null and not shop.checkout.visible, "Invite closes ledger to show visitor")
		_check(not shop.invite_customer(), "Cannot invite duplicate simultaneous customer")
		shop.customer.speed = 1000
		await _wait_for_counter(shop)
		if visit == 0:
			await _snapshot("customer-at-counter")
		var sale: CounterSale = shop.customer.sale
		shop.customer._process(3600)
		_check(shop.customer.state == "waiting" and not sale.completed and shop.visits_invited == visit + 1, "Waiting has no timeout or effect on visit allocation")
		_check(sale.items.size() == visit + 1, "Customer basket has requested physical items")
		shop._open_checkout()
		if visit == 0:
			await _snapshot("basket-ledger")
		shop.checkout.total_entry.text = "999"
		shop.checkout.agree.pressed.emit()
		_check(not sale.total_accepted and shop.till == expected_till, "Incorrect total costs nothing")
		shop.checkout.total_entry.text = str(sale.total())
		shop.checkout.agree.pressed.emit()
		_check(sale.total_accepted, "Ledger accepts entered basket total")
		_check(not shop.complete_sale() and shop.till == expected_till, "Wrong change cannot complete or credit sale")
		# Key press dispenses once; hold repeats; release and leaving stop repeat.
		var key := InputEventKey.new()
		key.physical_keycode = KEY_1
		key.pressed = true
		shop.checkout._input(key)
		_check(sale.coins[1] == 1, "Keyboard coin adds once")
		shop.checkout._process(0.39)
		_check(sale.coins[1] == 1, "Holding has initial delay")
		shop.checkout._process(0.02)
		_check(sale.coins[1] == 2, "Holding repeats after delay")
		key.pressed = false
		shop.checkout._input(key)
		shop.checkout._process(1)
		_check(sale.coins[1] == 2, "Key release stops dispensing")
		shop.checkout._begin_hold(10)
		shop.checkout._notification(NOTIFICATION_APPLICATION_FOCUS_OUT)
		shop.checkout._process(1)
		_check(sale.coins[10] == 1 and shop.checkout.held_value == 0, "Losing focus stops pouch repeat")
		sale.remove_coin(10)
		shop.checkout.tray_coins[0].pressed.emit()
		_check(sale.coins[1] == 1, "Click tray stack removes one coin")
		shop.checkout.pouches[0].button_down.emit()
		shop._close_checkout()
		shop.checkout._process(1)
		_check(sale.coins[1] == 2 and shop.checkout.held_value == 0, "Leaving counter stops held pouch")
		shop._open_checkout()
		_check(shop.checkout.sale == sale and sale.coins[1] == 2 and sale.total_accepted, "Returning preserves transaction and coins")
		shop._pause()
		_check(paused and not shop.checkout.visible, "Pause suspends ledger")
		shop._start()
		_check(not paused and shop.checkout.visible and shop.checkout.sale == sale, "Resume restores pending ledger")
		while sale.coins[1] > 0:
			sale.remove_coin(1)
		var change := sale.payment - sale.total()
		for value: int in [100, 10, 1]:
			while change >= value:
				sale.add_coin(value)
				change -= value
		expected_till += sale.total()
		shop.checkout.refresh()
		if visit == 2:
			await _snapshot("change-tray")
		shop.checkout.confirm.pressed.emit()
		_check(sale.completed and shop.till == expected_till and shop.sales_completed == visit + 1, "Correct change commits sale exactly once")
		_check(not shop.complete_sale() and shop.till == expected_till, "Repeated confirm cannot duplicate revenue")
		await _wait_for_departure(shop)
	_check(shop.visits_invited == 3 and not shop.invite_customer(), "Fixed visit allocation is independent of elapsed time")
	for slot: StockSlot in shop.slots:
		_check(slot.item == null, "All six sold items leave shop stock")
	_check(shop.till == 140, "Revenue equals six shelf prices")
	shop.free()
	await process_frame

func _empty_shelves() -> void:
	var shop = load("res://scenes/shop.tscn").instantiate()
	shop.persistence_enabled = false
	root.add_child(shop)
	shop._start()
	shop.open_trading()
	var remarks: Array[String] = []
	shop.invite_customer()
	shop.customer.remarked.connect(func(message: String) -> void: remarks.append(message))
	shop.customer.speed = 1000
	await _wait_for_departure(shop)
	_check(not remarks.is_empty(), "Missing shelf stock receives explicit customer feedback")
	_check(shop.till == 0 and shop.sales_completed == 0, "Empty shelf cannot sell delivery stock")
	var remaining := 0
	for slot: StockSlot in shop.slots:
		if slot.item != null:
			remaining += 1
	_check(remaining == 6, "Unavailable visit leaves delivery items untouched")
	shop.free()
	await process_frame


func _snapshot(name: String) -> void:
	if not "--capture-checkout" in OS.get_cmdline_user_args():
		return
	await process_frame
	await RenderingServer.frame_post_draw
	var folder := ProjectSettings.globalize_path("res://builds/screenshots")
	DirAccess.make_dir_recursive_absolute(folder)
	_check(root.get_texture().get_image().save_png(folder.path_join(name + ".png")) == OK, "Save checkout screenshot: " + name)


func _competing_pickup() -> void:
	var shop = load("res://scenes/shop.tscn").instantiate()
	shop.persistence_enabled = false
	root.add_child(shop)
	shop._start()
	shop.open_trading()
	_stock(shop)
	shop.invite_customer()
	shop.customer.speed = 1000
	for i in range(1000):
		if shop.customer.state == "browsing":
			break
		await process_frame
	shop.customer.set_process(false)
	var target: StockSlot = shop.customer.destination
	var item := target.item
	shop.player.position = Vector3(target.position.x, 0.05, -1.9)
	await physics_frame
	await physics_frame
	_check(ItemActions.interact(shop.player, target).ok, "Player can take targeted item before customer arrives")
	shop.customer.set_process(true)
	await _wait_for_counter(shop)
	_check(shop.customer.sale.items.size() == 1 and shop.customer.sale.items[0] != item, "Customer retries another shelf after competing pickup")
	_check(shop.player.held_item == item and item.get_parent() == shop.player.carry_anchor, "Customer cannot take or duplicate player-held stock")
	_check(target.item == null, "Competing shelf remains empty")
	shop.free()
	await process_frame
