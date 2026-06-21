# SCPI command reference

The firmware exposes a SCPI-style ASCII command interface over UART1.

## Line format

- Commands are terminated with `\r`, `\n`, or both.
- Commands are case-insensitive (the parser uppercases the line before matching).
- Numeric arguments are decimal, e.g. `12.5`, with up to 3 fractional digits; extra digits beyond the third are ignored.
- Responses are terminated with `\r\n`.
- An unrecognized command returns `ERR\r\n`.

There's no command queueing or interrupt-driven UART reception — once a line starts arriving, the firmware blocks (up to ~200 ms) until it's complete or times out, before resuming the front-panel update loop. See `firmware/microbench_psu.X/scpi.c` for the details.

## Commands

| Command | Description |
|---|---|
| `*IDN?` | Returns identification string: `TychoBECH,MicrobenchPSU,1,1.0` |
| `*RST` | Disables the output and resets both setpoints to 0 |
| `OUTP ON` / `OUTPUT ON` | Enables the output |
| `OUTP OFF` / `OUTPUT OFF` | Disables the output |
| `OUTP?` / `OUTPUT?` | Queries output state: `1` (on) or `0` (off) |
| `VOLT <value>` / `VOLTAGE <value>` | Sets the voltage setpoint, in volts (e.g. `VOLT 12.5`). Clamped to 0-24 V |
| `VOLT?` / `VOLTAGE?` | Queries the voltage setpoint, in volts |
| `CURR <value>` / `CURRENT <value>` | Sets the current limit setpoint, in amps (e.g. `CURR 0.5`). Clamped to 0-2 A |
| `CURR?` / `CURRENT?` | Queries the current limit setpoint, in amps |
| `MEAS:VOLT?` / `MEASURE:VOLTAGE?` | Returns the measured output voltage, in volts |
| `MEAS:CURR?` / `MEASURE:CURRENT?` | Returns the measured output current, in amps |
| `MEAS:TEMP?` / `MEASURE:TEMPERATURE?` | Returns the measured regulator temperature, in degC |
| `SYST:ERR?` / `SYSTEM:ERROR?` | Returns `OVERTEMP` if the over-temperature protection has tripped, otherwise `0` |

Accepted forms for `OUTP`/`OUTPUT` arguments: `ON` / `1` to enable, `OFF` / `0` to disable.

## Notes

- `OUTP ON` is refused (silently ignored) while the over-temperature protection is active. Check `SYST:ERR?` if the output won't turn on.
- `VOLT`/`CURR` writes (whether from SCPI or the front panel) are debounced before being saved to EEPROM — the value is committed about 5 seconds after the last change, so rapid scripted setpoint sweeps don't wear out the EEPROM on every step.
- Setpoint and measurement values share the same 3-decimal formatting (e.g. `12.500`), regardless of the underlying ADC resolution.

## Example session

```
*IDN?
TychoBECH,MicrobenchPSU,1,1.0

VOLT 5.0
CURR 0.5
OUTP ON
OUTP?
1

MEAS:VOLT?
4.998
MEAS:CURR?
0.102
MEAS:TEMP?
28.400

OUTP OFF
```
