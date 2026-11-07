class_name StockItem
extends Node3D

var product_id: String
var display_name: String
var tint: Color
var magical: bool = false


func configure(id: String, title: String, color: Color, glows: bool = false) -> StockItem:
	product_id = id
	display_name = title
	tint = color
	magical = glows
	name = title.replace(" ", "")
	var body_material := StandardMaterial3D.new()
	body_material.albedo_color = color
	body_material.roughness = 0.32
	body_material.metallic = 0.15
	if glows:
		body_material.emission_enabled = true
		body_material.emission = color
		body_material.emission_energy_multiplier = 1.8
	var bottle := CylinderMesh.new()
	bottle.top_radius = 0.105
	bottle.bottom_radius = 0.12
	bottle.height = 0.24
	bottle.radial_segments = 16
	_add_mesh(bottle, Vector3(0, 0.12, 0), body_material)
	var neck := CylinderMesh.new()
	neck.top_radius = 0.045
	neck.bottom_radius = 0.065
	neck.height = 0.1
	neck.radial_segments = 12
	_add_mesh(neck, Vector3(0, 0.29, 0), body_material)
	var cork := CylinderMesh.new()
	cork.top_radius = 0.05
	cork.bottom_radius = 0.05
	cork.height = 0.045
	var cork_material := StandardMaterial3D.new()
	cork_material.albedo_color = Color("b68b57")
	_add_mesh(cork, Vector3(0, 0.35, 0), cork_material)
	var band := CylinderMesh.new()
	band.top_radius = 0.108
	band.bottom_radius = 0.113
	band.height = 0.07
	var label_material := StandardMaterial3D.new()
	label_material.albedo_color = Color("ead6ac")
	_add_mesh(band, Vector3(0, 0.16, 0), label_material)
	if glows:
		var light := OmniLight3D.new()
		light.position.y = 0.24
		light.light_color = color
		light.light_energy = 0.65
		light.omni_range = 1.8
		add_child(light)
	return self


func _add_mesh(mesh: Mesh, offset: Vector3, material: Material) -> void:
	var visual := MeshInstance3D.new()
	visual.mesh = mesh
	visual.material_override = material
	visual.position = offset
	add_child(visual)
