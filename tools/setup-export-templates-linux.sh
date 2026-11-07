#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p .tools/godot
archive="$PWD/.tools/godot/export_templates.tpz"
if [[ ! -f "$archive" ]]; then
  curl --fail --location --retry 3 \
    https://github.com/godotengine/godot-builds/releases/download/4.7.2-stable/Godot_v4.7.2-stable_export_templates.tpz \
    --output "$archive"
fi
printf '%s  %s\n' \
  'ca4d71c4d7b81dfc15d1a98baa07534aa95b03fdda78a0075b06672e1648d2e5f40980c9adc28d23e1b92e732ee7bf3461997aa804af74ec2fcd7a93ccb84079' \
  "$archive" | sha512sum --check
python3 - "$archive" <<'PY'
import sys, zipfile
from pathlib import Path
archive = Path(sys.argv[1])
destination = archive.parent / 'editor_data/export_templates/4.7.2.stable'
destination.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(archive) as package:
    for name in ('windows_release_x86_64.exe', 'windows_debug_x86_64.exe',
                 'linux_release.x86_64', 'linux_debug.x86_64'):
        (destination / name).write_bytes(package.read('templates/' + name))
PY

for notice in LICENSE COPYRIGHT; do
  curl --fail --location --retry 3 \
    "https://raw.githubusercontent.com/godotengine/godot/4.7.2-stable/$notice.txt" \
    --output ".tools/godot/GODOT-$notice.txt"
done
