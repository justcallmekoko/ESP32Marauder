# ESP32 Marauder — Arduino Library Dependencies

Install all of these via **Tools → Manage Libraries** in Arduino IDE unless noted otherwise.

## Must Install

| Library | Author | Search term | Notes |
|---|---|---|---|
| LinkedList | Ivan Seidel | `LinkedList` | |
| ArduinoJson | Benoit Blanchon | `ArduinoJson` | |
| NimBLE-Arduino | h2zero | `NimBLE-Arduino` | |
| AsyncTCP | ESP32Async / dvarrel | `AsyncTCP` | |
| ESP32Ping | marian-craciunescu | `ESP32Ping` or `Ping` | If not found in Library Manager, install via zip from github.com/marian-craciunescu/ESP32Ping |
| MicroNMEA | Steve Marple | `MicroNMEA` | |

## Already Installed (from OLED display setup)

| Library | Author |
|---|---|
| Adafruit GFX Library | Adafruit |
| Adafruit SH110X | Adafruit |

## Built into ESP32 Core (no install needed)

- WiFi
- DNSServer
- Update
- SoftwareSerial
- Wire, SPI, FS, SPIFFS

## Not needed for ESP32_OLED target

- TFT_eSPI — only used for HAS_SCREEN hardware (ILI9341 TFT displays)
- XPT2046_Touchscreen — only for CYD touch hardware
- lv_arduino (LVGL) — only for specific hardware targets

## Board Manager

- **esp32 by Espressif Systems** `2.0.17` (**not** 3.x — Marauder uses tcpip_adapter, esp_spiram_init, esp_event_send_internal which were removed in core 3.x)
  - URL: `https://dl.espressif.com/dl/package_esp32_index.json`
  - Board: **ESP32 Dev Module**

## Version-pinned Libraries

| Library | Required version | Reason |
|---|---|---|
| NimBLE-Arduino | `2.3.2` | 2.5.0 changes addData/getPayload/setAdvertisedDeviceCallbacks API; Marauder tested with 1.3.8 and 2.3.2 |
