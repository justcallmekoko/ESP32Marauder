// TFT_eSPI setup for ONX2432G028.
// Hardware reference: OpenNextion-SKU-ONX2432G028 ESP-IDF examples.

#define USER_SETUP_LOADED

#define ST7789_DRIVER
#define TFT_RGB_ORDER TFT_BGR

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TFT_INVERSION_OFF

// ONX2432G028: ST7789 is write-only SPI; LCD reset is driven through PCF8574 EXIO6.
#define TFT_MOSI 1
#define TFT_SCLK 5
#define TFT_CS   2
#define TFT_DC   3
#define TFT_RST  -1
#define TFT_BL   6

// ONX2432G028 uses I2C CST826 capacitive touch, not SPI touch.
#define TOUCH_CS -1

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY 2500000
