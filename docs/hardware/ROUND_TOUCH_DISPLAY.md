# Round touch display JXI V1.0

Status: display and touch interfaces confirmed electrically.

## Expected hardware

- 1.28-inch round IPS LCD, 240 x 240 pixels;
- GC9A01 LCD controller;
- 4-wire, write-only SPI display interface;
- CST816D capacitive touch controller (`0x15`, chip ID `0xB6`);
- I2C touch interface;
- active-high, externally controllable backlight input.

The PCB exposes two connectors. Always follow the signal names printed beside
the connector; do not infer contact order from a cable's wire colours.

## Test wiring

USB connector on the ProMicro nRF52840 is at the top.

| Display signal | nRF52840 pin | Board silk |
| --- | --- | --- |
| `VIN` | 3.3 V | `VCC` |
| `GND` | Ground | `GND` |
| `SCL` | P0.20 / SPI SCK | `020` |
| `SDA` | P0.22 / SPI MOSI | `022` |
| `CS` | P0.24 | `024` |
| `DC` | P1.00 | `100` |
| `RES` | P0.11 | `011` |
| `BLK` | P1.04 | `104` |

| Touch signal | nRF52840 pin | Board silk |
| --- | --- | --- |
| `TP_SCL` | P0.29 / I2C SCL | `029` |
| `TP_SDA` | P0.31 / I2C SDA | `031` |
| `TP_RST` | P0.02 | `002` |
| `TP_INT` | P1.15 | `115` |

Use `VCC` (3.3 V), not `RAW`, for the first bring-up. All logic signals are
3.3 V. The firmware drives `BLK` high at startup. Later this output can move
to a PWM peripheral for brightness and power management.

## Debug power

J-LinkOB `VDD` is the SWD voltage-reference connection and must not be treated
as the power source for the watch board and display. During bench debugging:

- power the ProMicro nRF52840 from its own USB connector;
- keep J-LinkOB connected to `VDD`, `DIO`, `CLK`, and `GND`;
- keep the display powered from the board `VCC` and `GND`;
- all grounds must remain common.

The target may appear to execute when only the debugger is attached, but that
condition cannot supply the LCD backlight reliably and may involve unintended
back-powering through signal or reference connections.

## Bring-up order

1. Connect only `VIN`, `GND`, and `BLK` first. A powered module with `BLK`
   high should visibly illuminate.
2. Disconnect power and add the seven LCD connections.
3. Run the existing GC9A01 colour test.
4. Disconnect power and add the four touch connections.
5. Probe the I2C bus and confirm the controller identity before enabling the
   touch driver.
