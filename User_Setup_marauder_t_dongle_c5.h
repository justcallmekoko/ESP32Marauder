// TFT_eSPI setup for the LilyGo T-Dongle C5 onboard 80x160 ST7735 display.
#define ST7735_DRIVER
#define TFT_WIDTH  80
#define TFT_HEIGHT 160
#define ST7735_GREENTAB160x80
#define TFT_RGB_ORDER TFT_BGR

#define TFT_MISO  7
#define TFT_MOSI  2
#define TFT_SCLK  6
#define TFT_CS   10
#define TFT_DC    3
#define TFT_RST   1
#define TFT_BL    0
#define TFT_BACKLIGHT_ON LOW

#define LOAD_GLCD
#define SPI_FREQUENCY 27000000
#define SPI_READ_FREQUENCY 16000000
