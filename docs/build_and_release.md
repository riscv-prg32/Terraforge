# Build, deploy, and publish notes

## Static syntax check

A local stub header is included only for CI/static syntax validation. It is not the PRG32 runtime.

```bash
./tools/static_check.sh
```

## Cartridge build

Clone PRG32 next to this repository, or set `PRG32_REPO` when it lives
elsewhere:

```bash
export PRG32_REPO=/path/to/PRG32
```

Build for ESP32-C6 hardware:

```bash
export PRG32_ARCHITECTURE=esp32c6
scripts/build.sh
```

The script writes `dist/terraforge32-esp32c6.prg32`. By default this is a
portable ABI-table cartridge and is not tied to one firmware ELF.

Build for QEMU:

```bash
export PRG32_ARCHITECTURE=qemu
scripts/build.sh
```

The script writes `dist/terraforge32-qemu.prg32`.

Current PRG32 `main` builds portable cartridges only. The code uses the
public timed-note audio API and default instrument for tones and melodies.

`tools/build_cartridge.sh` remains as a compatibility wrapper around
`scripts/build.sh`.

## Upload to a board

```bash
PYTHONPATH="$PRG32_REPO" python3 -m prg32 esp32c6 upload \
  dist/terraforge32-esp32c6.prg32 \
  --url http://192.168.4.1
```

Use the board address shown in PRG32 setup mode when the board is on classroom
Wi-Fi.

## Stage in QEMU

```bash
PYTHONPATH="$PRG32_REPO" python3 -m prg32 qemu upload \
  dist/terraforge32-qemu.prg32 \
  --flash "$PRG32_REPO/build-qemu/qemu_flash.bin" \
  --partitions "$PRG32_REPO/partitions_prg32.csv"
```

## Pack a CartridgeStore bundle

Build both architecture variants first, then pack the flat Store bundle:

```bash
export PRG32_ARCHITECTURE=esp32c6
scripts/build.sh

export PRG32_ARCHITECTURE=qemu
scripts/build.sh

scripts/pack-store-bundle.sh
```

The bundle is `dist/terraforge32-store-bundle.zip`.

## Publish to CartridgeStore

```bash
PYTHONPATH="$PRG32_REPO" python3 -m prg32 store publish-bundle \
  dist/terraforge32-store-bundle.zip \
  --store-url http://192.168.1.42:5080 \
  --token "$PRG32_STORE_TOKEN"
```

The store URL and token may also be kept in `~/.prg32/config.json`.

## Release checklist

1. Run `assets/generate_assets.py` after editing PNG/WAV masters.
2. Run `./tools/static_check.sh`.
3. Build `dist/terraforge32-esp32c6.prg32` and `dist/terraforge32-qemu.prg32`.
4. Test on the PRG32 emulator or target board.
5. Pack `dist/terraforge32-store-bundle.zip`.
6. Verify controls: rotate, move, run, dig, place, select, restart.
7. Verify clean-room requirement: no external commercial art, audio, names, maps, or lore.
8. Tag the release and attach the `.prg32` cartridges, store bundle, and source package.

## Notes for classroom modification

Useful exercises:

- Increase `RAYS` or reduce `COL_W` and compare frame rate.
- Change the 32-entry trigonometry table to 64 entries and evaluate smoother rotation.
- Add texture sampling from `tf_tiles` instead of flat shaded columns.
- Modify `hashed_wall()` and the carved rooms to study procedural generation.
- Add additional floor effects using `floor_tile`.


## Optional stereo audio release path

The repository ships a true stereo WAV master at `assets/wav/terraforge32_stereo_loop.wav`. The default C cartridge remains compatible with the minimal PRG32 audio API by using the compact score in `assets/include/terraforge_music.h`. For an Audio Plus release, convert and pack the WAV or an equivalent tracker/MIDI version with the PRG32 audio tools and include the generated AUDIO block when building the cartridge.
