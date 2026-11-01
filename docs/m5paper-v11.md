# M5Paper v1.1 Marauder port

This target is for the original **M5Stack M5Paper v1.1 / K049-B**, with a classic
ESP32-D0WDQ6-V3 and IT8951 display controller. It is not a PaperS3 image.
The upstream base is ESP32Marauder v1.20.0, commit
`511bb3694d0c4dc064888e762bde44fcc9b7f48a`.

## Hardware contract

| Function | Configuration |
| --- | --- |
| ESP32 | 240 MHz, 16 MB flash, external PSRAM enabled |
| Display | IT8951, 960 × 540 native, 540 × 960 portrait UI |
| Display SPI | SCK 14, MOSI 12, MISO 13, CS 15, shared VSPI with SD |
| Display power / ready | GPIO23 enable; GPIO27 HRDY |
| SD | CS 4, shared Arduino SPI instance and transaction lock |
| Touch | GT911, SDA 21, SCL 22, INT 36; probe 0x5d then 0x14 |
| Navigation | GPIO37 up, GPIO39 down, GPIO38 center/select/power |
| Power | GPIO2 main power; GPIO5 external 5 V boost disabled |

GPIO36–39 are input-only and have no internal pull resistors. The board supplies
the button bias; this port does not drive the touch interrupt or attempt a
software reset through it. The vendor's older driver and current documentation
use different left/right names for the jog switch; the GPIO mapping above is
the firmware's explicit contract.

## Controls and presentation

- Tap the top third to move up, the bottom third to move down, and release a
  tap in the middle third to select. Holding up/down repeats.
- The jog switch provides optional up/down. Short center release selects;
  during supported activity views it exits back to the menu.
- Hold center for three seconds to request shutdown, then release it. On
  battery, hold center for approximately two seconds to power up. USB bypasses
  the hardware power switch, so shutdown also enters deep sleep with center wake.
- Menus use eight spacious rows, a small selection arrow, and stable text.
  Activity titles use a large font and wrap instead of shrinking.
- The display has no backlight. LCD backlight and resistive-touch calibration
  paths are disabled for this target.
- Short center release cancels the touch keyboard, including when touch is
  unavailable. New keyboard layouts become active only after a successful
  panel refresh and touch release, so a press on the old layout cannot type
  into a layout that is not visible yet.

## Standalone startup

The original M5Paper uses an external USB/UART bridge. Startup does not wait
for a serial monitor, host connection, or incoming serial byte. The first
`setup()` action drives the main power hold high and releases retained sleep
pins before serial, SD, display, or radio initialization.

To qualify a revision, disconnect USB entirely, shut down and restart from a
charged battery, then move the selection and open a menu with the jog control.
The e-paper panel retains its image with no power, so a visible menu by itself
is not evidence that the CPU booted. Repeat using USB power with the serial
monitor closed and record the exact firmware commit. These cold-start checks
remain pending for the rebased revision because the board was unavailable.

## Display implementation

`M5PaperDisplay` implements the Marauder GFX compatibility surface and uses the
vendor-maintained M5GFX IT8951 driver, pinned to commit
`65c816a38386062d2c3dc47d41237a9083acbc75`.
M5GFX is configured explicitly for this board rather than probing other boards.
Display writes use the vendor's 40 MHz setting. Reads use 4 MHz after an
intermittent zero VCOM read was observed at 20 MHz; startup verifies the factory
VCOM read before enabling the display. Panel VCOM and
waveform tables are left at the factory values.

Three monochrome buffers in PSRAM separate drawing, transmission, and the last
completed screen. Dirty-window comparison suppresses unchanged frames. A worker
task submits 32-row strips while touch and the main loop continue running.
Each SPI transfer releases the shared Arduino bus lock; the waveform wait does
not hold the SPI bus. Small updates use DU4, larger partial updates use DU, and
startup/manual cleaning/every 32 partial updates use GC16. Draw bursts coalesce
for 25 ms, with a 250 ms maximum coalescing interval and 100 ms minimum submission
spacing. These are scheduling bounds, not a claim of physical refresh speed.

The project applies `tools/m5paper_panel_patch.py` to its local, pinned M5GFX
dependency. The patch verifies source hashes, preserves upstream license
notices, latches HRDY timeouts, aborts failed image uploads, and exposes the
failure to the worker. A failed upload never becomes the last successful
frame. The job checks an eight-second budget between driver calls; a blocking
driver wait can exceed that budget by one bounded HRDY wait (about 4.1 seconds).
This is not a hard eight-second interrupt deadline. Unknown dependency edits
fail the build instead of applying the patch to an unreviewed driver version.

The GT911 is polled every 8 ms. Not-ready reports preserve an existing touch;
ready zero-contact reports release it. Invalid ready reports are acknowledged,
and persistent I2C failures release a touch after 250 ms.

## Build and install

Use Python 3.12 and `pioarduino==6.2.0`:

```sh
python -m pip install pioarduino==6.2.0
python -m platformio run -e m5paper_v11 -t factory
python -m platformio run -e m5paper_v11 -t upload --upload-port YOUR_PORT
```

The `factory` target builds the application and merges its boot images into
`firmware.factory.bin` at offset 0x0; a plain build produces `firmware.bin` only.
The environment pins Arduino-ESP32 3.3.4 / ESP-IDF 5.5.1, matching the upstream
classic ESP32 build matrix. Both this SDK and 3.3.12 exceeded the default IRAM
budget when PSRAM and Bluetooth were linked together. `tools/m5paper_link.py`
creates a board-local linker script equivalent to disabling
`CONFIG_SPIRAM_CACHE_LIBTIME_IN_IRAM`: calendar/time conversion functions use
flash. Interrupt code, radio code, memory primitives, and the hardware memory map
stay unchanged. The script verifies the pinned SDK's expected rules and never
modifies an installed SDK. This follows
[Espressif's documented libc placement option](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32/api-guides/performance/ram-usage.html#putting-c-library-in-flash).

The partition table provides two 4 MB app slots and SPIFFS in the remaining
space. First installation changes the partition layout; preserve a full 16 MB
flash backup before installing. Keep that backup private because it may contain
the previous firmware's settings. Application-only updates use offset 0x10000.
The classic ESP32 bootloader starts at 0x1000; do not use the Sticky/S3 bootloader.

## Local diagnostics

`tools/paper_serial.py` requires pyserial and an explicit serial port. It does
not initiate radio tests:

```sh
python tools/paper_serial.py status --port YOUR_PORT
python tools/paper_serial.py test --port YOUR_PORT
python tools/paper_serial.py tick --port YOUR_PORT
python tools/paper_serial.py dump --port YOUR_PORT --output paper-frame.pbm
python tools/paper_serial.py menu --port YOUR_PORT
```

Other supported modes are `clean`, `listen`, `preview recon`, `preview deauth`,
and `powertest`. The power test sleeps and wakes after five seconds when USB
keeps the board supplied; battery power removal prevents a timer wake.
PBM dumps show the software framebuffer and cannot prove physical orientation
or absence of e-paper ghosting.

## Validation scope

The seven host model tests cover every pixel in all rotations, input bounds,
three full-width navigation zones, packed-row padding, and dirty rectangles
that erase an old menu marker while drawing a new one, plus safe IT8951 transfer
windows at every panel row. Run with
`python -m platformio test -e m5paper_native` on a host with a C++ compiler.

The owner confirmed normal text orientation, touch alignment at both corner
targets, touch menu navigation, and jog up/down/short-center selection. Battery
shutdown/restart, activity-view exits, long-term ghosting, SD capture under
sustained load, and the full Marauder feature matrix still require device
validation. See `m5paper-v11-validation.md` for the evidence and remaining checks.

## Primary references

- [M5Stack M5Paper v1.1 hardware documentation](https://docs.m5stack.com/en/core/Paper_v1.1)
- [M5GFX driver source at the pinned revision](https://github.com/m5stack/M5GFX/tree/65c816a38386062d2c3dc47d41237a9083acbc75)
- [M5Stack's original M5EPD driver](https://github.com/m5stack/M5EPD)
- [ESP32Marauder upstream](https://github.com/justcallmekoko/ESP32Marauder)

M5GFX is MIT licensed, with its embedded LovyanGFX code retaining its own BSD
license notices. It is consumed as a pinned dependency, not relicensed here.
