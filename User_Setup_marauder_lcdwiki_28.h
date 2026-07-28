//                            USER DEFINED SETTINGS
//   TFT_eSPI setup for the LCDWiki 2.8" ESP32-S3 Display (MARAUDER_LCDWIKI_28)
//   https://www.lcdwiki.com/2.8inch_ESP32-S3_Display
//   ESP32-S3-WROOM-1 N16R8, 2.8" 240x320 ILI9341V panel, FT6336 capacitive touch.
//
//   Arduino IDE board settings for this module:
//     Board:  ESP32S3 Dev Module
//     Flash Size: 16MB (128Mb)    PSRAM: OPI PSRAM
//     USB CDC On Boot: Enabled    USB Mode: Hardware CDC and JTAG

// ##################################################################################
// Section 1. Driver
// ##################################################################################

#define ILI9341_DRIVER

// Panel native resolution (portrait)
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// This panel uses BGR sub-pixel order. If red/blue look swapped, use TFT_RGB.
#define TFT_RGB_ORDER TFT_BGR

// Use the ESP32-S3 FSPI (SPI2) peripheral. This keeps the TFT off the SD card's
// bus (the Marauder SD code defaults to HSPI/SPI3); sharing a bus makes SD init
// clobber the display. It also avoids an S3 SPI-register crash in TFT_eSPI when
// the port is left at its default.
#define USE_FSPI_PORT

// If colours are inverted (whites show as black) uncomment one of these.
// #define TFT_INVERSION_ON
// #define TFT_INVERSION_OFF

// ##################################################################################
// Section 2. Pins (ESP32-S3)
// ##################################################################################

#define TFT_MISO 13
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS   10    // Chip select
#define TFT_DC   46    // Data/command
#define TFT_RST  -1    // Panel reset is tied to the module reset
#define TFT_BL   45    // Backlight (PWM brightness handled by the firmware)
#define TFT_BACKLIGHT_ON HIGH

#define TOUCH_CS -1    // No SPI touch chip; FT6336 is I2C (handled in firmware)

// ##################################################################################
// Section 3. Fonts
// ##################################################################################

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_GFXFF
#define SMOOTH_FONT

// ##################################################################################
// Section 4. SPI speed
// ##################################################################################

#define SPI_FREQUENCY       20000000
#define SPI_READ_FREQUENCY  16000000
