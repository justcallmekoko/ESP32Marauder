// Marauder board: Waveshare ESP32-C5-Touch-LCD-3.5 (ST7796, 320x480).
// Pin map verified against Waveshare's own example repo (07_lvgl_demo,
// 03_SD_Card) and the board schematic. LCD reset and backlight are driven by
// the on-board I/O expander (0x24), not a GPIO, so TFT_RST/TFT_BL are unused
// here; see ws_c5_exio.h. FT6336 touch is I2C-only, so TOUCH_CS is unused.
#define ST7796_DRIVER
#define TFT_WIDTH   320
#define TFT_HEIGHT  480
#define TFT_INVERSION_ON

#define TFT_MISO  2
#define TFT_MOSI  7
#define TFT_SCLK  6
#define TFT_CS    8
#define TFT_DC    5
#define TFT_RST  -1    // LCD reset is on the I/O expander (EXIO 1)
#define TFT_BL   -1    // Backlight is on the I/O expander PWM register
#define TOUCH_CS -1    // FT6336 is I2C-only; no SPI touch chip

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_GFXFF
#define SMOOTH_FONT

// Conservative clocks for first bring-up; Waveshare runs the panel at
// 60 MHz and can be raised once verified on-device.
#define SPI_FREQUENCY        60000000
#define SPI_READ_FREQUENCY   16000000
