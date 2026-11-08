class_name FenStory
extends RefCounted

var decision := "unseen"
var outcome := "unresolved"
var reward := "none"

func offer(day: int, visitor: String) -> bool:
	if day != 1 or visitor != "fen" or decision != "unseen":
		return false
	decision = "offered"
	return true

func choose(action: String, day: int) -> bool:
	if day != 1 or decision not in ["offered", "deferred"]:
		return false
	match action:
		"promise": decision = "promised"
		"defer": decision = "deferred"
		"decline": decision = "declined"
		_: return false
	return true

func night_passes() -> void:
	if decision in ["offered", "deferred"]:
		decision = "ignored"

# Outcome depends on a committed purchase of an actual physical item.
func paid_visit(sale: CounterSale, day: int, visitor: String) -> bool:
	if sale == null or day != 2 or visitor != "fen" or not sale.completed or outcome != "unresolved":
		return false
	var bought_light := false
	for item in sale.items:
		if is_instance_valid(item) and item.product_id == "light":
			bought_light = true
	return _resolve(bought_light)

func departed(day: int, visitor: String) -> bool:
	if day != 2 or visitor != "fen":
		return false
	return _resolve(false)

func close_day_two() -> void:
	_resolve(false)

func _resolve(bought_light: bool) -> bool:
	if outcome != "unresolved":
		return false
	if decision == "promised":
		outcome = "fulfilled" if bought_light else "missed"
		if bought_light:
			reward = "pending"
	else:
		outcome = "ordinary"
	return true

func claim_bonus() -> bool:
	if reward != "pending":
		return false
	reward = "claimed"
	return true

func has_note() -> bool:
	return decision != "unseen"

func note_title(day: int = 1) -> String:
	if reward == "pending":
		return "Fen • a thank-you parcel (optional)"
	if outcome != "unresolved":
		return "Fen • the Lantern Walk (optional)"
	if decision == "promised":
		return "Fen • starlight on today's second visit" if day == 2 else "Fen • starlight on tomorrow's visit"
	if decision == "deferred":
		return "Fen's question • saved for later"
	return "Fen has a small question (optional)" if decision == "offered" else "Fen • your earlier conversation"

func to_state() -> Dictionary:
	return {"decision": decision, "outcome": outcome, "reward": reward}

static func from_state(data: Dictionary) -> FenStory:
	var story := FenStory.new()
	# Saves made before the encounter existed remain ordinary mornings.
	if data.is_empty():
		return story
	if data.get("decision") not in ["unseen", "offered", "deferred", "promised", "declined", "ignored"]:
		return null
	if data.get("outcome") not in ["unresolved", "fulfilled", "missed", "ordinary"] or data.get("reward") not in ["none", "pending", "claimed"]:
		return null
	if data.outcome == "fulfilled":
		if data.decision != "promised" or data.reward == "none":
			return null
	elif data.reward != "none":
		return null
	if (data.outcome == "missed" and data.decision != "promised") or (data.outcome == "ordinary" and data.decision == "promised"):
		return null
	story.decision = data.decision
	story.outcome = data.outcome
	story.reward = data.reward
	return story
