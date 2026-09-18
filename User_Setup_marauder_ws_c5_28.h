#pragma once

#define USER_SETUP_INFO "User_Setup_marauder_ws_c5_28"

#define ST7789_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// #define CGRAM_OFFSET
#define TFT_RGB_ORDER TFT_BGR

#define I2C_SDA 0
#define I2C_SCL 1

// SPI pins 
#define TFT_SCLK   6    // BSP_LCD_PCLK
#define TFT_MOSI   7    // BSP_LCD_MOSI
#define TFT_MISO   8   // NC
#define TFT_DC     9    // BSP_LCD_DC
#define TFT_CS    10    // BSP_LCD_CS
#define TFT_RST   -1    // Via CH32V003 EXIO1 handled in setup()
#define TFT_BL    -1    // Via CH32V003 PWM handled in setup()

#define TP_SDA 0
#define TP_SCL 1
#define TP_RST -1   // Via CH32V003 EXIO0
#define TP_INT 5

#define SD_CLK 6
#define SD_MOSI 7
#define SD_MISO 8
#define SD_CS 23

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

#define SPI_FREQUENCY      40000000
#define SPI_READ_FREQUENCY 20000000
