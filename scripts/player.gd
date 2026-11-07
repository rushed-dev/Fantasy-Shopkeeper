class_name ShopPlayer
extends ShopActor

var camera: Camera3D
var move_speed: float = 3.0
var mouse_sensitivity: float = 0.0022


func _init() -> void:
	process_mode = Node.PROCESS_MODE_PAUSABLE
	collision_layer = 4
	collision_mask = 1
	var capsule := CapsuleShape3D.new()
	capsule.radius = 0.25
	capsule.height = 1.7
	var collider := CollisionShape3D.new()
	collider.shape = capsule
	collider.position.y = 0.85
	add_child(collider)
	camera = Camera3D.new()
	camera.name = "Eyes"
	camera.position.y = 1.58
	camera.fov = 75.0
	camera.near = 0.05
	add_child(camera)
	carry_anchor.reparent(camera, false)
	carry_anchor.position = Vector3(0.38, -0.38, -0.72)
	carry_anchor.rotation_degrees = Vector3(-8, 0, 12)


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		rotate_y(-event.relative.x * mouse_sensitivity)
		camera.rotation.x = clampf(camera.rotation.x - event.relative.y * mouse_sensitivity, -1.35, 1.35)


func _physics_process(delta: float) -> void:
	if Input.mouse_mode != Input.MOUSE_MODE_CAPTURED:
		velocity = Vector3.ZERO
		return
	var movement := Input.get_vector("move_left", "move_right", "move_forward", "move_back")
	var direction := global_basis * Vector3(movement.x, 0, movement.y)
	velocity.x = direction.x * move_speed
	velocity.z = direction.z * move_speed
	if not is_on_floor():
		velocity.y -= 18.0 * delta
	else:
		velocity.y = 0.0
	move_and_slide()


func action_origin() -> Vector3:
	return camera.global_position


func aimed_slot() -> StockSlot:
	var start := camera.global_position
	var finish := start - camera.global_basis.z * (interaction_reach + 0.15)
	var ray := PhysicsRayQueryParameters3D.create(start, finish)
	ray.exclude = [get_rid()]
	ray.collision_mask = 3
	var hit := get_world_3d().direct_space_state.intersect_ray(ray)
	if hit.is_empty():
		return null
	return hit.collider as StockSlot
