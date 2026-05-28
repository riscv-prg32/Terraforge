#!/usr/bin/env bash
set -euo pipefail
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
cp tools/prg32_stub.h "$TMP/prg32.h"
gcc -std=c99 -Wall -Wextra -fsyntax-only -I"$TMP" c/game.c
