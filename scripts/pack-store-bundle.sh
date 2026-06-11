#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
prg32_repo="${PRG32_REPO:-"${PRG32_ROOT:-"$repo_dir/../PRG32"}"}"
stage_dir="$repo_dir/dist/store-bundle"

rm -rf "$stage_dir"
mkdir -p "$stage_dir"
cp "$repo_dir/metadata/manifest.json" "$stage_dir/manifest.json"
cp "$repo_dir/assets/icon.png" "$stage_dir/icon.png"
cp "$repo_dir/assets/screenshot.png" "$stage_dir/screenshot.png"
cp "$repo_dir/dist/terraforge32-esp32c6.prg32" "$stage_dir/terraforge32-esp32c6.prg32"
cp "$repo_dir/dist/terraforge32-qemu.prg32" "$stage_dir/terraforge32-qemu.prg32"

python3 "$prg32_repo/tools/prg32_game.py" pack-bundle \
  --manifest "$stage_dir/manifest.json" \
  --out "$repo_dir/dist/terraforge32-store-bundle.zip"

echo "$repo_dir/dist/terraforge32-store-bundle.zip"
