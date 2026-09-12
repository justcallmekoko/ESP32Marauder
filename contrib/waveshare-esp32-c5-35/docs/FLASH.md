# Flashing

## Safety first

Identify the ESP32-C5 serial port before writing. Do not guess between unrelated COM ports. The CH343 interface can reset the board when a monitor asserts DTR or RTS; use 115200 baud and do not open a serial monitor during an update.

Verify every selected file against `firmware/SHA256SUMS.txt`.

## OTA-safe USB application update

When the supplied `default_8MB` partition layout is already installed, write the OTA selector and application together:

```powershell
esptool --chip esp32c5 --port COMx --baud 115200 --before default-reset --after hard-reset --no-stub write-flash --flash-mode keep --flash-freq keep --flash-size keep 0xe000 ./contrib/waveshare-esp32-c5-35/firmware/boot_app0.bin 0x10000 ./contrib/waveshare-esp32-c5-35/firmware/esp32_marauder_waveshare_c5_35_app.bin
```

This selects OTA 0 and updates only OTA metadata plus the OTA 0 application. NVS, SPIFFS, coredump, SD contents, and OTA 1 are not addressed. Do not write only the application at `0x10000`: after an SD update the boot metadata may still select OTA 1, making the USB update appear ineffective.

The included helper performs this OTA-safe update by default:

```powershell
./contrib/waveshare-esp32-c5-35/tools/flash-waveshare-c5.ps1 -Port COMx
```

## Complete image for a new or unknown flash layout

`esp32_marauder_waveshare_c5_35_merged.bin` is the complete 8 MiB image. This overwrites the complete internal flash, including NVS, SPIFFS, coredump, and both OTA slots. It does not erase the separate microSD card:

```powershell
esptool --chip esp32c5 --port COMx --baud 115200 --before default-reset --after hard-reset --no-stub write-flash 0x0 ./contrib/waveshare-esp32-c5-35/firmware/esp32_marauder_waveshare_c5_35_merged.bin
```

The helper requires the explicit `-FullImage` switch for this destructive mode:

```powershell
./contrib/waveshare-esp32-c5-35/tools/flash-waveshare-c5.ps1 -Port COMx -FullImage
```

Replace `COMx` only after verifying the actual device port. If your esptool release uses underscore command names, use `write_flash` instead of `write-flash`.

## SD update and firmware-slot selector

Upload `esp32_marauder_waveshare_c5_35_app.bin` through the SD File Manager, stop the file-manager mode, and select it under `Device -> Update Firmware`. The updater writes the inactive OTA slot and restarts into it.

`Device -> Firmware Slot` then offers only the other slot. Selecting it opens the masked prompt `Enter dev passwd`. The developer confirmation password is `11111111` (exactly eight digits `1`). It prevents accidental slot changes; it is documented here rather than displayed on the device and is not a cryptographic secret.

Before changing the boot selection, the firmware verifies the target image structure, checksum, and appended SHA-256 digest. A successful switch shows a high-contrast status and performs a controlled restart.

## Image identity

- merged image: 8,388,608 bytes, SHA-256 `b925ad7e7c6bda9072b3b006b58db3d80af1d5929011148a9a9ac2ac29e66cbd`
- application image: 2,109,872 bytes, SHA-256 `64307554007412dd8e4d480e27310024909879ed1697107765d493dd907eaac2`
- OTA selector: 8,192 bytes, SHA-256 `f94c5d786a7a8fab06ac5d10e33bf37711a6697636dc037559ea19cc410a17f0`
- bootloader: 20,800 bytes, SHA-256 `5e19f4d759045cfc38446c70d0dacbd59bdc3e71208fd46b1a24375bc289fa60`
- partitions: 3,072 bytes, SHA-256 `1d9cca96de0fe07ad7fc0648b9878ddecd9ce565e38b589ad20fea698ed4c80c`
