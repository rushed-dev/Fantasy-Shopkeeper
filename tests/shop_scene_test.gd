extends SceneTree

var assertions := 0
var failures := 0
var walk_route := false

func _initialize() -> void:
	_run.call_deferred()

func _check(ok: bool, message: String) -> void:
	assertions += 1
	if not ok:
		failures += 1
		push_error(message)

func _run() -> void:
	walk_route = "--walk-route" in OS.get_cmdline_user_args()
	if walk_route and DisplayServer.get_name() == "headless":
		push_error("--walk-route requires a graphical display")
		quit(1)
		return
	# Construct outside the tree, as the shop does during _ready(). Local
	# hierarchy setup must not depend on a global transform being available.
	var detached := ShopPlayer.new()
	_check(not detached.is_inside_tree(), "Player can be constructed outside scene tree")
	_check(detached.carry_anchor.get_parent() == detached.camera, "Detached carry anchor belongs to camera")
	_check(detached.carry_anchor.position.is_equal_approx(Vector3(0.38, -0.38, -0.72)), "Detached carry anchor has local offset")
	detached.free()
	var shop = load("res://scenes/shop.tscn").instantiate()
	shop.persistence_enabled = false
	root.add_child(shop)
	_check(shop.overlay.visible and not shop.started, "Opening menu explains delivery before play")
	shop.resume_button.pressed.emit()
	var player: ShopPlayer = shop.player
	_check(is_instance_valid(player.carry_anchor), "Player creates carry anchor")
	_check(player.carry_anchor.get_parent() == player.camera, "Carry anchor follows camera")
	_check(player.carry_anchor.position.is_equal_approx(Vector3(0.38, -0.38, -0.72)), "Player initialisation completes")
	await physics_frame
	var start := player.position
	Input.action_press("move_forward")
	for i in range(20):
		await physics_frame
	Input.action_release("move_forward")
	if DisplayServer.get_name() == "headless":
		print("SKIP: walking requires a display with mouse capture")
	else:
		_check(player.position.z < start.z - 0.5, "Movement input moves player through shop")
	# Aim and press the same interaction action used by the player. Relocate between
	# stations to isolate stock geometry/reach from route and manual mouse control.
	for source: StockSlot in shop.slots:
		if source.is_shelf or not is_instance_valid(source.item):
			continue
		var item := source.item
		var destination: StockSlot
		for shelf: StockSlot in shop.shelf_slots:
			if shelf.accepts(item) and not is_instance_valid(shelf.item):
				destination = shelf
				break
		await _interact(shop, source, Vector3(source.position.x, 0.05, -0.7 if source.position.z < 0.6 else 1.85))
		_check(player.held_item == item and source.item == null, "Picked up delivery %s" % source.name)
		await _interact(shop, destination, Vector3(destination.position.x, 0.05, -1.9))
		_check(destination.item == item and player.held_item == null, "Stocked shelf %s" % destination.name)
	_check(shop.completed and "Day 1" in shop.progress_label.text, "Six bottles complete delivery and update HUD")
	if walk_route:
		player.rotation = Vector3.ZERO
		Input.action_press("move_forward")
		for frame in range(60):
			await physics_frame
		Input.action_release("move_forward")
		_check(player.position.z > -2.9, "Shelf collision prevents walking through display")
	shop._pause()
	var paused_at := player.position
	Input.action_press("move_back")
	for frame in range(5):
		await physics_frame
	Input.action_release("move_back")
	_check(player.position.is_equal_approx(paused_at), "Pause prevents movement")
	_check(shop.completed, "Pause preserves stocked items")
	_check(paused and shop.overlay.visible, "Pause opens menu")
	shop._start()
	_check(not paused and not shop.overlay.visible, "Resume returns to shop")
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--capture-final="):
			player.position = Vector3(0, 0.05, -1.5)
			player.rotation = Vector3.ZERO
			player.camera.rotation = Vector3(-0.15, 0, 0)
			Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
			await process_frame
			await RenderingServer.frame_post_draw
			_check(root.get_texture().get_image().save_png(argument.trim_prefix("--capture-final=")) == OK, "Save completed shop screenshot")
	print("SHOP SCENE: %d assertions, %d failures" % [assertions, failures])
	quit(1 if failures else 0)

func _interact(shop: Node, slot: StockSlot, at: Vector3) -> void:
	var player: ShopPlayer = shop.player
	if walk_route:
		# Cross the clear central aisle before approaching either side of the tray.
		await _walk_to(player, Vector3(0, 0, player.position.z))
		await _walk_to(player, Vector3(0, 0, at.z))
		await _walk_to(player, at)
	else:
		player.position = at
	player.velocity = Vector3.ZERO
	player.rotation = Vector3.ZERO
	player.camera.look_at(slot.global_position + Vector3(0, 0.18, 0))
	await physics_frame
	await physics_frame
	_check(player.aimed_slot() == slot, "Camera can target %s" % slot.name)
	var event := InputEventAction.new()
	event.action = "interact"
	event.pressed = true
	shop._unhandled_input(event)


func _walk_to(player: ShopPlayer, target: Vector3) -> void:
	player.rotation = Vector3.ZERO
	for frame in range(300):
		var offset := target - player.position
		offset.y = 0
		if offset.length() < 0.09:
			break
		var direction := offset.normalized()
		Input.action_press("move_right", maxf(direction.x, 0))
		Input.action_press("move_left", maxf(-direction.x, 0))
		Input.action_press("move_back", maxf(direction.z, 0))
		Input.action_press("move_forward", maxf(-direction.z, 0))
		await physics_frame
	for action in ["move_right", "move_left", "move_back", "move_forward"]:
		Input.action_release(action)
	var remaining := target - player.position
	remaining.y = 0
	_check(remaining.length() < 0.12, "Walk to %s without bypassing collision" % target)
