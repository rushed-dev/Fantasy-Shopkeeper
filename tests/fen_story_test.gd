extends "day_loop_test.gd"

var captured_names: Dictionary = {}

const FEN_SAVE := "user://fen-story-regression-morning.json"

func _run() -> void:
	_model_story()
	await _successful_promise()
	await _ordinary_routes()
	await _missed_promise()
	await _legacy_morning()
	print("FEN STORY: %d assertions, %d failures" % [assertions, failures])
	quit(1 if failures else 0)

func _model_story() -> void:
	var story := FenStory.new()
	_check(not story.offer(2, "fen") and not story.offer(1, "ordinary"), "Only Fen's first-day visit offers the story")
	_check(story.offer(1, "fen") and not story.offer(1, "fen"), "Question offered once")
	_check(story.choose("defer", 1) and story.choose("promise", 1), "Deferred question can be accepted before bedtime")
	_check(not story.choose("decline", 1) and not story.choose("promise", 2), "Commitment cannot be accidentally changed by repeat clicks or after bedtime")
	var sale := CounterSale.new(100)
	_check(not story.paid_visit(sale, 2, "fen") and story.reward == "none", "Unpaid basket cannot fulfill a promise")
	_check(not story.claim_bonus(), "No bonus before fulfillment")
	var root_basket := Node3D.new()
	root.add_child(root_basket)
	var shelf := StockSlot.new()
	shelf.is_shelf = true
	root.add_child(shelf)
	shelf.seed(StockItem.new().configure("light", "Bottled starlight", Color.GOLD, true))
	sale.take(shelf, 35, root_basket)
	sale.submit_total("35")
	_check(not story.paid_visit(sale, 2, "fen"), "Agreeing total still does not fulfill the promise")
	for i in range(6): sale.add_coin(10)
	for i in range(5): sale.add_coin(1)
	_check(sale.confirm().ok, "Fixture completes ordinary paid purchase")
	_check(not story.paid_visit(sale, 1, "fen") and not story.paid_visit(sale, 2, "ordinary"), "Wrong day or customer cannot deliver the outcome")
	_check(story.paid_visit(sale, 2, "fen") and story.outcome == "fulfilled" and story.reward == "pending", "Paid physical starlight resolves once with one pending tonic")
	_check(not story.paid_visit(sale, 2, "fen") and not story.departed(2, "fen"), "Repeated visit callbacks cannot reset or duplicate fulfillment")
	_check(story.claim_bonus() and not story.claim_bonus(), "Bonus is committed once")
	var state := ShopRun.new().morning_state()
	state.erase("fen")
	_check(ShopRun.from_morning(state) != null and ShopRun.from_morning(state).fen.decision == "unseen", "Older morning saves remain compatible")
	state.fen = {"decision": "declined", "outcome": "fulfilled", "reward": "pending"}
	_check(ShopRun.from_morning(state) == null, "Contradictory saved story is rejected")
	root_basket.free()
	shelf.free()

func _pay_customer(shop: Node) -> void:
	shop._open_checkout()
	var sale: CounterSale = shop.customer.sale
	if shop.visits_invited == 3:
		await _fen_snapshot("fen-three-item-ledger")
	_check(sale.submit_total(str(sale.total())).ok, "Normal total is accepted for story visitor")
	var change := sale.payment - sale.total()
	for value: int in [100, 10, 1]:
		while change >= value:
			sale.add_coin(value)
			change -= value
	_check(shop.complete_sale(), "Story visitor pays through normal commit boundary")
	await _wait_for_departure(shop)

func _visit(shop: Node) -> void:
	_check(shop.invite_customer(), "Story uses an existing allocated visit")
	shop.customer.speed = 1000
	await _wait_for_counter(shop)

func _day_one(shop: Node, choice: String) -> void:
	_stock(shop)
	shop.open_trading()
	for i in range(3):
		await _visit(shop)
		if i == 1:
			_check(shop.customer.customer_id == "fen" and shop.customer.name_tag.visible, "Fen is the identifiable second ordinary visitor")
			shop._open_checkout()
			await _fen_snapshot("fen-ledger")
			_check(not shop.fen_panel.visible and shop.checkout.story_button.visible, "Optional question does not interrupt checkout")
			if choice != "ignore":
				shop.checkout.story_button.pressed.emit()
				_check(shop.fen_panel.visible and shop.fen_panel.promise.visible, "Question opens only on demand")
				await _fen_snapshot("fen-offer")
				shop._pause()
				shop._start()
				_check(shop.fen_panel.visible and not paused, "Pause resumes the conversation without making a choice")
				if choice == "defer":
					shop.fen_panel.defer.pressed.emit()
					_check(shop.run.fen.decision == "deferred" and not shop.fen_panel.visible, "Defer returns to ordinary trade with a quiet note")
				else:
					shop._choose_fen(choice)
					shop.fen_panel.hide()
					shop._open_checkout()
		await _pay_customer(shop)
	shop._day_action("close")
	_check(shop.run.cash == 152 and shop.run.visits_resolved == 3, "Choice does not change first-day stock, cash or customer allocation")

func _order_and_sleep(shop: Node) -> void:
	for product in shop.day_panel.order_fields:
		shop.day_panel.order_fields[product].value = 2
	shop._day_action("buy")
	shop._advance_morning()
	_check(shop.run.day == 2 and shop.run.cash == 96 and _count_stock(shop) == 6, "Ordinary second-day delivery remains unchanged")

func _successful_promise() -> void:
	if FileAccess.file_exists(FEN_SAVE):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(FEN_SAVE))
	var shop := _new_shop()
	await _day_one(shop, "defer")
	shop.day_panel.story_button.pressed.emit()
	shop.fen_panel.promise.pressed.emit()
	_check(shop.run.fen.decision == "promised", "Deferred note can be accepted during evening planning")
	shop.fen_panel.hide()
	shop._open_checkout()
	shop.persistence_enabled = true
	shop.save_path = FEN_SAVE
	_order_and_sleep(shop)
	var checkpoint := FileAccess.get_file_as_string(FEN_SAVE)
	_check(JSON.parse_string(checkpoint).run.fen.decision == "promised", "Day-two checkpoint stores the accepted promise")
	shop.free()
	await process_frame
	var resumed = load("res://scenes/shop.tscn").instantiate()
	resumed.save_path = FEN_SAVE
	root.add_child(resumed)
	resumed._start()
	_check(resumed.run.fen.decision == "promised" and resumed.fen_light.light_energy == 0, "Fresh scene resumes promise with lamp and reward unresolved")
	_stock(resumed)
	resumed.open_trading()
	await _visit(resumed)
	await _pay_customer(resumed)
	await _visit(resumed)
	resumed._open_checkout()
	var fen_sale: CounterSale = resumed.customer.sale
	_check(not resumed.complete_sale() and resumed.run.fen.reward == "none" and resumed.fen_light.light_energy == 0, "Failed arithmetic cannot activate story or reward")
	await _pay_customer(resumed)
	_check(fen_sale.completed and resumed.run.fen.outcome == "fulfilled" and resumed.fen_light.light_energy > 0, "Fen completes payment, walks to lamp and lights it")
	_check(resumed.fen_lamp_label.visible and resumed.run.revenue == 70 and resumed.run.fen.reward == "pending", "Visible consequence accompanies normal revenue and a pending gift")
	resumed._open_fen()
	await _fen_snapshot("fen-fulfilled")
	# Fill every delivery space before claiming: the gift must remain pending.
	for slot: StockSlot in resumed.slots:
		if not slot.is_shelf and slot.item == null:
			resumed._seed_product(slot, "tonic")
	var before := _count_stock(resumed)
	_check(not resumed._claim_fen_bonus() and resumed.run.fen.reward == "pending" and _count_stock(resumed) == before, "Full tray keeps reward pending without creating or losing an item")
	await _fen_snapshot("fen-full-tray")
	var space: StockSlot = resumed.slots[6]
	space.item.free()
	space.item = null
	_check(resumed._claim_fen_bonus() and resumed.run.fen.reward == "claimed" and space.item.product_id == "tonic", "Freed delivery space receives exactly one physical tonic")
	_check(not resumed._claim_fen_bonus() and _count_stock(resumed) == before, "Repeated claim cannot add another bottle")
	await _fen_snapshot("fen-thank-you")
	# Look back at the decorative lamp; it is not a stock item.
	resumed.fen_panel.hide()
	resumed.checkout.hide()
	resumed.day_panel.hide()
	resumed.player.position = Vector3(0, 0.05, 2.5)
	resumed.player.camera.look_at(resumed.fen_bulb.global_position)
	await _fen_snapshot("fen-lamp")
	_check(resumed._load_morning() and resumed.run.fen.decision == "promised" and resumed.run.fen.reward == "none", "Reload rolls story and gift back with the morning")
	_check(resumed.fen_light.light_energy == 0 and _count_stock(resumed) == 6 and resumed.run.cash == 96, "Reload removes claimed bonus and lamp outcome without duplicating money or delivery")
	_check(FileAccess.get_file_as_string(FEN_SAVE) == checkpoint, "Mid-day story actions do not overwrite the morning checkpoint")
	resumed.free()
	DirAccess.remove_absolute(ProjectSettings.globalize_path(FEN_SAVE))
	await process_frame

func _ordinary_routes() -> void:
	for choice: String in ["decline", "ignore", "defer"]:
		var shop := _new_shop()
		await _day_one(shop, choice)
		_order_and_sleep(shop)
		_check(shop.run.fen.decision == ("declined" if choice == "decline" else "ignored"), "Unaccepted question ends quietly at bedtime")
		_stock(shop)
		await _serve_all(shop)
		_check(shop.run.cash == 224 and shop.run.debt == 0 and shop.run.fen.reward == "none", "Ordinary two-day trade stays viable without participation")
		_check(shop.run.fen.outcome == "ordinary" and shop.fen_light.light_energy == 0, "No promise means no forced outcome or bonus")
		shop.free()
		await process_frame

func _missed_promise() -> void:
	for reason: String in ["no_stock", "price", "closed_early"]:
		var shop := _new_shop()
		await _day_one(shop, "promise")
		_order_and_sleep(shop)
		if reason != "no_stock":
			_stock(shop)
		if reason == "price":
			shop._set_shelf_price("light", 999)
		shop.open_trading()
		if reason == "closed_early":
			shop._day_action("close")
		else:
			# First visitor, then Fen. Missing all stock leaves without checkout.
			for visit in range(2):
				shop.invite_customer()
				shop.customer.speed = 1000
				if reason == "no_stock":
					await _wait_for_departure(shop)
				else:
					await _wait_for_counter(shop)
					await _pay_customer(shop)
		_check(shop.run.fen.outcome == "missed" and shop.run.fen.reward == "none" and shop.run.debt == 0, "Missed promise has no fine, gift or debt: " + reason)
		_check(shop.fen_light.light_energy == 0, "Missed promise leaves trial lamp unlit")
		if reason == "price":
			_check("buns" in shop.toast.text, "Partial paid basket acknowledges missed promise in a brief remark")
		shop._open_fen()
		_check("buns" in shop.fen_panel.copy.text, "Fallback acknowledges bakery solution without guilt")
		shop.free()
		await process_frame

func _fen_snapshot(name: String) -> void:
	if not "--capture-fen" in OS.get_cmdline_user_args() or captured_names.has(name):
		return
	captured_names[name] = true
	await process_frame
	await RenderingServer.frame_post_draw
	var folder := ProjectSettings.globalize_path("res://builds/screenshots")
	DirAccess.make_dir_recursive_absolute(folder)
	_check(root.get_texture().get_image().save_png(folder.path_join(name + ".png")) == OK, "Save Fen screenshot")


func _legacy_morning() -> void:
	var legacy := _new_shop()
	legacy.run.day = 2
	legacy.run.cash = 96
	legacy.persistence_enabled = true
	legacy.save_path = FEN_SAVE
	legacy._save_morning()
	var data: Dictionary = JSON.parse_string(FileAccess.get_file_as_string(FEN_SAVE))
	data.run.erase("fen")
	MorningSave.write_file(FEN_SAVE, data)
	legacy.free()
	await process_frame
	var resumed = load("res://scenes/shop.tscn").instantiate()
	resumed.save_path = FEN_SAVE
	root.add_child(resumed)
	_check(not resumed.save_blocked and resumed.run.day == 2 and resumed.run.cash == 96 and _count_stock(resumed) == 6, "Fresh scene accepts an older day-two save without story metadata")
	_check(not resumed.run.fen.has_note() and resumed.run.fen.reward == "none", "Older day-two morning does not invent a promise or gift")
	resumed.free()
	DirAccess.remove_absolute(ProjectSettings.globalize_path(FEN_SAVE))
	await process_frame
