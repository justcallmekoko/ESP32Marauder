# Seeed XIAO ESP32-S3 Plus 1.47-inch IPS display (initial port)

Target: `XIAO_ESP32_S3_TOUCH`. This is the ESP32-S3 Plus, 16 MB flash / 8 MB
OPI PSRAM, with the 172 x 320 JD9853A panel and AXS5106L touch hardware.
The target name describes the board; **touch input is currently disabled**.
Use BTN_A to move down through menus and BTN_B to select. Navigation wraps;
holding BTN_A does not repeatedly scroll. Serial CLI remains available.

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
so no patch to the installed TFT_eSPI rotation table is needed.

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
for a serial host at startup, permitting standalone power.

Native regression checks:

```sh
platformio test -e native
platformio test -e native_xiao
python -m unittest discover -s tools -p 'test_*.py'
```

## Limitations and validation still needed

- The saved bring-up reported phantom touch events that could dim the screen.
  This port explicitly disables touch, including calibration and touch reads.
  Touch keyboards and other touch-only interactions are unavailable.
- SD and direct upload remain disabled: the saved port reported a shared-SPI
  handoff problem. Battery monitoring is not implemented.
- The rebased source requires physical verification of display colors and
  alignment, both buttons, standalone boot, and Wi-Fi/BLE scanning. Existing
  binaries from the original port do not validate this revision.
- This initial port is not added to the release-build matrix until hardware
  validation is complete.
