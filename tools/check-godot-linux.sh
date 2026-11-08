#!/usr/bin/env bash
# Run engine checks and reject script errors even when Godot exits zero.
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
export XDG_CACHE_HOME="${XDG_CACHE_HOME:-$PWD/.tools/cache}"
export XDG_DATA_HOME="${XDG_DATA_HOME:-$PWD/.tools/data}"
export XDG_CONFIG_HOME="${XDG_CONFIG_HOME:-$PWD/.tools/config}"
mkdir -p "$XDG_CACHE_HOME" "$XDG_DATA_HOME" "$XDG_CONFIG_HOME" test-results
engine="$PWD/.tools/godot/Godot_v4.7.2-stable_linux.x86_64"
check() {
  local name="$1"
  shift
  "$engine" "$@" 2>&1 | tee "test-results/$name.log"
  if rg -n 'SCRIPT ERROR:|ERROR:' "test-results/$name.log"; then
    return 1
  fi
}
check import --headless --editor --path . --import --quit
check item-actions --headless --path . --script res://tests/item_actions_test.gd
check scene --headless --path . --script res://tests/shop_scene_test.gd
check checkout --headless --fixed-fps 60 --path . --script res://tests/checkout_test.gd
check smoke --headless --path . -- --smoke-test
