// ST7789V 135 x 240 (TTGO T-Display V18)

#define ST7789_DRIVER

#define TFT_WIDTH  135
#define TFT_HEIGHT 240

#define CGRAM_OFFSET

#define TFT_RGB_ORDER TFT_RGB

#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   16
#define TFT_RST  -1

#define TFT_BL   4
#define TFT_BACKLIGHT_ON HIGH

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

#define SPI_FREQUENCY  40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

