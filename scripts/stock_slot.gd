class_name StockSlot
extends StaticBody3D

var item: StockItem
var accepted_product: String = ""
var display_name: String = "Delivery space"
var is_shelf: bool = false
var highlight: MeshInstance3D


func _init() -> void:
	collision_layer = 2
	collision_mask = 0
	var hitbox := CollisionShape3D.new()
	var shape := BoxShape3D.new()
	shape.size = Vector3(0.48, 0.42, 0.48)
	hitbox.shape = shape
	hitbox.position.y = 0.19
	add_child(hitbox)
	var mesh := CylinderMesh.new()
	mesh.top_radius = 0.21
	mesh.bottom_radius = 0.21
	mesh.height = 0.012
	mesh.radial_segments = 32
	highlight = MeshInstance3D.new()
	highlight.mesh = mesh
	highlight.position.y = 0.008
	var material := StandardMaterial3D.new()
	material.albedo_color = Color("8b7450")
	material.roughness = 0.9
	highlight.material_override = material
	add_child(highlight)


func accepts(stock: StockItem) -> bool:
	return accepted_product.is_empty() or accepted_product == stock.product_id


func set_highlight(active: bool) -> void:
	var material := highlight.material_override as StandardMaterial3D
	material.albedo_color = Color("f4d28b") if active else Color("8b7450")
	material.emission_enabled = active
	material.emission = Color("9c703c")
	material.emission_energy_multiplier = 0.6


## Initial setup only. Gameplay transfers go through ItemActions.
func seed(stock: StockItem) -> void:
	assert(item == null, "Cannot seed an occupied slot")
	assert(accepts(stock), "Seed product must match slot")
	item = stock
	add_child(stock)
	stock.transform = Transform3D.IDENTITY
