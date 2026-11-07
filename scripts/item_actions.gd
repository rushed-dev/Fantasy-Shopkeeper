class_name ItemActions
extends RefCounted

## The sole gameplay boundary for moving items. No actor-specific privileges.
## Revalidate at commit: two actors can target the same item or destination.
static func inspect(actor: ShopActor, slot: StockSlot) -> Dictionary:
	if not is_instance_valid(actor) or not is_instance_valid(slot):
		return _result(false, "Choose a stock space.")
	if not actor.is_inside_tree() or not slot.is_inside_tree():
		return _result(false, "Stock is not in the shop.")
	var destination := slot.global_position + Vector3(0, 0.18, 0)
	if actor.action_origin().distance_to(destination) > actor.interaction_reach:
		return _result(false, "Move a little closer.")
	var ray := PhysicsRayQueryParameters3D.create(actor.action_origin(), destination)
	ray.exclude = [actor.get_rid()]
	ray.collision_mask = 3
	var obstruction := actor.get_world_3d().direct_space_state.intersect_ray(ray)
	if not obstruction.is_empty() and obstruction.collider != slot:
		return _result(false, "There is something in the way.")
	if is_instance_valid(actor.held_item):
		if is_instance_valid(slot.item):
			return _result(false, "This space is already occupied.")
		if not slot.accepts(actor.held_item):
			return _result(false, "This shelf is for %s." % slot.display_name)
		return _result(true, "Place %s" % actor.held_item.display_name)
	if not is_instance_valid(slot.item):
		return _result(false, "Empty %s" % ("shelf space" if slot.is_shelf else "delivery space"))
	return _result(true, "Pick up %s" % slot.item.display_name)


static func interact(actor: ShopActor, slot: StockSlot) -> Dictionary:
	var result := inspect(actor, slot)
	if not result.ok:
		return result
	if is_instance_valid(actor.held_item):
		var carried := actor.held_item
		actor.held_item = null
		slot.item = carried
		carried.reparent(slot, false)
		carried.transform = Transform3D.IDENTITY
		return _result(true, "Placed %s" % carried.display_name)
	var picked_up := slot.item
	slot.item = null
	actor.held_item = picked_up
	picked_up.reparent(actor.carry_anchor, false)
	picked_up.transform = Transform3D.IDENTITY
	return _result(true, "Picked up %s" % picked_up.display_name)


static func _result(ok: bool, message: String) -> Dictionary:
	return {"ok": ok, "message": message}
