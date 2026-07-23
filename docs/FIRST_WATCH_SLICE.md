# First working watch slice

Date: 2026-07-23

## Implemented

- ProMicro nRF52840 bare-metal startup and SWD debugging;
- GC9A01 240 x 240 round RGB565 display;
- CST816D capacitive touch controller at I2C address `0x15`;
- controllable LCD backlight;
- RTC1 eight-Hz animation time base using the internal 32.768 kHz RC clock,
  with civil time derived from groups of eight ticks;
- initial local time captured by the host build script;
- framebuffer-free NOG_C streaming backend;
- first target prototype of the `home-context` main window;
- built-in diagnostic digital watch face;
- incremental seconds update;
- StateSmith-generated GUI state machine;
- touch-triggered transition between the main and diagnostic screens;
- J-Link RAM diagnostics for clock, touch, GUI, and bus errors.

The firmware image currently uses 9,820 bytes of Flash and 1,104 bytes of
static RAM. No full-screen framebuffer or dynamic allocation is used.

The watch selects NOG_C's compact 32-bit line-clipping and freestanding memory
profile. Its coordinate limit is derived at compile time as four times the
largest configured display dimension. The build rejects accidental
reintroduction of 64-bit division, `memcpy`, `memmove`, or NOG_C libc-memory
dependencies.

## Layer boundary

```text
touch -> semantic event -> StateSmith GUI machine
                              |
                              v
RTC1 -> watch time model -> watch-owned target screens
                                      |
                                      v
                                    NOG_C
                                      |
                                      v
                              GC9A01 backend
```

These screens belong to this firmware. They are not part of NOG_C, which
remains a generic graphics engine. The new main window is a target prototype,
not yet a NOGGUI/Universal UI package: the independent UI repository still
owns the reusable semantic model, layout, profiles, skins, and package format.

## Current interaction

- `HOME_CONTEXT` starts after reset;
- its header shows reserved offline indicators for phone (`PH`), Technosense
  (`TS`), and detector (`DET`) connections;
- its context panel reports only the real local context (`LOCAL`) and does not
  invent connection state or phone data;
- large `HH:MM` digital time;
- a seconds progress bar;
- a tap is temporarily translated to `ACTIVATE`;
- `ACTIVATE` switches between `HOME_CONTEXT` and `DIAGNOSTIC_FACE`;
- seconds and seven-segment digits are updated differentially without clearing
  their complete bounds.
- the seconds bar receives an eight-step subsecond phase and advances by one
  pixel roughly every 0.4 seconds instead of jumping 2-3 pixels once a second.

See `docs/GUI_STATE_MACHINE.md` before adding screens or navigation.
See `docs/RENDERING_POLICY.md` for the no-flicker widget repaint contract.

## Known limitations

- the internal RC low-frequency clock is not sufficiently accurate for a
  finished watch;
- time is initialized at build time and is lost after a reset;
- no date, timezone, daylight-saving, phone synchronization, or settings UI;
- display writes run at 8 MHz, the maximum supported by the currently selected
  nRF52840 SPIM0 instance; `WATCH_LCD_SPI_MHZ` can select 1, 2, 4, or 8 MHz;
- NOG_C solid spans are mapped to one-row GC9A01 transactions;
- touch is polled rather than interrupt-driven;
- backlight has on/off control but no PWM brightness;
- the main loop does not enter low-power sleep;
- there is no BLE stack, bootloader, OTA, battery measurement, or storage.

## Next engineering gates

1. Confirm touch orientation and debounce behavior on the physical assembly.
2. Add a 240 x 240 round Display Profile to the independent NOGGUI project.
3. Measure the external 32.768 kHz crystal and switch RTC1 to LFXO when valid.
4. Add a solid-rectangle display transaction and evaluate SPIM3 at 16/32 MHz
   after signal-integrity tests.
5. Add interrupt-driven touch, display timeout, PWM dimming, and system sleep.
6. Select the BLE/platform baseline and synchronize time from a phone.
7. Reproduce the main-window prototype through the independent NOGGUI
   reference renderer and embedded adapter before promoting it to a reusable
   UI package, without moving watch-specific code into NOG_C.
