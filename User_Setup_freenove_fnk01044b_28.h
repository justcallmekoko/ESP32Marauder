//                            USER DEFINED SETTINGS
//   TFT_eSPI setup for Freenove / Freenove ESP32-S3 Display
//   FNK0104B / FNK01044B (2.8", ESP32-S3 N16R8 + ILI9341 SPI + FT6336U touch)
//
//   This file is self contained and is NOT referenced by the shared
//   User_Setup_Select.h of the repository. Select it at build time with one of:
//
//   * Arduino IDE / arduino-cli
//       copy this file into the TFT_eSPI library folder and make the library's
//       User_Setup_Select.h include it (the selector must exist: TFT_eSPI 2.5.34
//       always includes it). Minimal version of that file:
//           rm -f  <libraries>/TFT_eSPI/User_Setup_Select.h
//           cp User_Setup_freenove_fnk01044b_28.h <libraries>/TFT_eSPI/
//           printf '#include <User_Setup_freenove_fnk01044b_28.h>\n' \
//             > <libraries>/TFT_eSPI/User_Setup_Select.h
//       (this is what .tools/build.sh does locally: the repository's own
//       User_Setup_Select.h is never modified)
//
//   * or simply add the line
//           #include <User_Setup_freenove_fnk01044b_28.h>
//       to the User_Setup_Select.h of your TFT_eSPI installation.
//
//   Pin values must match the MARAUDER_FREENOVE_FNK0104B block of
//   esp32_marauder/configs.h.

// ##################################################################################
// Section 1. Driver
// ##################################################################################

#define ILI9341_DRIVER

// 2.8-inch portrait panel native resolution
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// Colour order/inversion for the FNK0104B/FNK01044B panel
#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_ON

// ##################################################################################
// Section 2. Pin assignments
// ##################################################################################

#define TFT_MISO  13
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_CS    10    // Chip select
#define TFT_DC    46    // Data/Command
#define TFT_RST   -1    // Reset tied to the board reset
#define TFT_BL    45    // Backlight (LCD_BL), active HIGH
#define TFT_BACKLIGHT_ON HIGH

#define TOUCH_CS  -1    // No resistive touch controller; FT6336U is I2C only

// ##################################################################################
// Section 3. Fonts (Marauder uses GFXFF/FreeFonts)
// ##################################################################################

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// ##################################################################################
// Section 4. SPI
// ##################################################################################

#define SPI_FREQUENCY  40000000
#define USE_HSPI_PORT
