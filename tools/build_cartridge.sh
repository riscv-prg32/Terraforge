#!/usr/bin/env bash
set -euo pipefail
PRG32_ROOT=${PRG32_ROOT:-../PRG32}
BUILD_DIR=${BUILD_DIR:-$PRG32_ROOT/build-esp32c6}
python3 "$PRG32_ROOT/tools/prg32_game.py" build \
  c/game.c \
  --firmware-elf "$BUILD_DIR/PRG32.elf" \
  --entry-prefix terraforge32_c \
  --name terraforge32 \
  --out terraforge32.prg32
