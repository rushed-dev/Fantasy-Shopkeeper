class_name CounterSale
extends RefCounted

const DENOMINATIONS := [1, 10, 100]
var items: Array[StockItem] = []
var prices: Array[int] = []
var payment: int
var total_accepted := false
var completed := false
var coins := {1: 0, 10: 0, 100: 0}

func _init(amount: int = 0) -> void:
	payment = amount

# Recheck ownership at pickup: a player may have moved this item while browsing.
func take(slot: StockSlot, price: int, basket: Node3D) -> bool:
	if completed or total_accepted or not slot.is_shelf or not is_instance_valid(slot.item) or price < 0:
		return false
	var item := slot.item
	slot.item = null
	item.reparent(basket, false)
	item.position = Vector3((items.size() - 1) * 0.24, 0, 0)
	items.append(item)
	prices.append(price)
	return true

func total() -> int:
	var amount := 0
	for price in prices:
		amount += price
	return amount

func submit_total(text: String) -> Dictionary:
	if completed or items.is_empty():
		return _result(false, "There is no basket to total.")
	var entered := text.strip_edges()
	if not entered.is_valid_int() or int(entered) != total():
		return _result(false, "That total doesn't match the basket. Check the prices and try again.")
	if payment < total():
		return _result(false, "The customer's payment does not cover this basket.")
	total_accepted = true
	return _result(true, "Total agreed. Count out the customer's change, then confirm.")

func add_coin(value: int) -> bool:
	if completed or not total_accepted or not DENOMINATIONS.has(value):
		return false
	coins[value] += 1
	return true

func remove_coin(value: int) -> bool:
	if completed or not DENOMINATIONS.has(value) or coins[value] == 0:
		return false
	coins[value] -= 1
	return true

func change_value() -> int:
	return coins[1] + coins[10] * 10 + coins[100] * 100

# This is the only commit boundary. Failure preserves basket, payment and coins.
func confirm() -> Dictionary:
	if completed:
		return _result(false, "This sale has already been paid.")
	if not total_accepted:
		return _result(false, "Agree the basket total first.")
	if change_value() != payment - total():
		return _result(false, "The change isn't right yet. You can add or take back coins.")
	completed = true
	return _result(true, "Thank you! That's everything. A little magic for the road.")

func _result(ok: bool, message: String) -> Dictionary:
	return {"ok": ok, "message": message}
