#pragma once

#define USER_SETUP_INFO "User_Setup_waveshare_esp32c6_lcd_147"

#define ST7789_DRIVER

#define TFT_WIDTH 172
#define TFT_HEIGHT 320

#define CGRAM_OFFSET

#define TFT_RGB_ORDER TFT_RGB

#define TFT_MISO 5
#define TFT_MOSI 6
#define TFT_SCLK 7

#define TFT_CS 14
#define TFT_DC 15
#define TFT_RST 21

#define TFT_BL 22
#define TFT_BACKLIGHT_ON HIGH

#define TOUCH_CS -1

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500000

