// TFT_eSPI User_Setup for the LilyGo T-Embed CC1101 (ESP32-S3).
// ST7789 170x320 panel. Pinout: LilyGo T-Embed-CC1101 examples/utilities.h
// The 170-wide column offset (colstart=35) is applied automatically by
// TFT_eSPI 2.5.34 when TFT_WIDTH=170 and CGRAM_OFFSET are set.

#define USER_SETUP_ID 900

#define CGRAM_OFFSET
#define ST7789_2_DRIVER
#define TFT_RGB_ORDER TFT_RGB

#define TFT_WIDTH  170
#define TFT_HEIGHT 320

#define TFT_BACKLIGHT_ON HIGH

// T-Embed CC1101 pinout
#define TFT_MISO 10
#define TFT_MOSI 9
#define TFT_SCLK 11
#define TFT_CS   41
#define TFT_DC   16
#define TFT_RST  -1   // tied to EN, no dedicated pin
#define TFT_BL   21
#define TOUCH_CS -1

// Fonts
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// SPI: ESP32-S3, this panel handles 40-80 MHz
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  16000000
