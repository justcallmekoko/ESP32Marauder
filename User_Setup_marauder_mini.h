//                            USER DEFINED SETTINGS
//   TFT_eSPI setup for Marauder Mini (ESP32 + ST7735 2.0" 160x128)
//   Uses standard hardware SPI (VSPI) on ESP32


// ##################################################################################
//
// Section 1. Call up the right driver file and any options for it
//
// ##################################################################################

// Display type -  only define if RPi display
//#define RPI_DRIVER

// Only define one driver, the other ones must be commented out
//#define ILI9341_DRIVER // OG Marauder
#define ST7735_DRIVER    // 2.0" ST7735 160x128
//#define ST7789_DRIVER
//#define ILI9163_DRIVER
//#define S6D02A1_DRIVER
//#define RPI_ILI9486_DRIVER
//#define HX8357D_DRIVER
//#define ILI9481_DRIVER
//#define ILI9486_DRIVER
//#define ILI9488_DRIVER
//#define ST7789_2_DRIVER
//#define R61581_DRIVER
//#define RM68140_DRIVER
//#define ST7796_DRIVER

// Some displays support SPI reads via the MISO pin, other displays have a single
// bi-directional SDA pin and the library will try to read this via the MOSI line.
// To use the SDA line for reading data from the TFT uncomment the following line:

// #define TFT_SDA_READ      // This option is for ESP32 ONLY, tested with ST7789 display only

// For ST7735, ST7789 and ILI9163 ONLY, define the colour order
// Try ONE option at a time to find the correct colour order for your display
// If colours look wrong (red/blue swapped), toggle between TFT_RGB and TFT_BGR

  #define TFT_RGB_ORDER TFT_BGR  // Try TFT_RGB if colours are wrong (2.0" ST7735 often uses BGR)

// For M5Stack ESP32 module with integrated ILI9341 display ONLY, remove // in line below

// #define M5STACK

// For ST7735, ST7789 and ILI9163 ONLY, define the pixel width and height in portrait orientation
// 2.0" ST7735 display — native 128 cols × 160 rows (portrait)
// Rotation 1 or 3 (landscape) gives 160×128 effective display
#define TFT_WIDTH  128
#define TFT_HEIGHT 160

// For ST7735 ONLY, define the type of display — this selects the correct init sequence.
// Try the options below if the screen does not display correctly (wrong colours,
// mirror images, offset, or stray pixels at edges).
// Only ONE of these should be uncommented for ST7735 driver.
//
// OFFSETS at rotation = 3 (landscape 160×128, current MARAUDER_MINI setting):
//   Tab variant    |  horizontal (left→right)  |  vertical (top→down)
//   GREENTAB       |  1px (min)                |  2px (max)
//   GREENTAB2      |  2px (max)                |  1px
//   GREENTAB3      |  2px (max)                |  1px   ← try 1st, fixes left edge garbage
//   REDTAB         |  0px                      |  0px
//   BLACKTAB       |  0px                      |  0px
// If garbage on LEFT edge:  prefer GREENTAB2 / GREENTAB3 (larger horizontal offset)
// If garbage on TOP edge:   prefer GREENTAB (larger vertical offset)
// If still garbage:         try REDTAB or BLACKTAB (different init command sequence)

//#define ST7735_GREENTAB        // Larger VERTICAL offset 2px, fixes top garbage
// #define ST7735_GREENTAB2     // colstart=2, rowstart=1 (if display shifts right)
#define ST7735_GREENTAB3       // Larger HORIZONTAL offset 2px, fixes left garbage [TRY FIRST]
// #define ST7735_GREENTAB128   // For 128 x 128 display
// #define ST7735_REDTAB
// #define ST7735_BLACKTAB
// #define ST7735_REDTAB160x80  // For 160 x 80 display with 24 pixel offset
// #define ST7735_INITB

// If colours are inverted (white shows as black) then uncomment one of the next
// 2 lines — try both options, one should correct the inversion.
// Many cheap ST7735 modules need TFT_INVERSION_ON for correct colors

// #define TFT_INVERSION_ON
#define TFT_INVERSION_OFF

// If a backlight control signal is available then define the TFT_BL pin in Section 2
// below. The backlight will be turned ON when tft.begin() is called, but the library
// needs to know if the LEDs are ON with the pin HIGH or LOW. If the LEDs are to be
// driven with a PWM signal or turned OFF/ON then this must be handled by the user
// sketch. e.g. with digitalWrite(TFT_BL, LOW);

#define TFT_BACKLIGHT_ON HIGH  // HIGH or LOW are options

// ##################################################################################
//
// Section 2. Define the pins that are used to interface with the display here
//
// ##################################################################################

// We must use hardware SPI, a minimum of 3 GPIO pins is needed.
// Typical setup for ESP8266 NodeMCU ESP-12 is :
//
// Display SDO/MISO  to NodeMCU pin D6 (or leave disconnected if not reading TFT)
// Display LED       to NodeMCU pin VIN (or 5V, see below)
// Display SCK       to NodeMCU pin D5
// Display SDI/MOSI  to NodeMCU pin D7
// Display DC (RS/AO)to NodeMCU pin D3
// Display RESET     to NodeMCU pin D4 (or RST, see below)
// Display CS        to NodeMCU pin D8 (or GND, see below)
// Display GND       to NodeMCU pin GND (0V)
// Display VCC       to NodeMCU 5V or 3.3V
//
// The TFT RESET pin can be connected to the NodeMCU RST pin or 3.3V to free up a control pin
//
// The DC (Data Command) pin may be labeled AO or RS (Register Select)
//
// With some displays such as the ILI9341 the TFT CS pin can be connected to GND if no more
// SPI devices (e.g. an SD Card) are connected, in this case comment out the #define TFT_CS
// line below so it is NOT defined. Other displays such at the ST7735 require the TFT CS pin
// to be toggled during setup, so in these cases the TFT_CS line must be defined and connected.
//
// The NodeMCU D0 pin can be used for RST
//
//
// Note: only some versions of the NodeMCU provide the USB 5V on the VIN pin
// If 5V is not available at a pin you can use 3.3V but backlight brightness
// will be lower.


// ###### EDIT THE PIN NUMBERS IN THE LINES FOLLOWING TO SUIT YOUR ESP8266 SETUP ######

// For NodeMCU - use pin numbers in the form PIN_Dx where Dx is the NodeMCU pin designation
//#define TFT_CS   PIN_D8  // Chip select control pin D8
//#define TFT_DC   PIN_D3  // Data Command control pin
//#define TFT_RST  PIN_D4  // Reset pin (could connect to NodeMCU RST, see next line)
//#define TFT_RST  -1    // Set TFT_RST to -1 if the display RESET is connected to NodeMCU RST or 3.3V

//#define TFT_BL PIN_D1  // LED back-light (only for ST7789 with backlight control pin)

//#define TOUCH_CS PIN_D2     // Chip select pin (T_CS) of touch screen

//#define TFT_WR PIN_D2       // Write strobe for modified Raspberry Pi TFT only


// ######  FOR ESP8266 OVERLAP MODE EDIT THE PIN NUMBERS IN THE FOLLOWING LINES  ######

// Overlap mode shares the ESP8266 FLASH SPI bus with the TFT so has a performance impact
// but saves pins for other functions. It is best not to connect MISO as some displays
// do not tristate that line wjen chip select is high!
// On NodeMCU 1.0 SD0=MISO, SD1=MOSI, CLK=SCLK to connect to TFT in overlap mode
// On NodeMCU V3  S0 =MISO, S1 =MOSI, S2 =SCLK
// In ESP8266 overlap mode the following must be defined

//#define TFT_SPI_OVERLAP

// In ESP8266 overlap mode the TFT chip select MUST connect to pin D3
//#define TFT_CS   PIN_D3
//#define TFT_DC   PIN_D5  // Data Command control pin
//#define TFT_RST  PIN_D4  // Reset pin (could connect to NodeMCU RST, see next line)
//#define TFT_RST  -1  // Set TFT_RST to -1 if the display RESET is connected to NodeMCU RST or 3.3V


// ###### EDIT THE PIN NUMBERS IN THE LINES FOLLOWING TO SUIT YOUR ESP32 SETUP   ######

// For ESP32 Dev board (only tested with ILI9341 display)
// The hardware SPI can be mapped to any pins

// XiaoMiao (小喵掌机) — 2.0" ST7735 160x128 (成品板引脚固定)
// TFT_RST = -1: 不使用MCU控制屏幕复位(ST7735自带POR上电复位)，
//               释放GPIO 19给SD卡MISO使用(SD_MISO=19)
#define TFT_CS   5  // Chip select control pin
#define TFT_DC   4  // Data Command control pin
#define TFT_RST  -1  // Set to -1 to skip MCU-controlled reset (GPIO 19 = SD_MISO)
#define TOUCH_CS -1
#define TFT_MISO -1  // ST7735 doesn't read back, leave disconnected
#define TFT_MOSI 23
#define TFT_SCLK 18
//#define TFT_BL   12  // LED back-light (directly connected to power)

/*
// ESP32 Marauder 
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   17  // Chip select control pin
#define TFT_DC   16  // Data Command control pin
#define TFT_RST   5  // Reset pin (could connect to RST pin)
//#define TFT_RST  -1  // Set TFT_RST to -1 if display RESET is connected to ESP32 board RST

#define TFT_BL   32  // LED back-light (only for ST7789 with backlight control pin)

#define TOUCH_CS 21     // Chip select pin (T_CS) of touch screen
*/
/////////////////////////////

// ESP32 Centauri
/*
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   27  // Chip select control pin
#define TFT_DC   26  // Data Command control pin
#define TFT_RST   5  // Reset pin (could connect to RST pin)
//#define TFT_RST  -1  // Set TFT_RST to -1 if display RESET is connected to ESP32 board RST

#define TFT_BL   32  // LED back-light (only for ST7789 with backlight control pin)

#define TOUCH_CS 21     // Chip select pin (T_CS) of touch screen
*/
/////////////////////////////

//#define TFT_WR 22    // Write strobe for modified Raspberry Pi TFT only

// For the M5Stack module use these #define lines
//#define TFT_MISO 19
//#define TFT_MOSI 23
//#define TFT_SCLK 18
//#define TFT_CS   14  // Chip select control pin
//#define TFT_DC   27  // Data Command control pin
//#define TFT_RST  33  // Reset pin (could connect to Arduino RESET pin)
//#define TFT_BL   32  // LED back-light (required for M5Stack)

// ######       EDIT THE PINs BELOW TO SUIT YOUR ESP32 PARALLEL TFT SETUP        ######

// The library supports 8 bit parallel TFTs with the ESP32, the pin
// selection below is compatible with ESP32 boards in UNO format.
// Wemos D32 boards need to be modified, see diagram in Tools folder.
// Only ILI9481 and ILI9341 based displays have been tested!

// Parallel bus is only supported on ESP32
// Uncomment line below to use ESP32 Parallel interface instead of SPI

//#define ESP32_PARALLEL

// The ESP32 and TFT the pins used for testing are:
//#define TFT_CS   33  // Chip select control pin (library pulls permanently low
//#define TFT_DC   15  // Data Command control pin - must use a pin in the range 0-31
//#define TFT_RST  32  // Reset pin, toggles on startup

//#define TFT_WR    4  // Write strobe control pin - must use a pin in the range 0-31
//#define TFT_RD    2  // Read strobe control pin

//#define TFT_D0   12  // Must use pins in the range 0-31 for the data bus
//#define TFT_D1   13  // so a single register write sets/clears all bits.
//#define TFT_D2   26  // Pins can be randomly assigned, this does not affect
//#define TFT_D3   25  // TFT screen update performance.
//#define TFT_D4   17
//#define TFT_D5   16
//#define TFT_D6   27
//#define TFT_D7   14


// ##################################################################################
//
// Section 3. Define the fonts that are to be used here
//
// ##################################################################################

// Comment out the #defines below with // to stop that font being loaded
// The ESP8366 and ESP32 have plenty of memory so commenting out fonts is not
// normally necessary. If all fonts are loaded the extra FLASH space required is
// about 17Kbytes. To save FLASH space only enable the fonts you need!

#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
//#define LOAD_FONT8N // Font 8. Alternative to Font 8 above, slightly narrower, so 3 digits fit a 160 pixel TFT
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

// Comment out the #define below to stop the SPIFFS filing system and smooth font code being loaded
// this will save ~20kbytes of FLASH
#define SMOOTH_FONT


// ##################################################################################
//
// Section 4. Other options
//
// ##################################################################################

// Define the SPI clock frequency, this affects the graphics rendering speed. Too
// fast and the TFT driver will not keep up and display corruption appears.
// With an ILI9341 display 40MHz works OK, 80MHz sometimes fails
// With a ST7735 display more than 27MHz may not work (spurious pixels and lines)
// With an ILI9163 display 27 MHz works OK.

// SPI frequency for ESP32 hardware SPI — ST7735 typically supports up to 27MHz
// 2.0" cheap modules often show stray pixels (left/top edge) above 10-12MHz due to
// weak PCB signal integrity. Start at 10MHz, then increase if display is clean.
// If left / top edge still shows garbage (random colored pixels) after trying all
// tab variants above: reduce SPI frequency by 2-4MHz and try again.
//#define SPI_FREQUENCY  27000000   // Absolute max (risky for 2.0" modules)
//#define SPI_FREQUENCY  20000000
//#define SPI_FREQUENCY  16000000
#define SPI_FREQUENCY  10000000      // ← start here (most stable for 2.0" cheap ST7735)

// Optional reduced SPI frequency for reading TFT
#define SPI_READ_FREQUENCY  10000000

// Touch controller SPI frequency (not used for this display)
#define SPI_TOUCH_FREQUENCY  2500000

// The ESP32 has 2 free SPI ports i.e. VSPI and HSPI, the VSPI is the default.
// Pin mapping for VSPI: SCK=18, MOSI=23, MISO=19 (matches our setup except TFT_RST=19)
// We use explicit pin definitions so no port conflict with RST pin on 19
// If the VSPI port is in use and pins are not accessible (e.g. TTGO T-Beam)
// then uncomment the following line:
//#define USE_HSPI_PORT

// Comment out the following #define if "SPI Transactions" do not need to be
// supported. When commented out the code size will be smaller and sketches will
// run slightly faster, so leave it commented out unless you need it!

// Transaction support is needed to work with SD library but not needed with TFT_SdFat
// Transaction support is required if other SPI devices are connected.

// Transactions are automatically enabled by the library for an ESP32 (to use HAL mutex)
// so changing it here has no effect

// #define SUPPORT_TRANSACTIONS
