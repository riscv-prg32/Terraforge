# Build and release notes

## Static syntax check

A local stub header is included only for CI/static syntax validation. It is not the PRG32 runtime.

```bash
./tools/static_check.sh
```

## Cartridge build

Clone PRG32 next to this repository and build the resident firmware first. Then run:

```bash
export PRG32_ROOT=../PRG32
export BUILD_DIR=$PRG32_ROOT/build-esp32c6
./tools/build_cartridge.sh
```

The script calls:

```bash
python3 "$PRG32_ROOT/tools/prg32_game.py" build \
  c/game.c \
  --firmware-elf "$BUILD_DIR/PRG32.elf" \
  --entry-prefix terraforge32_c \
  --name terraforge32 \
  --out terraforge32.prg32
```

## Release checklist

1. Run `assets/generate_assets.py` after editing PNG/WAV masters.
2. Run `./tools/static_check.sh`.
3. Build `terraforge32.prg32` with a real PRG32 firmware ELF.
4. Test on the PRG32 emulator or target board.
5. Verify controls: rotate, move, run, dig, place, select, restart.
6. Verify clean-room requirement: no external commercial art, audio, names, maps, or lore.
7. Tag the release and attach the `.prg32` cartridge plus this source package.

## Notes for classroom modification

Useful exercises:

- Increase `RAYS` or reduce `COL_W` and compare frame rate.
- Change the 32-entry trigonometry table to 64 entries and evaluate smoother rotation.
- Add texture sampling from `tf_tiles` instead of flat shaded columns.
- Modify `hashed_wall()` and the carved rooms to study procedural generation.
- Add additional floor effects using `floor_tile`.


## Optional stereo audio release path

The repository ships a true stereo WAV master at `assets/wav/terraforge32_stereo_loop.wav`. The default C cartridge remains compatible with the minimal PRG32 audio API by using the compact score in `assets/include/terraforge_music.h`. For an Audio Plus release, convert and pack the WAV or an equivalent tracker/MIDI version with the PRG32 audio tools and include the generated AUDIO block when building the cartridge.
