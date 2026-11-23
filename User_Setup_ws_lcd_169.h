// TFT_eSPI setup for the Waveshare ESP32-S3-Touch-LCD-1.69
//   Display : ST7789V2, 240 x 280, SPI
//   Pins (from Waveshare board_config/pin_config.h):
//     SCK=6  MOSI(SDA)=7  DC=4  CS=5  RST=8  BL=15   (no MISO - single data line)
//   Touch  : CST816T on I2C (handled separately by cst816.h, NOT by TFT_eSPI)
//
// Copy this file into the TFT_eSPI library folder and enable it from
// User_Setup_Select.h (see WS_LCD_169_PORT.md).

#define USER_SETUP_ID 169

// ---- Driver ----
#define ST7789_DRIVER            // Full configuration ST7789

// 240 x 280 panel. TFT_eSPI's ST7789 driver applies the correct 20px row
// offset automatically when TFT_HEIGHT is 280.
#define TFT_WIDTH  240
#define TFT_HEIGHT 280

// If reds/blues look swapped on the real panel, switch this to TFT_RGB.
// (ST7789 panels are commonly BGR.) -- HARDWARE-TUNABLE
#define TFT_RGB_ORDER TFT_BGR

// Many 240x280 ST7789 panels need colour inversion ON. If the screen shows a
// photo-negative (white<->black), comment this out. -- HARDWARE-TUNABLE
#define TFT_INVERSION_ON

// The ST7789 has no MISO; let the library read via MOSI if ever needed.
#define TFT_SDA_READ

// ---- Pins (ESP32-S3) ----
#define TFT_MISO -1
#define TFT_MOSI 7
#define TFT_SCLK 6
#define TFT_CS    5
#define TFT_DC    4
#define TFT_RST   8
#define TFT_BL   15             // backlight control pin
#define TFT_BACKLIGHT_ON HIGH

// ---- Fonts (match the other Marauder setups) ----
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// ---- SPI ----
// ST7789 is happy at 40-80MHz. Start at 40MHz; raise to 80MHz once stable.
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
