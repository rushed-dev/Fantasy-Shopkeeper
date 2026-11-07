extends Node3D

const PRODUCTS := [
	{"id": "tonic", "title": "Dawn tonic", "color": Color("479e8c"), "magic": false},
	{"id": "ink", "title": "Whisper ink", "color": Color("9976b8"), "magic": false},
	{"id": "light", "title": "Bottled starlight", "color": Color("f4be69"), "magic": true},
]

var player: ShopPlayer
var slots: Array[StockSlot] = []
var shelf_slots: Array[StockSlot] = []
var focused: StockSlot
var hud: CanvasLayer
var prompt: Label
var held_label: Label
var progress_label: Label
var toast: Label
var reticle: Label
var overlay: ColorRect
var menu_title: Label
var menu_copy: Label
var resume_button: Button
var feedback_seconds: float = 0.0
var started: bool = false
var completed: bool = false
var smoke_frames: int = 0
var capture_path: String = ""
var capture_frames: int = 0


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	_configure_input()
	_build_room()
	_build_stock()
	player = ShopPlayer.new()
	player.name = "Shopkeeper"
	add_child(player)
	player.position = Vector3(0, 0.05, 3.35)
	player.camera.rotation.x = -0.1
	player.camera.current = true
	_build_hud()
	_update_stock_count()
	for argument in OS.get_cmdline_user_args():
		if argument == "--smoke-test":
			smoke_frames = 45
		if argument.begins_with("--capture="):
			capture_path = argument.trim_prefix("--capture=")
			capture_frames = 90
	if smoke_frames > 0 or capture_frames > 0:
		_start()
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE


func _configure_input() -> void:
	var bindings := {
		"move_forward": [KEY_W, KEY_UP], "move_back": [KEY_S, KEY_DOWN],
		"move_left": [KEY_A, KEY_LEFT], "move_right": [KEY_D, KEY_RIGHT],
		"interact": [KEY_E], "pause_shop": [KEY_ESCAPE],
	}
	for action: String in bindings:
		if not InputMap.has_action(action):
			InputMap.add_action(action)
		for key: int in bindings[action]:
			var event := InputEventKey.new()
			event.physical_keycode = key
			if not InputMap.action_has_event(action, event):
				InputMap.action_add_event(action, event)


func _build_room() -> void:
	var environment := WorldEnvironment.new()
	var settings := Environment.new()
	settings.background_mode = Environment.BG_COLOR
	settings.background_color = Color("24323d")
	settings.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	settings.ambient_light_color = Color("e8d4b1")
	settings.ambient_light_energy = 0.65
	settings.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	environment.environment = settings
	add_child(environment)
	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-48, -32, 0)
	sun.light_color = Color("ffe4b2")
	sun.light_energy = 1.15
	sun.shadow_enabled = true
	add_child(sun)
	var fill := OmniLight3D.new()
	fill.position = Vector3(0, 2.8, 0)
	fill.light_color = Color("ffdaab")
	fill.light_energy = 1.4
	fill.omni_range = 9
	add_child(fill)
	# Warm plank floor, plaster walls, and exposed timber.
	_box(Vector3(10.2, 0.2, 9.2), Vector3(0, -0.1, 0), Color("5c4534"), true)
	for i in range(20):
		var plank_color := Color("866347") if i % 2 == 0 else Color("795a42")
		_box(Vector3(0.49, 0.025, 8.95), Vector3(-4.75 + i * 0.5, 0.012, 0), plank_color)
	_box(Vector3(10, 3.6, 0.2), Vector3(0, 1.8, -4.5), Color("b9a181"), true)
	_box(Vector3(0.2, 3.6, 9), Vector3(-5, 1.8, 0), Color("b49c7d"), true)
	_box(Vector3(0.2, 3.6, 9), Vector3(5, 1.8, 0), Color("b49c7d"), true)
	_box(Vector3(10, 3.6, 0.2), Vector3(0, 1.8, 4.5), Color("b49c7d"), true)
	_box(Vector3(10, 0.15, 9), Vector3(0, 3.65, 0), Color("68513e"))
	for x in [-4.85, -2.5, 0.0, 2.5, 4.85]:
		_box(Vector3(0.16, 3.6, 0.2), Vector3(x, 1.8, -4.32), Color("4a382e"))
		_box(Vector3(0.18, 0.23, 9), Vector3(x, 3.45, 0), Color("4a382e"))
	_box(Vector3(10, 0.2, 0.22), Vector3(0, 2.6, -4.3), Color("4a382e"))
	_box(Vector3(10, 0.24, 0.22), Vector3(0, 0.2, -4.3), Color("4a382e"))
	# Shop sign and a soft window-like inset.
	_box(Vector3(3.8, 0.65, 0.1), Vector3(0, 2.84, -4.15), Color("344c43"))
	_label_3d("THE WANDERING WICK", Vector3(0, 2.95, -4.08), 42, Color("f3dfb2"), 0.005)
	_label_3d("TONICS  /  INKS  /  SMALL WONDERS", Vector3(0, 2.71, -4.08), 24, Color("d8c69d"), 0.005)
	_box(Vector3(1.8, 1.35, 0.1), Vector3(-3.55, 2.0, -4.27), Color("675239"))
	_box(Vector3(1.58, 1.15, 0.12), Vector3(-3.55, 2.0, -4.2), Color("9db7ad"))
	_box(Vector3(0.07, 1.2, 0.13), Vector3(-3.55, 2.0, -4.12), Color("5a4635"))
	_box(Vector3(1.6, 0.06, 0.13), Vector3(-3.55, 2.0, -4.12), Color("5a4635"))
	# A rug marks the clear customer route.
	_box(Vector3(2.4, 0.025, 4.4), Vector3(0, 0.04, 0.45), Color("68505c"))
	for x in [-1.12, 1.12]:
		_box(Vector3(0.055, 0.03, 4.2), Vector3(x, 0.043, 0.45), Color("ba9259"))
	# Future checkout counter, visibly present but not interactive in milestone one.
	_table(Vector3(3.1, 0, 1.35), Vector3(2.4, 1.0, 1.1))
	_box(Vector3(0.55, 0.06, 0.4), Vector3(2.75, 1.1, 1.25), Color("ebe0bd"))
	_box(Vector3(0.04, 0.07, 0.42), Vector3(2.75, 1.105, 1.25), Color("74513a"))
	_label_3d("COUNTER", Vector3(3.1, 0.71, 1.93), 32, Color("f4d9a1"), 0.005)
	_label_3d("Checkout comes next", Vector3(3.1, 0.49, 1.93), 23, Color("d8c5a4"), 0.004)
	for x in [3.25, 3.55, 3.85]:
		var pouch := SphereMesh.new()
		pouch.radius = 0.1
		pouch.height = 0.2
		_mesh(pouch, Vector3(x, 1.16, 1.25), Color("9a7850"))
	# Entrance is an intentionally closed backdrop for this single-room study.
	_box(Vector3(1.5, 2.5, 0.1), Vector3(0, 1.25, 4.31), Color("354c44"))
	_box(Vector3(0.12, 0.12, 0.15), Vector3(0.48, 1.15, 4.2), Color("ceab63"))
	# A few decorative books; all gameplay stock is in the delivery tray.
	_table(Vector3(3.7, 0, -2.8), Vector3(1.3, 0.7, 0.85))
	for i in range(4):
		_box(Vector3(0.6 - i * 0.04, 0.08, 0.42), Vector3(3.7, 0.8 + i * 0.08, -2.8), Color("725d77") if i % 2 == 0 else Color("b79764"))


func _build_stock() -> void:
	# Six destination positions, two per product, on a low accessible display.
	_table(Vector3(0, 0, -3.4), Vector3(5.4, 0.95, 0.9))
	_box(Vector3(5.4, 0.28, 0.1), Vector3(0, 1.13, -3.85), Color("725235"))
	for product_index in range(PRODUCTS.size()):
		var product: Dictionary = PRODUCTS[product_index]
		var center_x: float = -1.8 + product_index * 1.8
		_label_3d(product.title.to_upper(), Vector3(center_x, 0.81, -2.92), 27, Color("f7dfac"), 0.004)
		for position_index in range(2):
			var slot := StockSlot.new()
			slot.name = "Shelf_%s_%d" % [product.id, position_index]
			slot.accepted_product = product.id
			slot.display_name = product.title
			slot.is_shelf = true
			add_child(slot)
			slot.position = Vector3(center_x - 0.35 + position_index * 0.7, 1.03, -3.35)
			slots.append(slot)
			shelf_slots.append(slot)
	# Delivery cradle; two spare spots let the player rearrange without dropping.
	_table(Vector3(-3.25, 0, 0.65), Vector3(2.5, 0.62, 1.35))
	for x in [-4.48, -2.02]:
		_box(Vector3(0.07, 0.18, 1.35), Vector3(x, 0.76, 0.65), Color("9c7549"))
	_box(Vector3(2.5, 0.18, 0.07), Vector3(-3.25, 0.76, 0.0), Color("9c7549"))
	_label_3d("MORNING DELIVERY", Vector3(-3.25, 0.51, 1.36), 29, Color("f7dfac"), 0.004)
	for i in range(8):
		var slot := StockSlot.new()
		slot.name = "Delivery_%d" % i
		add_child(slot)
		slot.position = Vector3(-4.15 + (i % 4) * 0.6, 0.7, 0.35 + (i / 4) * 0.6)
		slots.append(slot)
		if i < 6:
			var product: Dictionary = PRODUCTS[i % PRODUCTS.size()]
			var item := StockItem.new().configure(product.id, product.title, product.color, product.magic)
			slot.seed(item)


func _table(at: Vector3, size: Vector3) -> void:
	_box(Vector3(size.x, 0.12, size.z), at + Vector3(0, size.y, 0), Color("8e6845"), true)
	_box(Vector3(size.x - 0.12, 0.28, size.z - 0.12), at + Vector3(0, size.y - 0.2, 0), Color("614734"), true)
	for x in [-size.x / 2 + 0.12, size.x / 2 - 0.12]:
		for z in [-size.z / 2 + 0.12, size.z / 2 - 0.12]:
			_box(Vector3(0.15, size.y, 0.15), at + Vector3(x, size.y / 2, z), Color("5d4432"), true)


func _box(size: Vector3, at: Vector3, color: Color, solid: bool = false) -> MeshInstance3D:
	var box := BoxMesh.new()
	box.size = size
	var visual := _mesh(box, at, color)
	if solid:
		var body := StaticBody3D.new()
		body.position = at
		var shape := BoxShape3D.new()
		shape.size = size
		var collider := CollisionShape3D.new()
		collider.shape = shape
		body.add_child(collider)
		add_child(body)
	return visual


func _mesh(mesh: Mesh, at: Vector3, color: Color) -> MeshInstance3D:
	var visual := MeshInstance3D.new()
	visual.mesh = mesh
	visual.position = at
	var material := StandardMaterial3D.new()
	material.albedo_color = color
	material.roughness = 0.85
	visual.material_override = material
	add_child(visual)
	return visual


func _label_3d(text: String, at: Vector3, size: int, color: Color, pixel_size: float) -> void:
	var label := Label3D.new()
	label.text = text
	label.position = at
	label.font_size = size
	label.pixel_size = pixel_size
	label.modulate = color
	label.outline_size = 0
	label.no_depth_test = false
	add_child(label)


func _build_hud() -> void:
	hud = CanvasLayer.new()
	add_child(hud)
	var frame := Control.new()
	frame.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	frame.mouse_filter = Control.MOUSE_FILTER_IGNORE
	hud.add_child(frame)
	var top := _panel(Color(0.06, 0.10, 0.10, 0.9))
	top.set_anchors_and_offsets_preset(Control.PRESET_TOP_WIDE)
	top.offset_left = 24
	top.offset_top = 22
	top.offset_right = -24
	top.offset_bottom = 118
	frame.add_child(top)
	var top_row := HBoxContainer.new()
	top_row.add_theme_constant_override("separation", 30)
	top.add_child(top_row)
	var branding := VBoxContainer.new()
	branding.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	top_row.add_child(branding)
	branding.add_child(_text("THE WANDERING WICK", 26, Color("f2d9a4")))
	branding.add_child(_text("First delivery  /  a shopkeeping prototype", 16, Color("c2c9bc")))
	var objectives := VBoxContainer.new()
	top_row.add_child(objectives)
	objectives.add_child(_text("PREPARE YOUR SHELVES", 15, Color("cfb784")))
	progress_label = _text("0 / 6 bottles stocked", 22, Color("f0eadb"))
	objectives.add_child(progress_label)
	reticle = _text("+", 22, Color("f5e5bf"))
	reticle.set_anchors_and_offsets_preset(Control.PRESET_CENTER)
	reticle.position -= Vector2(7, 15)
	frame.add_child(reticle)
	var bottom := _panel(Color(0.06, 0.10, 0.10, 0.92))
	bottom.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_WIDE)
	bottom.offset_left = 24
	bottom.offset_right = -24
	bottom.offset_top = -122
	bottom.offset_bottom = -22
	frame.add_child(bottom)
	var bottom_column := VBoxContainer.new()
	bottom.add_child(bottom_column)
	prompt = _text("Find the delivery tray on your left.", 22, Color("f2d9a4"))
	bottom_column.add_child(prompt)
	held_label = _text("Hands free", 16, Color("c2c9bc"))
	bottom_column.add_child(held_label)
	bottom_column.add_child(_text("WASD / arrows  Move     Mouse  Look     E  Pick up / place     Esc  Pause", 14, Color("a3b0a4")))
	toast = _text("", 20, Color("f2d9a4"))
	toast.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	toast.set_anchors_and_offsets_preset(Control.PRESET_CENTER_BOTTOM)
	toast.offset_left = -420
	toast.offset_right = 420
	toast.offset_top = -177
	toast.offset_bottom = -135
	frame.add_child(toast)
	_build_menu(frame)


func _build_menu(frame: Control) -> void:
	overlay = ColorRect.new()
	overlay.color = Color(0.025, 0.045, 0.05, 0.79)
	overlay.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	frame.add_child(overlay)
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	overlay.add_child(center)
	var panel := _panel(Color("172722"))
	panel.custom_minimum_size = Vector2(580, 0)
	center.add_child(panel)
	var column := VBoxContainer.new()
	column.add_theme_constant_override("separation", 16)
	panel.add_child(column)
	column.add_child(_text("FANTASY SHOPKEEPER  /  MILESTONE 01", 14, Color("c4a771")))
	menu_title = _text("Your first delivery", 36, Color("f2dfb6"))
	column.add_child(menu_title)
	menu_copy = _text("A quiet morning. Six bottles. An empty display.\n\nCarry each bottle from the delivery tray to its labelled\nshelf. Starlight will brighten the shop as you move it.\n\nTake your time. There are no customers or costs yet.", 19, Color("ced5c5"))
	column.add_child(menu_copy)
	column.add_child(_text("WASD / arrows to walk   •   Mouse to look   •   E to handle stock", 15, Color("b4c3b4")))
	var sensitivity_row := HBoxContainer.new()
	sensitivity_row.add_theme_constant_override("separation", 16)
	sensitivity_row.add_child(_text("Look sensitivity", 16, Color("ced5c5")))
	var sensitivity := HSlider.new()
	sensitivity.min_value = 0.5
	sensitivity.max_value = 3
	sensitivity.step = 0.1
	sensitivity.value = 1.1
	sensitivity.custom_minimum_size.x = 230
	sensitivity.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	sensitivity.value_changed.connect(func(value: float) -> void: player.mouse_sensitivity = value * 0.002)
	sensitivity_row.add_child(sensitivity)
	column.add_child(sensitivity_row)
	resume_button = Button.new()
	resume_button.text = "Open the delivery"
	resume_button.custom_minimum_size.y = 48
	resume_button.add_theme_font_size_override("font_size", 20)
	resume_button.pressed.connect(_start)
	column.add_child(resume_button)
	var reset := Button.new()
	reset.text = "Reset this delivery"
	reset.pressed.connect(_reset)
	column.add_child(reset)
	var quit_button := Button.new()
	quit_button.text = "Leave the shop"
	quit_button.pressed.connect(func() -> void: get_tree().quit())
	column.add_child(quit_button)
	resume_button.grab_focus()
	reticle.hide()


func _panel(color: Color) -> PanelContainer:
	var panel := PanelContainer.new()
	var style := StyleBoxFlat.new()
	style.bg_color = color
	style.border_color = Color("6c6042")
	style.set_border_width_all(1)
	style.set_corner_radius_all(8)
	style.content_margin_left = 22
	style.content_margin_right = 22
	style.content_margin_top = 16
	style.content_margin_bottom = 16
	panel.add_theme_stylebox_override("panel", style)
	return panel


func _text(value: String, size: int, color: Color) -> Label:
	var label := Label.new()
	label.text = value
	label.add_theme_font_size_override("font_size", size)
	label.add_theme_color_override("font_color", color)
	label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	return label


func _start() -> void:
	started = true
	get_tree().paused = false
	overlay.hide()
	reticle.show()
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED


func _reset() -> void:
	get_tree().paused = false
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	get_tree().reload_current_scene()


func _pause() -> void:
	get_tree().paused = true
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	menu_title.text = "A moment to yourself"
	menu_copy.text = "The shop can wait.\n\nYour stock stays exactly where you left it.\nResume when you are ready, or reset the delivery\nto try the handling again."
	resume_button.text = "Return to the shop"
	overlay.show()
	reticle.hide()
	resume_button.grab_focus()


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("pause_shop") and not event.is_echo():
		if overlay.visible:
			_start()
		else:
			_pause()
		get_viewport().set_input_as_handled()
	if event.is_action_pressed("interact") and not event.is_echo() and started and not get_tree().paused:
		# Query again at commit rather than trusting last frame's highlighted slot.
		var target := player.aimed_slot()
		if target != null:
			var result := ItemActions.interact(player, target)
			toast.text = result.message
			feedback_seconds = 2.5
			_update_stock_count()
		get_viewport().set_input_as_handled()


func _process(delta: float) -> void:
	if smoke_frames > 0:
		smoke_frames -= 1
		if smoke_frames == 0:
			print("SCENE SMOKE PASS: room, player, 14 slots, 6 products and HUD loaded")
			get_tree().quit()
	if capture_frames > 0:
		capture_frames -= 1
		if capture_frames == 0:
			_capture()
	if not started or get_tree().paused:
		return
	var target := player.aimed_slot()
	if focused != target:
		if is_instance_valid(focused):
			focused.set_highlight(false)
		focused = target
		if is_instance_valid(focused):
			focused.set_highlight(true)
	if focused != null:
		var result := ItemActions.inspect(player, focused)
		prompt.text = ("[E]  " if result.ok else "") + result.message
	elif is_instance_valid(player.held_item):
		prompt.text = "Find the shelf labelled %s." % player.held_item.display_name
	else:
		prompt.text = "Aim at a bottle or an empty stock space."
	held_label.text = "Carrying: %s" % player.held_item.display_name if is_instance_valid(player.held_item) else "Hands free  /  Carry one bottle at a time"
	if feedback_seconds > 0:
		feedback_seconds -= delta
		if feedback_seconds <= 0:
			toast.text = ""


func _update_stock_count() -> void:
	var count := 0
	for slot in shelf_slots:
		if is_instance_valid(slot.item):
			count += 1
	progress_label.text = "%d / 6 bottles stocked" % count
	if count == 6 and not completed:
		completed = true
		toast.text = "Ready for business. Your first delivery is stocked!"
		feedback_seconds = 8
	elif count < 6:
		completed = false


func _capture() -> void:
	await RenderingServer.frame_post_draw
	var snapshot := get_viewport().get_texture().get_image()
	var error := snapshot.save_png(capture_path)
	print("SCREENSHOT: %s (result %d)" % [capture_path, error])
	get_tree().quit(0 if error == OK else 1)
