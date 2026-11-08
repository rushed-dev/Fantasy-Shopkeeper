class_name FenPanel
extends ColorRect

signal choice_requested(action: String)
signal claim_requested
signal back_requested
var story: FenStory
var title: Label
var copy: Label
var terms: Label
var promise: Button
var defer: Button
var decline: Button
var claim: Button
var feedback: Label

func _ready() -> void:
	color = Color(0.025, 0.045, 0.05, 0.94)
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(center)
	var panel := PanelContainer.new()
	panel.custom_minimum_size.x = 760
	var style := StyleBoxFlat.new()
	style.bg_color = Color("172722")
	style.set_corner_radius_all(10)
	style.content_margin_left = 28
	style.content_margin_right = 28
	style.content_margin_top = 24
	style.content_margin_bottom = 24
	panel.add_theme_stylebox_override("panel", style)
	center.add_child(panel)
	var column := VBoxContainer.new()
	column.add_theme_constant_override("separation", 14)
	panel.add_child(column)
	title = _label(column, "Fen / lamplighter", 28)
	copy = _label(column, "", 21)
	copy.custom_minimum_size.x = 704
	copy.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	terms = _label(column, "", 17)
	terms.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	promise = _button(column, "I'll try to have one for your visit tomorrow", func() -> void: choice_requested.emit("promise"))
	defer = _button(column, "Let me think — keep the note until bedtime", func() -> void: choice_requested.emit("defer"))
	decline = _button(column, "I can't promise that", func() -> void: choice_requested.emit("decline"))
	claim = _button(column, "Accept the spare tonic into the delivery tray", func() -> void: claim_requested.emit())
	feedback = _label(column, "", 17)
	feedback.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_button(column, "Back to the ledger / continue ordinary trade", func() -> void: back_requested.emit())
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
	button.custom_minimum_size.y = 42
	button.add_theme_font_size_override("font_size", 18)
	button.pressed.connect(action)
	parent.add_child(button)
	return button

func present(current: FenStory, day: int) -> void:
	story = current
	var choosing := day == 1 and story.decision in ["offered", "deferred"]
	promise.visible = choosing
	defer.visible = choosing
	decline.visible = choosing
	claim.visible = story.reward == "pending"
	feedback.text = ""
	terms.text = ""
	if choosing:
		copy.text = "The communal lantern has started attracting moths with opinions.\n\nCould you have a bottle of starlight when I come back tomorrow? I'll pay your shelf price. I'd like to try it in the lamp by your door."
		terms.text = "Optional promise: one bottled starlight on day two, visitor two. A normal paid purchase, subject to the usual price limit. Stock is not reserved automatically. You can keep trading and decide before bedtime. No fine if it doesn't work out."
	elif story.outcome == "fulfilled":
		copy.text = "There. Finally, a light that doesn't send directions to moths.\n\nThe lamp by your door should make a fine meeting point for the Lantern Walk. Thank you for remembering."
		terms.text = "Fen offers one spare dawn tonic. It is a gift, not another sale or a payment."
		if story.reward == "claimed":
			terms.text += "\nThe tonic is already in your delivery stock."
	elif story.outcome == "missed":
		copy.text = "I borrowed the bakery's lantern instead. Payment was in buns. Complicated exchange rate.\n\nNo trouble. I'll see you on the usual round."
		terms.text = "The promise wasn't fulfilled. No fine, debt or change to ordinary customer opportunities."
	elif story.decision == "promised":
		copy.text = "One bottle of starlight for Fen's second visit on day two. They'll buy it through the ordinary checkout."
		terms.text = "Keep it on a shelf where Fen can buy it. Ordering and stocking are still up to you; no bottle is locked away. No fine if it doesn't work out."
	else:
		copy.text = "The bakery has a lantern I can borrow. Unusual smell of cinnamon.\n\nCarry on with the shop. We'll find our way home."
		terms.text = "No promise was made. Ordinary trade and the Lantern Walk continue."
	show()
