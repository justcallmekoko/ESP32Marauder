# Marauder to Support NM-CYD-C5

More information can get from [RockBase-iot/NM-CYD-C5](https://github.com/RockBase-iot/NM-CYD-C5.git).

## TFT_eSPI

To support NM-CYD-C5 `ST7789_DRIVER`, change the `libraries\TFT_eSPI\User_Setup.h` to `User_Setup_nm_cyd_c5.h`.

To Support ESP32-C5 device, the TFT_eSPI.c/h file need to support `CONFIG_IDF_TARGET_ESP32C5`, can get from [RockBase-iot/NM-CYD-C5/Demos/Arduino](https://github.com/RockBase-iot/NM-CYD-C5/tree/main/Demos/Arduino/libraries/TFT_eSPI), and `Processors` add `TFT_eSPI_ESP32_C5.c/h` files.

## Where to flash the firmware?

If you just want to flash the firmware to test, you can find the firmware from [NMIoT Web Flasher](https://flash.nmiot.net).

Choose Project `marauder`, Device: `nm-cyd-c5`, version `v1.11.1`.



