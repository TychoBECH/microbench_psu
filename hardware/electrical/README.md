# Electrical

Three separate Altium Designer projects, one per board:

| Folder | Board |
|---|---|
| `main-board/` | Power supply, MCU, USB |
| `ui-board/` | Encoder and buttons |
| `display-board/` | Display and LEDs |

Each board folder holds its own Altium project files (`.PrjPcb`, `.SchDoc`, `.PcbDoc`, etc.) plus an `outputs/` subfolder for generated manufacturing/assembly outputs (gerbers, NC drill files, STEP exports). Treat `outputs/` as regenerable, not source of truth.

The main board references a shared component library (schematic symbols, PCB footprints, 3D models) as a git submodule at `main-board/Parts_Library/`, pulled from a separate repo. After cloning, run `git submodule update --init --recursive` to fetch it.
