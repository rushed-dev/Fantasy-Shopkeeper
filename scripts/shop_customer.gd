class_name ShopCustomer
extends Node3D

signal stock_changed
signal at_counter
signal departed
signal remarked(message: String)

var sale: CounterSale
var shelves: Array[StockSlot] = []
var requests: Array[String] = []
var prices: Dictionary
var basket: Node3D
var current_product: String
var destination: StockSlot
var route: Array[Vector3] = []
var state := "entering"
var speed := 1.8
var missing := 0

func _init() -> void:
	process_mode = Node.PROCESS_MODE_PAUSABLE
	basket = Node3D.new()
	basket.position = Vector3(0, 0.9, 0.35)
	add_child(basket)
	var coat := CapsuleMesh.new()
	coat.radius = 0.24
	coat.height = 1.1
	_visual(coat, Vector3(0, 0.7, 0), Color("617f80"))
	var head := SphereMesh.new()
	head.radius = 0.18
	head.height = 0.36
	_visual(head, Vector3(0, 1.42, 0), Color("d4b48a"))
	for x in [-0.06, 0.06]:
		var eye := SphereMesh.new()
		eye.radius = 0.018
		eye.height = 0.036
		_visual(eye, Vector3(x, 1.46, 0.165), Color("302c34"))
	var hat := CylinderMesh.new()
	hat.top_radius = 0.12
	hat.bottom_radius = 0.3
	hat.height = 0.27
	_visual(hat, Vector3(0, 1.68, 0), Color("836392"))
	var tray := BoxMesh.new()
	tray.size = Vector3(0.85, 0.08, 0.5)
	_visual(tray, Vector3(0, 0.86, 0.35), Color("927044"))

func _visual(mesh: Mesh, at: Vector3, tint: Color) -> void:
	var visual := MeshInstance3D.new()
	visual.mesh = mesh
	visual.position = at
	var material := StandardMaterial3D.new()
	material.albedo_color = tint
	visual.material_override = material
	add_child(visual)

func begin(wanted: Array[String], stock: Array[StockSlot], catalogue: Dictionary, amount: int) -> void:
	requests = wanted.duplicate()
	shelves = stock
	prices = catalogue
	sale = CounterSale.new(amount)
	position = Vector3(0, 0, 3.75)
	route = [Vector3(0, 0, -1.65)]

func _process(delta: float) -> void:
	if not route.is_empty():
		position = position.move_toward(route[0], speed * delta)
		if position.distance_to(route[0]) < 0.01:
			route.pop_front()
		return
	match state:
		"entering":
			_next_product()
		"browsing":
			if not is_instance_valid(destination) or not sale.take(destination, prices.get(destination.accepted_product, 0), basket):
				requests.push_front(current_product)
				remarked.emit("Someone got there first. I'll check the display again.")
			else:
				stock_changed.emit()
			_next_product()
		"to_counter":
			state = "waiting"
			at_counter.emit()
		"leaving":
			state = "done"
			departed.emit()
			queue_free()

func _next_product() -> void:
	if requests.is_empty():
		if sale.items.is_empty():
			remarked.emit("Nothing I wanted was on the shelves today. I'll try another time.")
			leave()
		else:
			state = "to_counter"
			route = [Vector3(0, 0, -1.65), Vector3(0, 0, 0.1), Vector3(2.8, 0, 0.1)]
		return
	var product: String = requests.pop_front()
	current_product = product
	destination = null
	var title := product
	for slot in shelves:
		if slot.accepted_product == product:
			title = slot.display_name
		if slot.accepted_product == product and is_instance_valid(slot.item):
			destination = slot
			break
	if destination == null:
		missing += 1
		remarked.emit("No %s on the shelves? I'll go without it today." % title)
		# Resolve one request per frame; no recursion or waiting penalty.
		state = "entering"
		return
	state = "browsing"
	route = [Vector3(position.x, 0, -1.65), Vector3(destination.position.x, 0, -1.65)]

func leave() -> void:
	state = "leaving"
	route = [Vector3(0, 0, position.z), Vector3(0, 0, 3.75)]
