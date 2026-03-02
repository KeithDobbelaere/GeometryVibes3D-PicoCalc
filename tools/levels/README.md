# Levels

This folder contains Geometry Vibes 3D level files.

## What’s in here

You may see two formats:

- **`.json`** — human-editable *authoring* format (what the Python editor writes/reads).
- **`.bin`** — compact *runtime* format (streamable by column; preferred for Pico runtime).

If both exist for the same level, treat the **`.json` as the source of truth** and regenerate the `.bin` from it.

## Naming

Recommended naming:
- `level_01.json`
- `L01.bin`

(Keep the numbering stable so the game can load levels by index.)

## JSON format (authoring)

A JSON file contains:

- `width`: number of columns
- `height`: always `9`
- `start`: player start cell `{ "x": ..., "y": ... }`
- `obstacles`: list of `{ "x", "y", "shape", "mod?" }`
- `portal`: stored relative to the last column (metadata)
- `endcap`: metadata about the auto-appended endcap width

Notes:
- The **portal is not an obstacle**; it’s metadata.
- The last **6 columns** are a fixed “endcap” structure that the editor:
  - **renders as a ghost overlay**
  - **locks from editing**
  - **auto-appends on export** (so you don’t hand-edit it)

## BIN format (runtime)

Binary files are designed for streaming so the whole map does not need to be loaded into RAM.

- Header: **16 bytes** (`GVL1`, version, width, height, start, portal info, etc.)
- Columns: `width` columns, each **7 bytes (56 bits)**

Each column packs **9 cells**, each cell is **6 bits**:
- `shape_id` (4 bits, `0` means empty)
- `mod_id` (2 bits)

This yields 54 bits used + 2 spare bits per column for future expansion.

## Editing / Export

Use the Python level editor (see `tools/level_editor/`) to:
- create or modify `.json`
- export `.bin` for runtime

Typical workflow:
1. Edit `level_XX.json`
2. Export `LXX.BIN`

## Adding new levels

1. Create a new level in the editor with the desired width.
2. Place start + obstacles (shapes + modifiers).
3. Export to JSON (and BIN if needed).
4. Save into this folder following the naming convention.
5. BIN files should be stored to the SD Card, e.g. levels/L01.BIN, etc.