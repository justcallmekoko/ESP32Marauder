# Seeed XIAO ESP32-S3 Plus 1.47-inch IPS display (initial port)

Target: `XIAO_ESP32_S3_TOUCH`. This is the ESP32-S3 Plus, 16 MB flash / 8 MB
OPI PSRAM, with the 172 x 320 JD9853A panel and AXS5106L touch hardware.
The target name describes the board; **touch input is currently disabled**.
Use BTN_A to move down through menus and BTN_B to select. Navigation wraps;
holding BTN_A does not repeatedly scroll. BTN_B stops an active scan and
returns to the menu, including Packet Monitor and analyzer screens. In those
graph screens BTN_A cycles channels/pages, wrapping after the last one.
The Brightness tool cycles levels with BTN_A and saves with BTN_B or after
three idle seconds. Serial CLI remains available.

## Pins

| Function | GPIO |
| --- | --- |
| Display SCK / MOSI / MISO | 7 / 9 / 8 |
| Display CS / DC / reset / backlight | 3 / 4 / 13 / 12 |
| BTN_A (down) / BTN_B (select), active low | 11 / 42 |
| Touch SDA / SCL / interrupt | 5 / 6 / 44 |
| SD CS (disabled) | 43 |

Display reset and touch reset share GPIO13. TFT_eSPI owns that reset.
The board-specific mirror command is applied by `Xiao147Display::setRotation`,
and reapplied after every `init`/`begin`, so no patch to the installed TFT_eSPI
rotation table is needed.
GPIO43 is driven high before LCD initialization so an inserted SD card remains
deselected while the display uses the shared SPI bus.

## Build

Use the project's Arduino dependencies, with Arduino-ESP32 **3.3.12**,
TFT_eSPI **2.5.34**, and NimBLE-Arduino **2.3.8**. Copy this repository's
`User_Setup_marauder_xiao_147.h` to TFT_eSPI's `User_Setup.h`, leaving the stock
`User_Setup_Select.h` selecting `User_Setup.h`. Do not combine board setups.
For a dedicated library folder named `libraries`, run from the repository root:

```sh
arduino-cli compile --fqbn 'esp32:esp32:XIAO_ESP32S3_Plus:PSRAM=opi,FlashSize=16M,PartitionScheme=default_8MB,CDCOnBoot=default' --libraries libraries --build-path build-xiao-147 --build-property 'compiler.cpp.extra_flags=-DXIAO_ESP32_S3_TOUCH' esp32_marauder
```

The selected TFT setup is checked at compile time. The firmware does not wait
for a serial host at startup. When USB CDC is enabled, its transmit timeout is
zero during initialization, so startup output that does not fit the transmit
buffer can be dropped. At the end of setup the native bounded timeout is
restored (250 ms for TinyUSB CDC, or 100 ms for Hardware USB Serial/JTAG), so
runtime CLI replies can wait for a connected host to drain its buffer. With the
documented TinyUSB configuration, writes return immediately when no serial
client is connected; restoring the timeout does not require a connection.

Native regression checks:

```sh
platformio test -e native
platformio test -e native_xiao
python -m unittest discover -s tools -p 'test_*.py'
```

## Limitations and validation still needed

- The saved bring-up reported phantom touch events that could dim the screen.
  This port explicitly disables touch, including calibration and touch reads.
  Touch keyboards and other touch-only interactions are unavailable. The
  `Add SSID`, `Join WiFi`, `Start AP`, and `Geofences` GUI entries are omitted
  because the target has no text editor or GPS. CLI configuration and saved
  Wi-Fi management remain available. Holding BTN_B does not enter the
  unavailable touch toggle.
- SD and direct upload remain disabled: the saved port reported a shared-SPI
  handoff problem. The inactive `Save/Load Files` menu is omitted. Battery
  monitoring is not implemented.
- The rebased source requires physical verification of display colors and
  alignment, both buttons, standalone boot, and Wi-Fi/BLE scanning. Existing
  binaries from the original port do not validate this revision.
- This initial port is not added to the release-build matrix until hardware
  validation is complete.

## Standalone startup qualification

The source uses upstream's unconditional nonblocking startup; it does not add
a `while (!Serial)` gate. The software review also checks the XIAO CDC path,
which has no serial-client connection requirement. This is source evidence,
not a substitute for a cold boot on the exact firmware image.

No XIAO board was available during the October 31, 2026 review. Before claiming
hardware qualification, flash the newly built image and record its commit and
SHA-256, then complete these checks:

1. Remove every power source for five seconds, release both buttons, and power
   the board from a battery or a USB supply with no data connection. Confirm
   the menu appears without opening a serial monitor.
2. Repeat with the SD slot empty and with a card inserted; the card stays
   unmounted, and the display must boot in both cases.
3. Navigate a menu longer than ten rows, confirm paging and wraparound, and
   confirm one action per physical press after contact bounce.
4. Open a passive scan, Packet Monitor, and an analyzer. Verify BTN_B exits each
   screen and restores the portrait menu with correct colors and alignment.
   Use BTN_A to cycle through all graph channels and activity pages, including
   wraparound from the last choice back to the first.
5. Change brightness with BTN_A, save with BTN_B, and verify it survives a cold
   boot. Repeated menu/display initialization must preserve the LCD mirror.
6. Attach a serial monitor, detach it, and reconnect it while the menu is
   running. Confirm physical navigation stays responsive throughout.
