class_name CheckoutPanel
extends ColorRect

signal close_requested
signal invite_requested
signal confirm_requested

var sale: CounterSale
var heading: Label
var basket_text: Label
var payment_text: Label
var feedback: Label
var total_entry: LineEdit
var agree: Button
var invite: Button
var confirm: Button
var pouches: Array[Button] = []
var tray_coins: Array[Button] = []
var held_value := 0
var repeat_in := 0.0
var coin_sound: AudioStreamPlayer

func _ready() -> void:
	color = Color(0.025, 0.045, 0.05, 0.92)
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(center)
	var panel := PanelContainer.new()
	panel.custom_minimum_size = Vector2(740, 0)
	var style := StyleBoxFlat.new()
	style.bg_color = Color("172722")
	style.set_corner_radius_all(10)
	style.content_margin_left = 28
	style.content_margin_right = 28
	style.content_margin_top = 20
	style.content_margin_bottom = 20
	panel.add_theme_stylebox_override("panel", style)
	center.add_child(panel)
	var column := VBoxContainer.new()
	column.add_theme_constant_override("separation", 12)
	panel.add_child(column)
	heading = _label(column, "The sales ledger", 28)
	basket_text = _label(column, "", 20)
	payment_text = _label(column, "", 20)
	var row := HBoxContainer.new()
	column.add_child(row)
	total_entry = LineEdit.new()
	total_entry.placeholder_text = "Enter basket total in bits"
	total_entry.custom_minimum_size = Vector2(360, 44)
	total_entry.max_length = 8
	row.add_child(total_entry)
	agree = _button(row, "Agree total", _agree_total)
	total_entry.text_submitted.connect(func(_text: String) -> void: _agree_total())
	_label(column, "Coin pouches — click or hold • keys 1 / 2 / 3", 17)
	var pouch_row := HBoxContainer.new()
	column.add_child(pouch_row)
	var tray_row := HBoxContainer.new()
	_label(column, "Change tray — take one back by clicking its stack (or Shift + 1 / 2 / 3)", 16)
	column.add_child(tray_row)
	for value: int in CounterSale.DENOMINATIONS:
		var coin := value
		var pouch := _button(pouch_row, "%s (%d)" % [_coin_name(coin).capitalize(), coin], func() -> void: pass)
		pouch.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		pouch.button_down.connect(func() -> void: _begin_hold(coin))
		pouch.button_up.connect(_stop_hold)
		pouches.append(pouch)
		var stack := _button(tray_row, "", func() -> void: _remove(coin))
		stack.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		tray_coins.append(stack)
	confirm = _button(column, "Confirm sale", func() -> void: confirm_requested.emit())
	invite = _button(column, "Invite a customer", func() -> void: invite_requested.emit())
	feedback = _label(column, "", 17)
	feedback.custom_minimum_size = Vector2(680, 52)
	feedback.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_button(column, "Return to the shop", func() -> void: close_requested.emit())
	coin_sound = AudioStreamPlayer.new()
	coin_sound.volume_db = -22
	var sound := AudioStreamWAV.new()
	sound.format = AudioStreamWAV.FORMAT_16_BITS
	sound.mix_rate = 22050
	var bytes := PackedByteArray()
	bytes.resize(2206)
	for i in range(1103):
		var sample := int(sin(i * TAU * 1100.0 / 22050.0) * 16000 * exp(-i / 180.0))
		bytes.encode_s16(i * 2, sample)
	sound.data = bytes
	coin_sound.stream = sound
	add_child(coin_sound)
	visibility_changed.connect(func() -> void: _stop_hold())
	hide()

func _label(parent: Node, text: String, size: int) -> Label:
	var label := Label.new()
	label.text = text
	label.add_theme_font_size_override("font_size", size)
	label.add_theme_color_override("font_color", Color("efdfbb"))
	parent.add_child(label)
	return label

func _button(parent: Node, text: String, action: Callable) -> Button:
	var button := Button.new()
	button.text = text
	button.custom_minimum_size.y = 40
	button.add_theme_font_size_override("font_size", 18)
	button.pressed.connect(action)
	parent.add_child(button)
	return button

func present(current: CounterSale, description: String, can_invite: bool) -> void:
	if sale != current:
		total_entry.text = ""
		feedback.text = "Take your time. You can leave the counter and return."
	sale = current
	invite.visible = can_invite
	basket_text.text = description
	if sale != null:
		basket_text.text = "Basket / prices in bits\n"
		for i in range(sale.items.size()):
			basket_text.text += "%s   —   %d\n" % [sale.items[i].display_name, sale.prices[i]]
	show()
	refresh()
	if sale != null and not sale.total_accepted:
		total_entry.grab_focus()

func refresh() -> void:
	var active := sale != null and not sale.completed
	total_entry.visible = active
	agree.visible = active
	total_entry.editable = active and not sale.total_accepted
	agree.disabled = not active or sale.total_accepted
	payment_text.text = "Customer payment: %d bits" % sale.payment if active and sale.total_accepted else "Agree the basket total to receive payment."
	if not active:
		payment_text.text = ""
	confirm.visible = active
	confirm.disabled = not active or not sale.total_accepted
	for i in range(pouches.size()):
		pouches[i].disabled = not active or not sale.total_accepted
		var value: int = CounterSale.DENOMINATIONS[i]
		var count: int = sale.coins[value] if sale != null else 0
		tray_coins[i].text = "%d × %s (%d)" % [count, _coin_name(value), value]
		tray_coins[i].disabled = not active or count == 0

func _agree_total() -> void:
	if sale == null:
		return
	var result := sale.submit_total(total_entry.text)
	feedback.text = result.message
	refresh()
	if result.ok:
		confirm.grab_focus()

func _begin_hold(value: int) -> void:
	held_value = value
	repeat_in = 0.4
	_add(value)

func _stop_hold() -> void:
	held_value = 0

func _add(value: int) -> void:
	if sale != null and sale.add_coin(value):
		if DisplayServer.get_name() != "headless":
			coin_sound.play()
		feedback.text = "Adjust the tray, then confirm when you are ready."
		refresh()

func _remove(value: int) -> void:
	if sale != null and sale.remove_coin(value):
		if DisplayServer.get_name() != "headless":
			coin_sound.play()
		feedback.text = "Adjust the tray, then confirm when you are ready."
		refresh()

func _process(delta: float) -> void:
	if not visible or held_value == 0:
		return
	repeat_in -= delta
	if repeat_in <= 0:
		_add(held_value)
		repeat_in = 0.14

func _input(event: InputEvent) -> void:
	if not visible:
		return
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT and not event.pressed:
		_stop_hold()
	if not event is InputEventKey or sale == null or not sale.total_accepted or sale.completed:
		return
	var index := [KEY_1, KEY_2, KEY_3].find(event.physical_keycode)
	if index < 0:
		return
	var value: int = CounterSale.DENOMINATIONS[index]
	if event.pressed and not event.echo:
		if event.shift_pressed:
			_remove(value)
		else:
			_begin_hold(value)
	elif not event.pressed and held_value == value:
		_stop_hold()
	get_viewport().set_input_as_handled()

func _coin_name(value: int) -> String:
	return {1: "bit", 10: "mark", 100: "crown"}[value]


func _exit_tree() -> void:
	coin_sound.stop()


func _notification(what: int) -> void:
	if what == NOTIFICATION_APPLICATION_FOCUS_OUT:
		_stop_hold()
