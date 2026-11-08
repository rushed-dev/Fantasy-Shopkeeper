class_name DayPanel
extends ColorRect

signal story_requested
signal action_requested(action: String)
signal price_changed(product: String, amount: int)
signal close_requested
var run: ShopRun
var story_button: Button
var heading: Label
var summary: Label
var feedback: Label
var order_label: Label
var order_fields: Dictionary = {}
var price_fields: Dictionary = {}
var buttons: Dictionary = {}
var capacity := 8

func _ready() -> void:
	color = Color(0.025, 0.045, 0.05, 0.93)
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(center)
	var panel := PanelContainer.new()
	panel.custom_minimum_size.x = 800
	var style := StyleBoxFlat.new()
	style.bg_color = Color("172722")
	style.content_margin_left = 28
	style.content_margin_right = 28
	style.content_margin_top = 20
	style.content_margin_bottom = 20
	style.set_corner_radius_all(10)
	panel.add_theme_stylebox_override("panel", style)
	center.add_child(panel)
	var column := VBoxContainer.new()
	column.add_theme_constant_override("separation", 10)
	panel.add_child(column)
	heading = _label(column, "", 28)
	summary = _label(column, "", 18)
	var grid := GridContainer.new()
	grid.columns = 4
	grid.add_theme_constant_override("h_separation", 20)
	column.add_child(grid)
	for text in ["Product", "Shelf price (bits)", "Supplier cost", "Order bottles"]:
		_label(grid, text, 17)
	for product: String in ShopRun.COSTS:
		var key := product
		_label(grid, {"tonic": "Dawn tonic", "ink": "Whisper ink", "light": "Bottled starlight"}[key], 18)
		var price := SpinBox.new()
		price.min_value = 1
		price.max_value = 999
		price.step = 1
		price.custom_minimum_size.x = 140
		grid.add_child(price)
		price_fields[key] = price
		price.value_changed.connect(func(value: float) -> void: price_changed.emit(key, int(value)))
		_label(grid, "%d bits" % ShopRun.COSTS[key], 18)
		var quantity := SpinBox.new()
		quantity.min_value = 0
		quantity.max_value = 8
		quantity.step = 1
		quantity.custom_minimum_size.x = 140
		grid.add_child(quantity)
		order_fields[key] = quantity
		quantity.value_changed.connect(func(_value: float) -> void: _order_quote())
	order_label = _label(column, "", 17)
	for entry in [["open", "Open the shop"], ["buy", "Pay supplier order"], ["advance", "Sleep and receive tomorrow's delivery"], ["borrow", "Recovery loan: receive 112 bits; repay 118 total"], ["repay", "Repay debt"], ["finish", "Finish the two-day experiment"]]:
		var key: String = entry[0]
		var button := Button.new()
		button.text = entry[1]
		button.custom_minimum_size.y = 38
		button.add_theme_font_size_override("font_size", 18)
		button.pressed.connect(func() -> void: action_requested.emit(key))
		column.add_child(button)
		buttons[key] = button
	story_button = Button.new()
	story_button.custom_minimum_size.y = 38
	story_button.pressed.connect(func() -> void: story_requested.emit())
	column.add_child(story_button)
	story_button.hide()
	feedback = _label(column, "", 17)
	feedback.custom_minimum_size = Vector2(744, 50)
	feedback.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	var back := Button.new()
	back.text = "Return to the shop"
	back.custom_minimum_size.y = 38
	back.pressed.connect(func() -> void: close_requested.emit())
	column.add_child(back)
	hide()

func _label(parent: Node, text: String, size: int) -> Label:
	var label := Label.new()
	label.text = text
	label.add_theme_font_size_override("font_size", size)
	label.add_theme_color_override("font_color", Color("efdfbb"))
	parent.add_child(label)
	return label

func present(current: ShopRun, free_spaces: int, message: String = "") -> void:
	var changed := run != current
	run = current
	capacity = free_spaces
	for product in price_fields:
		price_fields[product].set_value_no_signal(run.prices[product])
		if run.order_paid:
			order_fields[product].set_value_no_signal(run.pending_order[product])
		elif changed:
			order_fields[product].set_value_no_signal(2 if capacity >= 6 else 0)
	feedback.text = message
	show()
	refresh()

func quantities() -> Dictionary:
	var result := {}
	for product in order_fields:
		result[product] = int(order_fields[product].value)
	return result

func refresh() -> void:
	heading.text = "Day %d / %s" % [run.day, run.phase.capitalize()]
	summary.text = "Cash: %d bits   •   Debt: %d bits\nToday's takings: %d   •   Sales: %d   •   Shop expense: %d/day\nSupplier payments: %d   •   Debt repaid today: %d" % [run.cash, run.debt, run.revenue, run.sales, ShopRun.DAILY_COST, run.stock_spend, run.repayments]
	if run.phase == "preparation":
		summary.text += "\nPrepare as long as you like. Three customer opportunities follow opening."
	elif run.phase == "evening":
		summary.text += "\nShop expense charged once. Unsold stock stays; paid orders arrive tomorrow."
	elif run.phase == "complete":
		summary.text += "\nTwo days complete. You can still explore or restart from the pause menu."
	if run.phase == "evening" and run.day == 1 and not run.loan_taken and run.cash < 68:
		summary.text += "\nOne recovery loan: +112 cash, 118 debt. Repayment is voluntary.\nNo compounding or late fee; no second loan in this experiment."
	for product in price_fields:
		price_fields[product].editable = run.phase in ["preparation", "evening"]
		order_fields[product].editable = run.phase == "evening" and run.day == 1 and not run.order_paid
	buttons.open.visible = run.phase == "preparation"
	buttons.buy.visible = run.phase == "evening" and run.day == 1
	buttons.buy.disabled = run.order_paid
	buttons.advance.visible = run.phase == "evening" and run.day == 1
	buttons.advance.disabled = not run.order_paid
	buttons.borrow.visible = run.phase == "evening" and run.day == 1 and not run.loan_taken and run.cash < 68
	buttons.repay.visible = run.phase == "evening" and run.debt > 0 and run.cash > 0
	buttons.repay.text = "Repay %d bits now (voluntary; no compounding or late fee)" % mini(run.debt, maxi(run.cash, 0))
	buttons.finish.visible = run.phase == "evening" and run.day == 2
	_order_quote()

func _order_quote() -> void:
	if run == null or order_label == null:
		return
	order_label.text = "Planned order: %d bits • %d delivery spaces free. Prices lock while trading." % [run.order_cost(quantities()), capacity]
	if run.day == 2:
		order_label.text = "This experiment ends after day two; no third delivery is ordered."
	elif run.phase == "preparation":
		order_label.text = "Order in the evening. Only stock on shelves is available to customers."
	elif run.order_paid:
		order_label.text = "Order paid. It will be delivered once tomorrow morning."
