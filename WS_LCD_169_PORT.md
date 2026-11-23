# ESP32 Marauder — Waveshare ESP32-S3-Touch-LCD-1.69 (self-serve build)

The port is **written**. These steps apply it on your computer and build/flash.
Pinned to Marauder commit `511bb3694d0c4dc064888e762bde44fcc9b7f48a`.

## Board facts
| | |
|---|---|
| MCU | ESP32-S3, 16MB flash, OPI PSRAM, **native USB** |
| Display | ST7789V2, 240×280, SPI — SCK=6 MOSI=7 DC=4 CS=5 RST=8 BL=15 |
| Touch | CST816T (I2C 0x15) — SDA=11 SCL=10 RST=13 INT=14 |

## What the port does
A new target `MARAUDER_WS_LCD_169` that **inherits the CYD-Micro full-screen
ST7789 config** and overrides only: panel 240×280, CST816T capacitive touch
(replacing the CYD's resistive XPT2046), the touch pins, PSRAM on, and SD/battery/
LED off for first boot. Touch is a new driver (`cst816.h`) that drops into
Marauder's existing capacitive read path.

Bundle files: `ws169_configs_display.patch`, `cst816.h`,
`User_Setup_ws_lcd_169.h`, `MARAUDER_COMMIT.txt`.

## 1. Get the source + apply the port
```bash
git clone https://github.com/justcallmekoko/ESP32Marauder
cd ESP32Marauder
git checkout 511bb3694d0c4dc064888e762bde44fcc9b7f48a

# apply the two-file patch (configs.h + Display.h)
git apply /path/to/ws169_configs_display.patch

# add the two new files
cp /path/to/cst816.h               esp32_marauder/cst816.h
cp /path/to/User_Setup_ws_lcd_169.h .
```
Then **enable the board**: in `esp32_marauder/configs.h` uncomment
```c
#define MARAUDER_WS_LCD_169
```

## 2. Point TFT_eSPI at this display
Marauder uses an external **TFT_eSPI** library (install the version its wiki
lists). Then:
```bash
cp User_Setup_ws_lcd_169.h <Arduino/libraries/TFT_eSPI>/
```
In `TFT_eSPI/User_Setup_Select.h`, comment out any active setup include and add:
```c
#include <User_Setup_ws_lcd_169.h>
```

## 3. Toolchain
Use the **ESP32 Arduino core 2.0.x** (e.g. 2.0.17) — the CYD-Micro code paths this
port inherits target core 2.x, not 3.x.
```bash
arduino-cli core update-index
arduino-cli core install esp32:esp32@2.0.17
# install Marauder's required libraries per its wiki (TFT_eSPI fork, etc.)
```

## 4. Build
Native USB S3 — the serial-over-USB option (`CDCOnBoot=cdc`) matters for the CLI.
Verify option keys with `arduino-cli board details --fqbn esp32:esp32:esp32s3`.
```bash
arduino-cli compile \
  --fqbn 'esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,CDCOnBoot=cdc' \
  esp32_marauder
```
In Arduino IDE instead: Board = **ESP32S3 Dev Module**, PSRAM = **OPI PSRAM**,
Flash Size = **16MB**, USB CDC On Boot = **Enabled**.

## 5. Flash
If the board doesn't appear as a serial port: hold **BOOT**, tap **RESET** (or
replug USB), release BOOT → download mode.
```bash
arduino-cli upload -p /dev/ttyACM0 \
  --fqbn 'esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,CDCOnBoot=cdc' \
  esp32_marauder
```
(macOS port looks like `/dev/cu.usbmodemXXXX`; Windows like `COM5`.)

## 6. Expect to iterate — paste me what you see
This is a blind first pass. Likely follow-ups, in order:
1. **Compile errors** (missing library, a macro I misjudged) → paste the first
   error; I fix and reissue the patch.
2. **Blank/garbled screen** → colour order / inversion in
   `User_Setup_ws_lcd_169.h` (`TFT_RGB_ORDER`, `TFT_INVERSION_ON`), or the
   240×280 row offset.
3. **Touch off** → axis/rotation mapping, or the CST816 not ACKing on I2C
   (check the `[Touch] CST816 ID: 0x..` line on serial at 115200).
4. **Layout cramped/overflowing** → tune the 240×280 constants.

Serial monitor: **115200 baud**. Send me the boot log and a photo of the screen
and we'll converge fast.
