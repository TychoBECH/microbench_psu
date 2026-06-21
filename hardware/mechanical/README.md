# Mechanical

SolidWorks enclosure for the PSU.

- `Assembly_microbench_psu.SLDASM` — top-level assembly
- `Front Cover.SLDPRT`, `Back Cover.SLDPRT` — main enclosure shells
- `Knob.SLDPRT` — encoder knob
- `Lightpipe.SLDPRT` — light pipe for the front panel LEDs

## exports/

Print-ready and interchange exports. Treat this as regenerable output, not source of truth.

- `stl/` — print-ready meshes for the enclosure parts (covers, bezel, case, knob)
- `stp/` — STEP exports, including board outlines imported from each Altium project (`main-board.step`, `ui-board.step`, `display-board.step`) for mechanical fit-checking, plus third-party part models (e.g. `1455J801BK.step` for the enclosure base)
