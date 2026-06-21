# Bench PSU

A bench power supply built around a PIC18F16Q40, split across three boards: a main board (power supply, MCU, USB), a UI board (rotary encoder + buttons), and a display board (7-segment display + LEDs). Voltage (0-24 V) and current (0-2 A) are set via the encoder and a DAC, with live measurement readback through three I2C ADCs. Firmware features include EEPROM-backed setpoint persistence, automatic over-temperature shutdown, a standby/sleep mode, and a SCPI-style ASCII command interface over UART for remote control and automated testing. The enclosure is 3D-printed.

## Repository structure

```
firmware/microbench_psu.X/      MPLAB X project (PIC18F16Q40, XC8)
hardware/electrical/            Altium Designer projects, one per board
  main-board/                     Power supply, MCU, USB
    Parts_Library/                  Shared component library (git submodule)
  ui-board/                       Rotary encoder and buttons
  display-board/                  7-segment display and LEDs
hardware/mechanical/            SolidWorks enclosure and 3D-printed parts
hardware/bom/                   Bill of materials
docs/                            Pinout tables, SCPI command reference, notes
```

## Building the firmware

Open `firmware/microbench_psu.X` in MPLAB X IDE and build as usual. The project uses MPLAB Code Configurator (MCC) generated drivers under `mcc_generated_files/`, plus a set of hand-written modules (`display`, `encoder`, `adc`, `psu`, `eeprom_settings`, `scpi`, `i2c_bus`, `mcp47cxdx`) — see `firmware/microbench_psu.X/main.c` for the top-level control flow.

## Hardware

Each board under `hardware/electrical/` is its own Altium Designer project; see the README in each board's folder for what it covers. The main board references a shared component library as a git submodule — after cloning this repo, run:

```
git submodule update --init --recursive
```

to pull it in.

`hardware/mechanical/` holds the SolidWorks enclosure (front/back covers, knob, lightpipe) with STL/STEP exports for printing or referencing in other CAD tools.
