# Electrical

Three separate Altium Designer projects, one per board:

| Folder | Board |
|---|---|
| `main-board/` | Power supply, MCU, USB |
| `ui-board/` | Encoder and buttons |
| `display-board/` | Display and LEDs |

Each board folder holds its own Altium project files (`.PrjPcb`, `.SchDoc`, `.PcbDoc`, library files, etc.) plus an `outputs/` subfolder for generated manufacturing/assembly outputs (gerbers, drill files, pick-and-place, assembly drawings, exported PDFs). Treat `outputs/` as regenerable, not source of truth.
