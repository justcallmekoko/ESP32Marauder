# ESP32 Marauder v1.13.0: ESP32-3248S035C GT911 support

This is a community modification of `justcallmekoko/ESP32Marauder` tag `v1.13.0` for the 3.5-inch ESP32-3248S035C board.

## Hardware target

- ESP32-D0WD-V3, 4 MB flash
- ST7796 display, 320 x 480
- GT911 capacitive touch
- GT911 SDA: GPIO33
- GT911 SCL: GPIO32
- GT911 INT: GPIO21
- GT911 RST is documented as GPIO25; the tested code does not actively toggle it

## Code changes

`esp32_marauder/configs.h`

- adds `HAS_GT911_TOUCH` to the official `MARAUDER_CYD_3_5_INCH` target;
- defines the GT911 I2C and interrupt pins.

`esp32_marauder/Display.cpp`

- adds SensorLib's `TouchDrvGT911` driver;
- probes GT911 at I2C addresses `0x5D` and `0x14`;
- maps native 320 x 480 coordinates for display rotations 0-3;
- bypasses TFT_eSPI resistive-touch calibration;
- prints a clear serial success or failure message;
- adds a 100 ms release debounce.

## Why the debounce is required

SensorLib's GT911 `getPoint()` clears the controller data-ready frame. A rapid poll immediately afterward may return zero even while the finger is still on the screen. Marauder can interpret that zero as a release and process the same physical tap twice. The final build retains the last valid point for 100 ms, making one physical tap produce one menu action.

## TFT_eSPI build configuration

- TFT_eSPI 2.5.34
- `User_Setup_cyd_3_5_inch.h`
- ST7796 driver
- 320 x 480 resolution
- resistive `TOUCH_CS 33` disabled because GPIO33 is GT911 SDA
- complete upstream `User_Setup_Select.h` retained; only the selected setup include is changed

## Tested build

- Marauder version: v1.13.0
- ESP32 Arduino core: 2.0.11
- FQBN: `esp32:esp32:d32:PartitionScheme=min_spiffs`
- SensorLib: 0.2.2
- final sketch size: 1,360,937 bytes
- final global memory: 76,892 bytes
- full flash and application-only update tested successfully

This is not an official ESP32Marauder release. Preserve the upstream and third-party license notices when redistributing it.
