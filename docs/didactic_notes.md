# Didactic notes: first-person block rendering on PRG32

TerraForge 32 3D is meant to be read as a teaching cartridge. The source avoids floating point and keeps each subsystem visible in `c/game.c`.

## 1. World representation

The world is a `64 x 32` array of block identifiers. `TILE_AIR` is empty space; every other value is treated as a wall for the first-person renderer. A second `floor_tile` array stores floor effects such as water and lava. This separation lets the game render vertical block walls while still supporting floor hazards.

The generator combines deterministic hash noise with hand-carved rooms. This gives repeatable cave-like variation while ensuring that the starting hub, corridors, relic rooms, and learning-friendly loops always exist.

## 2. Fixed-point player state

Player, drone, and particle positions use Q8 fixed-point coordinates: one block is `256` units. This is a good compromise for small systems: movement is smoother than whole-tile motion, but all arithmetic stays integer-only.

The viewing angle is stored as one of 32 directions. `cos32` and `sin32` are precomputed tables scaled by 256, so movement and projection avoid `float`, `sin`, and `cos`.

## 3. Ray casting

The renderer casts 80 rays per frame. Each ray corresponds to a four-pixel-wide vertical stripe, so `80 x 4 = 320`, matching the PRG32 display width.

For every ray:

1. Start at the player position.
2. Step forward in fixed-point increments.
3. Track the current map cell.
4. Stop when a solid block is reached.
5. Estimate projected wall height as `constant / corrected_distance`.
6. Draw a filled vertical column with a material color.

The code applies a small distance correction using the ray offset angle. This reduces fisheye distortion while remaining integer-only.

## 4. Depth shading and material feedback

The separated palette in `assets/include/terraforge_assets.h` provides foreground and background colors for each tile. The 3D renderer uses these colors as near/far shades. Ore, lava, and crystals animate by alternating colors, creating activity without large textures.

## 5. Interaction model

The same `cast_ray()` function powers both rendering and gameplay. The center ray is used for targeting:

- `A` digs the first solid block under the crosshair when close enough.
- `Down + A` places the selected inventory block in the last empty cell before the wall.

This is a useful teaching pattern: one spatial query serves rendering, collision-like targeting, and UI feedback.

## 6. Billboards

Drones and particles are projected using dot products against the player's forward and side vectors. If an object is in front of the camera and inside a broad field of view, it is drawn as a scaled rectangle. This demonstrates the basic idea behind sprite billboards in early 3D games.

## 7. Collision and hazards

Collision tests sample four points around the player radius. This prevents the player from slipping through block corners without needing a physics engine. Water and lava are floor effects in the current cell: water slowly heals; lava rapidly damages.

## 8. Why this pushes PRG32

The cartridge draws a full-screen first-person view, minimap, particles, animated drones, HUD, and audio feedback while keeping all state in compact static arrays. It is intentionally CPU-visible and algorithmic rather than asset-heavy, which makes it a good PRG32 classroom example.


## Stereo soundtrack lesson

The soundtrack is intentionally stored outside `c/game.c` in two forms:

1. `assets/wav/terraforge32_stereo_loop.wav` is the authored stereo master. It uses two independent, original melodic layers plus a low pulse so students can inspect stereo interleaving in a standard WAV editor.
2. `assets/include/terraforge_music.h` is a compact score representation with left/right frequency fields and a `pan_hint`. `game.c` schedules this table with `music_tick()`. On minimal mono PRG32 builds it alternates the two voices; on Audio Plus workstations it can be used as the starting point for a real stereo AUDIO-block implementation.

This separation makes three teaching points explicit: asset provenance, platform fallback design, and the difference between a high-quality source asset and a constrained runtime representation.
