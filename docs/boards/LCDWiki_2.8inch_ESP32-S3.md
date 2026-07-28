# ESP32 Marauder — LCDWiki 2.8" ESP32‑S3 Display

Support for the **LCDWiki 2.8‑inch ESP32‑S3 Display** module.

- Product / wiki: <https://www.lcdwiki.com/2.8inch_ESP32-S3_Display>
- Build target: `MARAUDER_LCDWIKI_28` (in `esp32_marauder/configs.h`)
- TFT_eSPI setup: `User_Setup_marauder_lcdwiki_28.h`

## Hardware

| | |
|---|---|
| MCU | ESP32‑S3‑WROOM‑1 **N16R8** (Xtensa LX7 dual‑core, 240 MHz) |
| Flash / PSRAM | 16 MB flash + 8 MB **OPI** PSRAM |
| Display | 2.8" IPS **240×320**, driver **ILI9341V**, 4‑wire SPI |
| Touch | **FT6336** capacitive, I²C (addr `0x38`) |
| microSD | **SDIO‑wired** slot (used here in SPI mode — see notes) |
| RGB LED | 1× **WS2812** addressable |
| USB | native USB‑C (ESP32‑S3 USB, appears as a CDC serial port) |
| Radios | Wi‑Fi 2.4 GHz b/g/n, BT 5 (LE) |

## Pinout

### Display — ILI9341V (SPI, FSPI/SPI2)
| Signal | GPIO |
|---|---|
| SCLK | **12** |
| MOSI | **11** |
| MISO | **13** |
| CS | **10** |
| DC / RS | **46** |
| RST | tied to module reset (`-1`) |
| Backlight (BL) | **45** (PWM brightness) |

Colour order **BGR**, portrait native (240×320), `SCREEN_ORIENTATION 0`.

### Touch — FT6336 (I²C `0x38`)
| Signal | GPIO |
|---|---|
| SDA | **16** |
| SCL | **15** |
| RST | **18** |
| INT | 17 *(not used by firmware)* |

### microSD — SDIO slot, driven over SPI
| SPI role | SDIO line | GPIO |
|---|---|---|
| SCK | CLK | **38** |
| MOSI | CMD | **40** |
| MISO | DAT0 | **39** |
| CS | DAT3 | **47** |
| — | DAT1 | 41 *(held high in SPI mode)* |
| — | DAT2 | 48 *(held high in SPI mode)* |

> The card **must be formatted FAT32** (the Arduino `SD` library can’t mount exFAT / >32 GB default formats).

### Other on‑board
| Function | GPIO |
|---|---|
| RGB LED (WS2812) | **42** (firmware turns it off) |
| BOOT button (used as center/select) | **0** |
| RESET | chip EN |
| USB D+ / D‑ | 20 / 19 |

## Broken‑out / available pins

Everything the module brings out to a header, and whether it's free with this firmware:

| Connector | GPIO(s) | Status with `MARAUDER_LCDWIKI_28` |
|---|---|---|
| **Expansion header** | **IO2, IO3, IO14, IO21** | **Free** — general‑purpose, use for anything |
| **I²C header** | **IO16 (SDA) / IO15 (SCL)** | Usable, but it's the **same bus as the FT6336 touch** (`0x38`) — add‑ons must use a different address |
| **UART header** | **IO44 (TX) / IO43 (RX)** | Assigned to **GPS** here (`GPS_TX=44`, `GPS_RX=43`); free if you don't attach/use GPS |

So, with the current build, the immediately free pins are **IO2, IO3, IO14, IO21** (plus the
I²C and UART headers if you don't need touch add‑ons / GPS).

> Any GPIO not listed here or in the pinout tables above is taken by flash/PSRAM (26–37), USB
> (19/20), or an on‑board peripheral — treat it as unavailable.

## Building (Arduino IDE)

1. In `esp32_marauder/configs.h`, uncomment the board target:
   ```c
   #define MARAUDER_LCDWIKI_28
   ```
   (make sure every other `MARAUDER_*` target is commented out).
2. Copy `User_Setup_marauder_lcdwiki_28.h` into your **TFT_eSPI** library folder and
   enable it in `User_Setup_Select.h` (uncomment its `#include`, comment the others).
3. Arduino IDE **Tools** settings:
   - **Board:** ESP32S3 Dev Module
   - **Flash Size:** 16MB (128Mb)
   - **PSRAM:** OPI PSRAM
   - **USB CDC On Boot:** Enabled
   - **USB Mode:** Hardware CDC and JTAG
4. Build & flash. If flashing fails, hold **BOOT**, tap **RESET**, release **BOOT** to enter download mode.

## Notes & gotchas

- **Powers up on a charger/powerbank too.** With USB‑CDC as the console the firmware no
  longer blocks boot waiting for a serial host, so the board runs untethered.
- **Backlight is PWM** on IO45 (Device → *Brightness*, or hold the top/bottom edge). The
  firmware owns IO45 via `ledcAttach`; don’t drive it with `digitalWrite`.
- **SPI bus split:** the TFT is pinned to **FSPI/SPI2** so it doesn’t share a bus with the
  SD card (which uses HSPI/SPI3). Sharing a bus blanks the display when the SD initialises.
- **SD needs FAT32** (see above).

### Re‑calibrating the touch panel

The FT6336’s raw range is baked in via `LCDWIKI_TCAL_X0/X1/Y0/Y1` in `configs.h`. To
re‑measure for your unit, build once with `-DTOUCH_CAL` (or add `#define TOUCH_CAL`),
flash, and follow the on‑screen 2‑dot calibration; it prints the new constants over serial:

```
[TOUCHCAL] -DLCDWIKI_TCAL_X0=… -DLCDWIKI_TCAL_X1=… -DLCDWIKI_TCAL_Y0=… -DLCDWIKI_TCAL_Y1=…
```

Paste those values into `configs.h` and rebuild without `TOUCH_CAL`.
