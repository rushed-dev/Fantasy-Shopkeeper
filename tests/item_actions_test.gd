extends SceneTree

var failures: int = 0
var assertions: int = 0
var arena: Node3D


func _initialize() -> void:
	_run.call_deferred()


func _check(condition: bool, message: String) -> void:
	assertions += 1
	if not condition:
		failures += 1
		push_error("FAIL: " + message)


func _actor(at: Vector3 = Vector3.ZERO) -> ShopActor:
	var actor := ShopActor.new()
	arena.add_child(actor)
	actor.position = at
	return actor


func _slot(at: Vector3, product: String = "") -> StockSlot:
	var slot := StockSlot.new()
	arena.add_child(slot)
	slot.position = at
	slot.accepted_product = product
	slot.display_name = product
	return slot


func _item(product: String = "tonic") -> StockItem:
	return StockItem.new().configure(product, product, Color.TEAL)


func _run() -> void:
	arena = Node3D.new()
	root.add_child(arena)
	var player := _actor()
	var worker := _actor(Vector3(0.1, 0, 0))
	var delivery := _slot(Vector3(0, 0.9, -1))
	var shelf := _slot(Vector3(0.8, 0.9, -1), "tonic")
	var wrong_shelf := _slot(Vector3(-0.8, 0.9, -1), "ink")
	var distant := _slot(Vector3(0, 0.9, -8))
	var original := _item()
	delivery.seed(original)
	await physics_frame
	await physics_frame

	_check(ItemActions.interact(player, delivery).ok, "Player picks up reachable delivery stock")
	_check(delivery.item == null and player.held_item == original, "Pickup transfers unique ownership")
	_check(original.get_parent() == player.carry_anchor, "Physical item follows its carrier")
	_check(not ItemActions.interact(worker, delivery).ok, "Second actor cannot duplicate already taken stock")
	_check(worker.held_item == null, "Losing actor remains empty-handed")
	_check(not ItemActions.interact(player, wrong_shelf).ok, "Incompatible shelf rejects stock")
	_check(player.held_item == original and wrong_shelf.item == null, "Rejected transfer preserves stock")
	_check(not ItemActions.interact(player, distant).ok, "Cannot place stock out of reach")
	_check(ItemActions.interact(player, shelf).ok, "Player stocks compatible shelf")
	_check(player.held_item == null and shelf.item == original, "Stocking clears the carry slot")
	_check(original.get_parent() == shelf, "Stocked physical item belongs to its shelf")

	_check(ItemActions.interact(worker, shelf).ok, "Worker picks up through the exact same action")
	_check(not ItemActions.interact(worker, wrong_shelf).ok, "Worker obeys product compatibility")
	_check(not ItemActions.interact(worker, distant).ok, "Worker obeys reach limit")
	_check(ItemActions.interact(worker, delivery).ok, "Worker can return product to delivery")
	_check(worker.held_item == null and delivery.item == original, "Worker transfer conserves stock")

	# Competing destinations: a valid preview is not a reservation or permission to overwrite.
	var second_item := _item()
	shelf.seed(second_item)
	_check(ItemActions.interact(player, delivery).ok, "Player carries first item")
	_check(ItemActions.interact(worker, shelf).ok, "Worker carries second item")
	_check(ItemActions.inspect(player, shelf).ok and ItemActions.inspect(worker, shelf).ok, "Both actors can preview an empty compatible space")
	_check(ItemActions.interact(worker, shelf).ok, "First actor commits to destination")
	_check(not ItemActions.interact(player, shelf).ok, "Later actor cannot overwrite newly occupied destination")
	_check(player.held_item == original and shelf.item == second_item, "Competing transfers preserve both physical items")
	_check(ItemActions.interact(player, delivery).ok, "Player can safely put the rejected item back")

	# A wall must prevent both actors from retrieving visible-through-UI stock.
	var wall := StaticBody3D.new()
	var collider := CollisionShape3D.new()
	var shape := BoxShape3D.new()
	shape.size = Vector3(0.7, 2, 0.15)
	collider.shape = shape
	wall.add_child(collider)
	arena.add_child(wall)
	wall.position = Vector3(0, 1, -0.5)
	await physics_frame
	await physics_frame
	_check(not ItemActions.interact(player, delivery).ok, "Player cannot take stock through a wall")
	_check(not ItemActions.interact(worker, delivery).ok, "Worker cannot take stock through a wall")
	_check(delivery.item == original, "Blocked actions leave delivery intact")
	wall.free()
	await physics_frame
	await physics_frame
	_check(ItemActions.interact(worker, delivery).ok, "Stock becomes reachable after obstruction is removed")
	_check(not ItemActions.interact(worker, shelf).ok, "Full hands do not exchange occupied stock implicitly")
	_check(worker.held_item == original and shelf.item == second_item, "Carry capacity is preserved")
	_check(not ItemActions.interact(player, null).ok, "Missing target fails safely")
	arena.free()
	await process_frame
	print("ITEM ACTIONS: %d assertions, %d failures" % [assertions, failures])
	quit(1 if failures > 0 else 0)
