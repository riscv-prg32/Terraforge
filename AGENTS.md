# AGENTS.md

Guidance for automated coding agents and human maintainers working on TerraForge 32.

## Project intent

TerraForge 32 is an original PRG32 educational cartridge. Keep it copyright-safe: do not import Minecraft names, textures, sounds, recipes, UI layouts, maps, mobs, story elements, or other protected expression from commercial games. Broad ideas such as block worlds, digging, crafting-like inventory, caves, and first-person exploration are acceptable only when expressed with original code and assets.

## Build and validation

Use the repository scripts first:

```bash
./tools/static_check.sh
./tools/build_cartridge.sh
```

`static_check.sh` uses `tools/prg32_stub.h` and is intended to run without ESP-IDF. `build_cartridge.sh` requires `PRG32_ROOT` and `BUILD_DIR` to point to a built PRG32 firmware tree.

## Source and assets

- Keep runtime logic in `c/game.c`.
- Keep editable art/audio in `assets/png` and `assets/wav`.
- Keep generated C-facing data in `assets/include`.
- Update `assets/manifest.json` whenever adding or replacing assets.
- Prefer deterministic generators for procedural assets so students can reproduce them.

## Audio policy

The shipped soundtrack must remain original and clean-room. The stereo master is `assets/wav/terraforge32_stereo_loop.wav`; the compact runtime score is `assets/include/terraforge_music.h`. Maintain mono fallback behavior unless the minimum PRG32 API in this cartridge is deliberately raised.

## Didactic documentation

Any substantial renderer, gameplay, or asset-pipeline change must update `README.md` and `docs/didactic_notes.md`. Explain constraints, trade-offs, and why the implementation is suitable for PRG32/RISC-V teaching.

## Style

Favor compact, readable C with integer/fixed-point math. Avoid heap allocation, hidden dependencies, or large generated blobs in `c/game.c`.
