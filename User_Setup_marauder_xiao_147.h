// =====================================================================
//  ESP32 Marauder  --  Seeed XIAO ESP32-S3 (Plus) 1.47" Touch Display
// =====================================================================
//
//  Panel   : 1.47" IPS 172x320, controller JD9853A (ST7789-compatible)
//  Touch   : AXS5106L capacitive (I2C 0x63) -- disabled pending validation
//  MCU     : XIAO ESP32-S3 Plus (ESP32-S3R8, 16MB flash / 8MB PSRAM)
//
//  JD9853A is command-compatible with the ST7789. Seeed's own Seeed_GFX2
//  driver initialises this glass with the ST7789 sequence and only changes:
//     * colour order  -> BGR
//     * inversion      -> OFF
//     * column offset  -> 34  (172 centred in the 240-wide controller GRAM)
//  These are expressed below with stock TFT_eSPI options. The mirror
//  is set in Xiao147Display::setRotation; no custom
//  driver files are required. The 34px offset is produced automatically by
//  TFT_eSPI's ST7789 rotation code for a 172-wide panel (CGRAM_OFFSET).
// =====================================================================

#define USER_SETUP_ID 1470

#define ST7789_DRIVER            // JD9853A is ST7789-compatible

// On ESP32-S3 with arduino-esp32 3.x (IDF5), the default SPI_PORT (FSPI==0)
// resolves to a null register base in TFT_eSPI 2.5.34's low-level driver and
// crashes on the first writecommand(). Forcing the FSPI port sets SPI_PORT=2
// (the correct SPI2 register base) while still using the SPI2 peripheral.
#define USE_FSPI_PORT

#define TFT_WIDTH  172
#define TFT_HEIGHT 320
#define CGRAM_OFFSET             // enable the 34px column offset for 172-wide glass

#define TFT_RGB_ORDER TFT_BGR    // Seeed drives this glass as BGR
#define TFT_INVERSION_OFF        // Seeed disables inversion (overrides ST7789 INVON)

// ---- SPI pins (XIAO ESP32-S3 Plus D-label -> GPIO) ----
#define TFT_MISO  8   // D9
#define TFT_MOSI  9   // D10
#define TFT_SCLK  7   // D8
#define TFT_CS    3   // D2  LCD_CS
#define TFT_DC    4   // D3  LCD_DC
// LCD reset on GPIO13/D17. TFT_eSPI performs a proper hardware reset here right
// before the init sequence, which the JD9853 needs (the TFT_RST=-1 software-reset
// path leaves the panel blank). GPIO13 is physically shared with the AXS5106L
// touch reset; the touch driver does NOT toggle it (tft.init owns it).
#define TFT_RST  13   // D17 LCD_RST (shared with touch reset; owned by TFT_eSPI)
#define TFT_BL   12   // D18 LCD backlight
#define TFT_BACKLIGHT_ON HIGH

// ---- Fonts required by ESP32 Marauder ----
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// ---- SPI clocks ----
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  16000000
#define SPI_TOUCH_FREQUENCY  2500000
