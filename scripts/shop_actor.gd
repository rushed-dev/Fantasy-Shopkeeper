class_name ShopActor
extends CharacterBody3D

## Both the player and future restockers carry stock through this interface.
var held_item: StockItem
var carry_anchor: Node3D
var interaction_reach: float = 2.4


func _init() -> void:
	carry_anchor = Node3D.new()
	carry_anchor.name = "CarryAnchor"
	carry_anchor.position = Vector3(0.32, 1.25, -0.65)
	add_child(carry_anchor)


func action_origin() -> Vector3:
	return global_position + Vector3(0, 1.5, 0)
