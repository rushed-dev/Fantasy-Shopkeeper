#!/usr/bin/env bash
# Cross-build Windows and a locally verifiable Linux release.
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
bash tools/check-godot-linux.sh
bash tools/setup-export-templates-linux.sh
export XDG_CACHE_HOME="${XDG_CACHE_HOME:-$PWD/.tools/cache}"
export XDG_DATA_HOME="${XDG_DATA_HOME:-$PWD/.tools/data}"
export XDG_CONFIG_HOME="${XDG_CONFIG_HOME:-$PWD/.tools/config}"
engine="$PWD/.tools/godot/Godot_v4.7.2-stable_linux.x86_64"
mkdir -p builds/windows builds/linux
for target in 'Windows Desktop' Linux; do
  "$engine" --headless --path . --export-release "$target" 2>&1 | tee test-results/export.log
  if rg -n 'SCRIPT ERROR:|ERROR:' test-results/export.log; then exit 1; fi
done
builds/linux/FantasyShopkeeper.x86_64 --headless -- --smoke-test 2>&1 | tee test-results/export-smoke.log
if rg -n 'SCRIPT ERROR:|ERROR:' test-results/export-smoke.log; then exit 1; fi
"$engine" --headless --fixed-fps 60 --main-pack "$PWD/builds/linux/FantasyShopkeeper.x86_64" \
  --script "$PWD/tests/checkout_test.gd" 2>&1 | tee test-results/export-checkout.log
if rg -n 'SCRIPT ERROR:|ERROR:' test-results/export-checkout.log; then exit 1; fi
"$engine" --headless --fixed-fps 60 --main-pack "$PWD/builds/linux/FantasyShopkeeper.x86_64" \
  --script "$PWD/tests/day_loop_test.gd" 2>&1 | tee test-results/export-day-loop.log
if rg -n 'SCRIPT ERROR:|ERROR:' test-results/export-day-loop.log; then exit 1; fi
python3 - <<'PY'
import hashlib, zipfile
from pathlib import Path
for platform in ('windows', 'linux'):
    directory = Path('builds') / platform
    (directory / 'README.txt').write_text(
        'Fantasy Shopkeeper - Two Days prototype\n\n'
        'WASD/arrows: move. Mouse: look. E: handle stock or open ledger. Escape: return/pause.\n'
        'Stock all six bottles on their labelled shelves.\n'
        'At the counter, press E to set prices and explicitly open. Invite three customers each day.\n'
        'Enter their basket total, then count change with 1/10/100 pouches.\n'
        'Click or hold pouches, or use keys 1/2/3. Click tray stacks to take one back.\n'
        'Shift+1/2/3 removes coins. Confirm when ready; mistakes allow correction.\n'
        'Leaving the counter preserves the sale. Nobody times out.\n'
        'Close the shop for evening orders and finances. Shop expense is 12 bits daily.\n'
        'Pay an order, then sleep to receive it on day two. Recovery loan: 112 received, 118 due.\n'
        'Morning checkpoints save automatically; restarting restores the morning start.\n'
        'Workers and the optional encounter are not implemented yet.\n'
        'Windows export has not been tested on Windows.\n'
        'Engine licence and third-party notices: https://godotengine.org/license/\n')
    for name in ('GODOT-LICENSE.txt', 'GODOT-COPYRIGHT.txt'):
        (directory / name).write_bytes((Path('.tools/godot') / name).read_bytes())
    archive = Path('builds') / ('FantasyShopkeeper-' + platform + '.zip')
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as package:
        for path in sorted(directory.iterdir()):
            package.write(path, path.name)
    digest = hashlib.file_digest(archive.open('rb'), 'sha256').hexdigest()
    archive.with_suffix('.zip.sha256').write_text(digest + '  ' + archive.name + '\n')
    print(archive, digest)
PY
