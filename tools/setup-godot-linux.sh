#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
engine_dir="$project_root/.tools/godot"
release_tag="4.7.2-stable"
engine_name="Godot_v${release_tag}_linux.x86_64"
if [[ "$(uname -m)" != "x86_64" ]]; then
  printf '%s\n' 'This setup script targets Linux x86_64. Install the matching official Godot build for this host.' >&2
  exit 1
fi
mkdir -p "$engine_dir"
if [[ ! -x "$engine_dir/$engine_name" ]]; then
  curl --fail --location --retry 3 \
    "https://github.com/godotengine/godot-builds/releases/download/${release_tag}/${engine_name}.zip" \
    --output "$engine_dir/engine-linux.zip"
  printf '%s  %s\n' \
    '9aa00f7a605200940bce3027a567b782f49bd8e940dd06ae9e987bd65aee1b1467edd56ed84fcdcbdd44354bf613bdbb4e5d2913e925850368e150c59ed54c65' \
    "$engine_dir/engine-linux.zip" | sha512sum --check
  python3 -m zipfile -e "$engine_dir/engine-linux.zip" "$engine_dir"
  chmod +x "$engine_dir/$engine_name"
fi
touch "$engine_dir/_sc_"
"$engine_dir/$engine_name" --headless --version
printf '%s\n' "Godot is ready at $engine_dir/$engine_name"
