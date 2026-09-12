// TFT_eSPI setup for the Waveshare ESP32-C5-WIFI6-KIT with the
// Waveshare 3.5inch Capacitive Touch LCD (ST7796S + FT6336U).

// ##################################################################################
// Section 1. Driver
// ##################################################################################

#define ST7796_DRIVER

#define TFT_WIDTH  320
#define TFT_HEIGHT 480

#define TFT_INVERSION_ON

// ##################################################################################
// Section 2. Pin assignments
// ##################################################################################

#define TFT_MISO  4
#define TFT_MOSI  24
#define TFT_SCLK  23
#define TFT_CS    5
#define TFT_DC    3
#define TFT_RST   0
#define TFT_BL    1
// Backlight brightness is managed by Marauder's LEDC PWM code.  Defining
// TFT_BACKLIGHT_ON would make every tft.init() reconfigure this pin as a
// digital output and detach the PWM channel.

#define TOUCH_CS  -1  // FT6336U touch is connected over I2C.

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

#define SPI_FREQUENCY        27000000
#define SPI_READ_FREQUENCY   20000000
#define SPI_TOUCH_FREQUENCY   2500000
