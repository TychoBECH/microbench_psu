# Bench PSU

A compact lab bench power supply built around a PIC18F16Q40. Voltage (0–24 V) and current (0–2 A) are set via a rotary encoder and DAC, with live measurement readback through three I2C ADCs. Features include EEPROM-backed setpoint persistence, automatic over-temperature shutdown, a standby/sleep mode, and a SCPI-style ASCII command interface over UART for remote control and automated testing. Designed for a 3D-printed enclosure.

## Repository structure

```
firmware/I2C_development.X/   MPLAB X project (PIC18F16Q40, XC8)
hardware/schematic/           Schematic source and exports
hardware/pcb/                 PCB layout source, gerbers, renders
hardware/bom/                 Bill of materials
docs/                         Pinout tables, SCPI command reference, notes
```

## Building the firmware

Open `firmware/I2C_development.X` in MPLAB X IDE and build as usual. The project uses MPLAB Code Configurator (MCC) generated drivers under `mcc_generated_files/`.
