#pragma once

#ifndef configs_h

  #define configs_h

  #define POLISH_POTATO

  //// BOARD TARGETS
  //#define MARAUDER_M5STICKC
  //#define MARAUDER_M5CARDPUTER
  #define MARAUDER_MINI
  //#define MARAUDER_V4
  //#define MARAUDER_V6
  //#define MARAUDER_V6_1
  //#define MARAUDER_KIT
  //#define GENERIC_ESP32
  //#define MARAUDER_FLIPPER
  //#define ESP32_LDDB
  //#define MARAUDER_DEV_BOARD_PRO
  //#define XIAO_ESP32_S3
  //#define MARAUDER_REV_FEATHER
  //// END BOARD TARGETS

  #define MARAUDER_VERSION "v1.1.0"

  //// HARDWARE NAMES
  #ifdef MARAUDER_M5STICKC
    #define HARDWARE_NAME "M5Stick-C Plus"
  #elif defined(MARAUDER_M5CARDPUTER)
    #define HARDWARE_NAME "M5Cardputer"
  #elif defined(MARAUDER_MINI)
    #define HARDWARE_NAME "Marauder Mini"
  #elif defined(MARAUDER_REV_FEATHER)
    #define HARDWARE_NAME "Adafruit Feather ESP32-S2 Reverse TFT"
  #elif defined(MARAUDER_V4)
    #define HARDWARE_NAME "Marauder v4"
  #elif defined(MARAUDER_V6)
    #define HARDWARE_NAME "Marauder v6"
  #elif defined(MARAUDER_V6_1)
    #define HARDWARE_NAME "Marauder v6.1"
  #elif defined(MARAUDER_KIT)
    #define HARDWARE_NAME "Marauder Kit"
  #elif defined(MARAUDER_FLIPPER)
    #define HARDWARE_NAME "Flipper Zero Dev Board"
  #elif defined(ESP32_LDDB)
    #define HARDWARE_NAME "ESP32 LDDB"
  #elif defined(MARAUDER_DEV_BOARD_PRO)
    #define HARDWARE_NAME "Flipper Zero Dev Board Pro"
  #elif defined(XIAO_ESP32_S3)
    #define HARDWARE_NAME "XIAO ESP32 S3"
  #else
    #define HARDWARE_NAME "ESP32"
  #endif

  //// END HARDWARE NAMES

 //// BOARD FEATURES
  #ifdef MARAUDER_M5STICKC
    //#define FLIPPER_ZERO_HAT
    #define HAS_BATTERY
    #define HAS_BT
    #define HAS_BUTTONS
    //#define HAS_NEOPIXEL_LED
    #define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_SD
    #define USE_SD
    #define HAS_TEMP_SENSOR
    //#define HAS_GPS
    //#define HAS_SOFTWARE_SERIAL
  #endif

  #ifdef MARAUDER_M5CARDPUTER
    // #define FLIPPER_ZERO_HAT
    // #define HAS_BATTERY
    #define HAS_BT
    #define HAS_BUTTONS
    // #define HAS_NEOPIXEL_LED
    // #define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_SD
    #define USE_SD
    #define HAS_TEMP_SENSOR
    #define HAS_GPS
  #endif

  #ifdef MARAUDER_MINI
    //#define FLIPPER_ZERO_HAT
    //#define HAS_BATTERY
    #define HAS_BT
    #define HAS_BUTTONS
    #define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_SD
    #define USE_SD
    #define HAS_TEMP_SENSOR
    #define HAS_GPS
  #endif

  #ifdef MARAUDER_REV_FEATHER
    //#define FLIPPER_ZERO_HAT
    //#define HAS_BATTERY
    //#define HAS_BT
    #define HAS_BUTTONS
    #define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_SD
    #define USE_SD
    #define HAS_TEMP_SENSOR
    #define HAS_GPS
  #endif

  #ifdef MARAUDER_V4
    //#define FLIPPER_ZERO_HAT
    #define HAS_BATTERY
    #define HAS_BT
    //#define HAS_BUTTONS
    #define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_SD
    #define USE_SD
    #define HAS_TEMP_SENSOR
    #define HAS_GPS
  #endif

  #if defined(MARAUDER_V6) || defined(MARAUDER_V6_1)
    //#define FLIPPER_ZERO_HAT
    #define HAS_BATTERY
    #define HAS_BT
    #define HAS_BT_REMOTE
    #define HAS_BUTTONS
    #define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_SD
    #define USE_SD
    #define HAS_TEMP_SENSOR
    #define HAS_GPS
  #endif

  #ifdef MARAUDER_KIT
    //#define FLIPPER_ZERO_HAT
    #define HAS_BATTERY
    #define HAS_BT
    //#define HAS_BUTTONS
    #define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_SD
    #define USE_SD
    #define HAS_TEMP_SENSOR
    #define HAS_GPS
  #endif

  #ifdef GENERIC_ESP32
    //#define FLIPPER_ZERO_HAT
    //#define HAS_BATTERY
    #define HAS_BT
    //#define HAS_BUTTONS
    //#define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    //#define HAS_SCREEN
    //#define HAS_SD
    //#define HAS_TEMP_SENSOR
    //#define HAS_GPS
  #endif

  #ifdef MARAUDER_FLIPPER
    //#define FLIPPER_ZERO_HAT
    //#define HAS_BATTERY
    //#define HAS_BT
    //#define HAS_BUTTONS
    //#define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    //#define HAS_SCREEN
    #define HAS_GPS
    #define HAS_SD
    #define USE_SD
    //#define HAS_TEMP_SENSOR
  #endif

  #ifdef ESP32_LDDB
    //#define FLIPPER_ZERO_HAT
    //#define HAS_BATTERY
    #define HAS_BT
    //#define HAS_BUTTONS
    #define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    //#define HAS_SCREEN
    #define HAS_SD
    #define USE_SD
    //#define HAS_TEMP_SENSOR
    //#define HAS_GPS
  #endif

  #ifdef MARAUDER_DEV_BOARD_PRO
    //#define FLIPPER_ZERO_HAT
    //#define HAS_BATTERY
    #define HAS_BT
    //#define HAS_BUTTONS
    #define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    //#define HAS_SCREEN
    #define HAS_SD
    #define USE_SD
    //#define HAS_TEMP_SENSOR
    #define HAS_GPS
  #endif

  #ifdef XIAO_ESP32_S3
    #define FLIPPER_ZERO_HAT
    //#define HAS_BATTERY
    #define HAS_BT
    //#define HAS_BUTTONS
    //#define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    //#define HAS_SCREEN
    //#define HAS_SD
    //#define HAS_TEMP_SENSOR
    //#define HAS_GPS
  #endif
  //// END BOARD FEATURES

  //// POWER MANAGEMENT
  #ifdef HAS_PWR_MGMT
    #ifdef MARAUDER_M5STICKC
      #include "AXP192.h"
    #endif
  #endif
  //// END POWER MANAGEMENT

  //// BUTTON DEFINITIONS
  #ifdef HAS_BUTTONS

    #ifdef MARAUDER_REV_FEATHER
      #define L_BTN -1
      #define C_BTN 1
      #define U_BTN 0
      #define R_BTN -1
      #define D_BTN 2

      //#define HAS_L
      //#define HAS_R
      #define HAS_U
      #define HAS_D
      #define HAS_C

      #define L_PULL true
      #define C_PULL false
      #define U_PULL true
      #define R_PULL true
      #define D_PULL false
    #endif

    #ifdef MARAUDER_MINI
      #define L_BTN 13
      #define C_BTN 34
      #define U_BTN 36
      #define R_BTN 39
      #define D_BTN 35

      #define HAS_L
      #define HAS_R
      #define HAS_U
      #define HAS_D
      #define HAS_C

      #define L_PULL true
      #define C_PULL true
      #define U_PULL true
      #define R_PULL true
      #define D_PULL true
    #endif

    #ifdef MARAUDER_M5STICKC
      #define L_BTN -1
      #define C_BTN 37
      #define U_BTN -1
      #define R_BTN -1
      #define D_BTN 39

      //#define HAS_L
      //#define HAS_R
      //#define HAS_U
      #define HAS_D
      #define HAS_C

      #define L_PULL true
      #define C_PULL true
      #define U_PULL true
      #define R_PULL true
      #define D_PULL true
    #endif

    #ifdef MARAUDER_M5CARDPUTER
      #define L_BTN -1
      #define C_BTN 0
      #define U_BTN -1
      #define R_BTN -1
      #define D_BTN -1

      //#define HAS_L
      //#define HAS_R
      //#define HAS_U
      //#define HAS_D
      #define HAS_C

      #define L_PULL true
      #define C_PULL true
      #define U_PULL true
      #define R_PULL true
      #define D_PULL true
    #endif

    #ifdef MARAUDER_V6
      #define L_BTN -1
      #define C_BTN 0
      #define U_BTN -1
      #define R_BTN -1
      #define D_BTN -1

      //#define HAS_L
      //#define HAS_R
      //#define HAS_U
      //#define HAS_D
      #define HAS_C

      #define L_PULL true
      #define C_PULL true
      #define U_PULL true
      #define R_PULL true
      #define D_PULL true
    #endif

    #ifdef MARAUDER_V6_1
      #define L_BTN -1
      #define C_BTN 0
      #define U_BTN -1
      #define R_BTN -1
      #define D_BTN -1

      //#define HAS_L
      //#define HAS_R
      //#define HAS_U
      //#define HAS_D
      #define HAS_C

      #define L_PULL true
      #define C_PULL true
      #define U_PULL true
      #define R_PULL true
      #define D_PULL true
    #endif  

  #endif
  //// END BUTTON DEFINITIONS

  //// DISPLAY DEFINITIONS
  #ifdef HAS_SCREEN

    #ifdef MARAUDER_M5STICKC
      #define SCREEN_CHAR_WIDTH 40
      //#define TFT_MISO 19
      #define TFT_MOSI 15
      #define TFT_SCLK 13
      #define TFT_CS 5
      #define TFT_DC 23
      #define TFT_RST 18
      #define TFT_BL -1
      #define TOUCH_CS -1
      //#define SD_CS 1

      #define SCREEN_BUFFER

      #define MAX_SCREEN_BUFFER 9

      #define BANNER_TEXT_SIZE 1

      #ifndef TFT_WIDTH
        #define TFT_WIDTH 135
      #endif

      #ifndef TFT_HEIGHT
        #define TFT_HEIGHT 240
      #endif

      #define CHAR_WIDTH 6
      #define SCREEN_WIDTH TFT_HEIGHT // Originally 240
      #define SCREEN_HEIGHT TFT_WIDTH // Originally 320
      #define HEIGHT_1 TFT_WIDTH
      #define WIDTH_1 TFT_WIDTH
      #define STANDARD_FONT_CHAR_LIMIT (TFT_WIDTH/6) // number of characters on a single line with normal font
      #define TEXT_HEIGHT (TFT_HEIGHT/10) // Height of text to be printed and scrolled
      #define BOT_FIXED_AREA 0 // Number of lines in bottom fixed area (lines counted from bottom of screen)
      #define TOP_FIXED_AREA 48 // Number of lines in top fixed area (lines counted from top of screen)
      #define YMAX TFT_HEIGHT // Bottom of screen area
      #define minimum(a,b)     (((a) < (b)) ? (a) : (b))
      //#define MENU_FONT NULL
      #define MENU_FONT &FreeMono9pt7b // Winner
      //#define MENU_FONT &FreeMonoBold9pt7b
      //#define MENU_FONT &FreeSans9pt7b
      //#define MENU_FONT &FreeSansBold9pt7b
      #define BUTTON_SCREEN_LIMIT 6
      #define BUTTON_ARRAY_LEN 100
      #define STATUS_BAR_WIDTH (TFT_HEIGHT/16)
      #define LVGL_TICK_PERIOD 6
    
      #define FRAME_X 100
      #define FRAME_Y 64
      #define FRAME_W 120
      #define FRAME_H 50
    
      // Red zone size
      #define REDBUTTON_X FRAME_X
      #define REDBUTTON_Y FRAME_Y
      #define REDBUTTON_W (FRAME_W/2)
      #define REDBUTTON_H FRAME_H
    
      // Green zone size
      #define GREENBUTTON_X (REDBUTTON_X + REDBUTTON_W)
      #define GREENBUTTON_Y FRAME_Y
      #define GREENBUTTON_W (FRAME_W/2)
      #define GREENBUTTON_H FRAME_H
    
      #define STATUSBAR_COLOR 0x4A49

    #endif

    #ifdef MARAUDER_M5CARDPUTER
      #define SCREEN_CHAR_WIDTH 40
      //#define TFT_MISO -1
      #define TFT_MOSI 35
      #define TFT_SCLK 36
      #define TFT_CS 37
      #define TFT_DC 34
      #define TFT_RST 33
      #define TFT_BL 38
      // #define TOUCH_CS -1

      #define SCREEN_BUFFER

      #define MAX_SCREEN_BUFFER 9

      #define BANNER_TEXT_SIZE 1

      #ifndef TFT_WIDTH
        #define TFT_WIDTH 135
      #endif

      #ifndef TFT_HEIGHT
        #define TFT_HEIGHT 240
      #endif

      #define CHAR_WIDTH 6
      #define SCREEN_WIDTH TFT_HEIGHT // Originally 240
      #define SCREEN_HEIGHT TFT_WIDTH // Originally 320
      #define HEIGHT_1 TFT_WIDTH
      #define WIDTH_1 TFT_WIDTH
      #define STANDARD_FONT_CHAR_LIMIT (TFT_WIDTH/6) // number of characters on a single line with normal font
      #define TEXT_HEIGHT (TFT_HEIGHT/10) // Height of text to be printed and scrolled
      #define BOT_FIXED_AREA 0 // Number of lines in bottom fixed area (lines counted from bottom of screen)
      #define TOP_FIXED_AREA 48 // Number of lines in top fixed area (lines counted from top of screen)
      #define YMAX TFT_HEIGHT // Bottom of screen area
      #define minimum(a,b)     (((a) < (b)) ? (a) : (b))
      //#define MENU_FONT NULL
      #define MENU_FONT &FreeMono9pt7b // Winner
      //#define MENU_FONT &FreeMonoBold9pt7b
      //#define MENU_FONT &FreeSans9pt7b
      //#define MENU_FONT &FreeSansBold9pt7b
      #define BUTTON_SCREEN_LIMIT 6
      #define BUTTON_ARRAY_LEN 100
      #define STATUS_BAR_WIDTH (TFT_HEIGHT/16)
      #define LVGL_TICK_PERIOD 6
    
      #define FRAME_X 100
      #define FRAME_Y 64
      #define FRAME_W 120
      #define FRAME_H 50
    
      // Red zone size
      #define REDBUTTON_X FRAME_X
      #define REDBUTTON_Y FRAME_Y
      #define REDBUTTON_W (FRAME_W/2)
      #define REDBUTTON_H FRAME_H
    
      // Green zone size
      #define GREENBUTTON_X (REDBUTTON_X + REDBUTTON_W)
      #define GREENBUTTON_Y FRAME_Y
      #define GREENBUTTON_W (FRAME_W/2)
      #define GREENBUTTON_H FRAME_H
    
      #define STATUSBAR_COLOR 0x4A49

    #endif

    #ifdef MARAUDER_M5CARDPUTER
      #define SCREEN_CHAR_WIDTH 40
      //#define TFT_MISO -1
      #define TFT_MOSI 35
      #define TFT_SCLK 36
      #define TFT_CS 37
      #define TFT_DC 34
      #define TFT_RST 33
      #define TFT_BL 38
      // #define TOUCH_CS -1

      #define SCREEN_BUFFER

      #define MAX_SCREEN_BUFFER 9

      #define BANNER_TEXT_SIZE 1

      #ifndef TFT_WIDTH
        #define TFT_WIDTH 135
      #endif

      #ifndef TFT_HEIGHT
        #define TFT_HEIGHT 240
      #endif

      #define CHAR_WIDTH 6
      #define SCREEN_WIDTH TFT_HEIGHT // Originally 240
      #define SCREEN_HEIGHT TFT_WIDTH // Originally 320
      #define HEIGHT_1 TFT_WIDTH
      #define WIDTH_1 TFT_WIDTH
      #define STANDARD_FONT_CHAR_LIMIT (TFT_WIDTH/6) // number of characters on a single line with normal font
      #define TEXT_HEIGHT (TFT_HEIGHT/10) // Height of text to be printed and scrolled
      #define BOT_FIXED_AREA 0 // Number of lines in bottom fixed area (lines counted from bottom of screen)
      #define TOP_FIXED_AREA 48 // Number of lines in top fixed area (lines counted from top of screen)
      #define YMAX TFT_HEIGHT // Bottom of screen area
      #define minimum(a,b)     (((a) < (b)) ? (a) : (b))
      //#define MENU_FONT NULL
      #define MENU_FONT &FreeMono9pt7b // Winner
      //#define MENU_FONT &FreeMonoBold9pt7b
      //#define MENU_FONT &FreeSans9pt7b
      //#define MENU_FONT &FreeSansBold9pt7b
      #define BUTTON_SCREEN_LIMIT 6
      #define BUTTON_ARRAY_LEN 13
      #define STATUS_BAR_WIDTH (TFT_HEIGHT/16)
      #define LVGL_TICK_PERIOD 6
    
      #define FRAME_X 100
      #define FRAME_Y 64
      #define FRAME_W 120
      #define FRAME_H 50
    
      // Red zone size
      #define REDBUTTON_X FRAME_X
      #define REDBUTTON_Y FRAME_Y
      #define REDBUTTON_W (FRAME_W/2)
      #define REDBUTTON_H FRAME_H
    
      // Green zone size
      #define GREENBUTTON_X (REDBUTTON_X + REDBUTTON_W)
      #define GREENBUTTON_Y FRAME_Y
      #define GREENBUTTON_W (FRAME_W/2)
      #define GREENBUTTON_H FRAME_H
    
      #define STATUSBAR_COLOR 0x4A49

    #endif

    #ifdef MARAUDER_V4
      #define SCREEN_CHAR_WIDTH 40
      #define HAS_ILI9341
      #define BANNER_TEXT_SIZE 2

      #ifndef TFT_WIDTH
        #define TFT_WIDTH 240
      #endif

      #ifndef TFT_HEIGHT
        #define TFT_HEIGHT 320
      #endif

      #define TFT_SHIELD
    
      #define SCREEN_WIDTH TFT_WIDTH
      #define SCREEN_HEIGHT TFT_HEIGHT
      #define HEIGHT_1 TFT_WIDTH
      #define WIDTH_1 TFT_HEIGHT
      #define STANDARD_FONT_CHAR_LIMIT (TFT_WIDTH/6) // number of characters on a single line with normal font
      #define TEXT_HEIGHT 16 // Height of text to be printed and scrolled
      #define BOT_FIXED_AREA 0 // Number of lines in bottom fixed area (lines counted from bottom of screen)
      #define TOP_FIXED_AREA 48 // Number of lines in top fixed area (lines counted from top of screen)
      #define YMAX 320 // Bottom of screen area
      #define minimum(a,b)     (((a) < (b)) ? (a) : (b))
      //#define MENU_FONT NULL
      #define MENU_FONT &FreeMono9pt7b // Winner
      //#define MENU_FONT &FreeMonoBold9pt7b
      //#define MENU_FONT &FreeSans9pt7b
      //#define MENU_FONT &FreeSansBold9pt7b
      #define BUTTON_SCREEN_LIMIT 12
      #define BUTTON_ARRAY_LEN 12
      #define STATUS_BAR_WIDTH 16
      #define LVGL_TICK_PERIOD 6
    
      #define FRAME_X 100
      #define FRAME_Y 64
      #define FRAME_W 120
      #define FRAME_H 50
    
      // Red zone size
      #define REDBUTTON_X FRAME_X
      #define REDBUTTON_Y FRAME_Y
      #define REDBUTTON_W (FRAME_W/2)
      #define REDBUTTON_H FRAME_H
    
      // Green zone size
      #define GREENBUTTON_X (REDBUTTON_X + REDBUTTON_W)
      #define GREENBUTTON_Y FRAME_Y
      #define GREENBUTTON_W (FRAME_W/2)
      #define GREENBUTTON_H FRAME_H
    
      #define STATUSBAR_COLOR 0x4A49
    
      #define KIT_LED_BUILTIN 13
    #endif

    #if defined(MARAUDER_V6) || defined(MARAUDER_V6_1)
      #define SCREEN_CHAR_WIDTH 40
      #define HAS_ILI9341
    
      #define BANNER_TEXT_SIZE 2

      #ifndef TFT_WIDTH
        #define TFT_WIDTH 240
      #endif

      #ifndef TFT_HEIGHT
        #define TFT_HEIGHT 320
      #endif

      #define TFT_DIY
    
      #define SCREEN_WIDTH TFT_WIDTH
      #define SCREEN_HEIGHT TFT_HEIGHT
      #define HEIGHT_1 TFT_WIDTH
      #define WIDTH_1 TFT_HEIGHT
      #define STANDARD_FONT_CHAR_LIMIT (TFT_WIDTH/6) // number of characters on a single line with normal font
      #define TEXT_HEIGHT 16 // Height of text to be printed and scrolled
      #define BOT_FIXED_AREA 0 // Number of lines in bottom fixed area (lines counted from bottom of screen)
      #define TOP_FIXED_AREA 48 // Number of lines in top fixed area (lines counted from top of screen)
      #define YMAX 320 // Bottom of screen area
      #define minimum(a,b)     (((a) < (b)) ? (a) : (b))
      //#define MENU_FONT NULL
      #define MENU_FONT &FreeMono9pt7b // Winner
      //#define MENU_FONT &FreeMonoBold9pt7b
      //#define MENU_FONT &FreeSans9pt7b
      //#define MENU_FONT &FreeSansBold9pt7b
      #define BUTTON_SCREEN_LIMIT 12
      #define BUTTON_ARRAY_LEN 12
      #define STATUS_BAR_WIDTH 16
      #define LVGL_TICK_PERIOD 6

      #define FRAME_X 100
      #define FRAME_Y 64
      #define FRAME_W 120
      #define FRAME_H 50
    
      // Red zone size
      #define REDBUTTON_X FRAME_X
      #define REDBUTTON_Y FRAME_Y
      #define REDBUTTON_W (FRAME_W/2)
      #define REDBUTTON_H FRAME_H
    
      // Green zone size
      #define GREENBUTTON_X (REDBUTTON_X + REDBUTTON_W)
      #define GREENBUTTON_Y FRAME_Y
      #define GREENBUTTON_W (FRAME_W/2)
      #define GREENBUTTON_H FRAME_H
    
      #define STATUSBAR_COLOR 0x4A49
    
      #define KIT_LED_BUILTIN 13
    #endif 

    #ifdef MARAUDER_KIT
      #define SCREEN_CHAR_WIDTH 40
      #define HAS_ILI9341
    
      #define BANNER_TEXT_SIZE 2

      #ifndef TFT_WIDTH
        #define TFT_WIDTH 240
      #endif

      #ifndef TFT_HEIGHT
        #define TFT_HEIGHT 320
      #endif

      #define TFT_DIY
      #define KIT
    
      #define SCREEN_WIDTH TFT_WIDTH
      #define SCREEN_HEIGHT TFT_HEIGHT
      #define HEIGHT_1 TFT_WIDTH
      #define WIDTH_1 TFT_HEIGHT
      #define STANDARD_FONT_CHAR_LIMIT (TFT_WIDTH/6) // number of characters on a single line with normal font
      #define TEXT_HEIGHT 16 // Height of text to be printed and scrolled
      #define BOT_FIXED_AREA 0 // Number of lines in bottom fixed area (lines counted from bottom of screen)
      #define TOP_FIXED_AREA 48 // Number of lines in top fixed area (lines counted from top of screen)
      #define YMAX 320 // Bottom of screen area
      #define minimum(a,b)     (((a) < (b)) ? (a) : (b))
      //#define MENU_FONT NULL
      #define MENU_FONT &FreeMono9pt7b // Winner
      //#define MENU_FONT &FreeMonoBold9pt7b
      //#define MENU_FONT &FreeSans9pt7b
      //#define MENU_FONT &FreeSansBold9pt7b
      #define BUTTON_SCREEN_LIMIT 12
      #define BUTTON_ARRAY_LEN 12
      #define STATUS_BAR_WIDTH 16
      #define LVGL_TICK_PERIOD 6

      #define FRAME_X 100
      #define FRAME_Y 64
      #define FRAME_W 120
      #define FRAME_H 50

      // Red zone size
      #define REDBUTTON_X FRAME_X
      #define REDBUTTON_Y FRAME_Y
      #define REDBUTTON_W (FRAME_W/2)
      #define REDBUTTON_H FRAME_H

      // Green zone size
      #define GREENBUTTON_X (REDBUTTON_X + REDBUTTON_W)
      #define GREENBUTTON_Y FRAME_Y
      #define GREENBUTTON_W (FRAME_W/2)
      #define GREENBUTTON_H FRAME_H
    
      #define STATUSBAR_COLOR 0x4A49
    
      #define KIT_LED_BUILTIN 13
    #endif
  
    #ifdef MARAUDER_MINI
      #define SCREEN_CHAR_WIDTH 40
      #define TFT_MISO 19
      #define TFT_MOSI 23
      #define TFT_SCLK 18
      #define TFT_CS 27
      #define TFT_DC 26
      #define TFT_RST 5
      #define TFT_BL 32
      #define TOUCH_CS 21
      #define SD_CS 4

      #define SCREEN_BUFFER

      #define MAX_SCREEN_BUFFER 9

      #define BANNER_TEXT_SIZE 1

      #ifndef TFT_WIDTH
        #define TFT_WIDTH 128
      #endif

      #ifndef TFT_HEIGHT
        #define TFT_HEIGHT 128
      #endif

      #define CHAR_WIDTH 6
      #define SCREEN_WIDTH TFT_WIDTH // Originally 240
      #define SCREEN_HEIGHT TFT_HEIGHT // Originally 320
      #define HEIGHT_1 TFT_WIDTH
      #define WIDTH_1 TFT_WIDTH
      #define STANDARD_FONT_CHAR_LIMIT (TFT_WIDTH/6) // number of characters on a single line with normal font
      #define TEXT_HEIGHT (TFT_HEIGHT/10) // Height of text to be printed and scrolled
      #define BOT_FIXED_AREA 0 // Number of lines in bottom fixed area (lines counted from bottom of screen)
      #define TOP_FIXED_AREA 48 // Number of lines in top fixed area (lines counted from top of screen)
      #define YMAX TFT_HEIGHT // Bottom of screen area
      #define minimum(a,b)     (((a) < (b)) ? (a) : (b))
      //#define MENU_FONT NULL
      #define MENU_FONT &FreeMono9pt7b // Winner
      //#define MENU_FONT &FreeMonoBold9pt7b
      //#define MENU_FONT &FreeSans9pt7b
      //#define MENU_FONT &FreeSansBold9pt7b
      #define BUTTON_SCREEN_LIMIT 10
      #define BUTTON_ARRAY_LEN 100
      #define STATUS_BAR_WIDTH (TFT_HEIGHT/16)
      #define LVGL_TICK_PERIOD 6

      #define FRAME_X 100
      #define FRAME_Y 64
      #define FRAME_W 120
      #define FRAME_H 50

      // Red zone size
      #define REDBUTTON_X FRAME_X
      #define REDBUTTON_Y FRAME_Y
      #define REDBUTTON_W (FRAME_W/2)
      #define REDBUTTON_H FRAME_H

      // Green zone size
      #define GREENBUTTON_X (REDBUTTON_X + REDBUTTON_W)
      #define GREENBUTTON_Y FRAME_Y
      #define GREENBUTTON_W (FRAME_W/2)
      #define GREENBUTTON_H FRAME_H
    
      #define STATUSBAR_COLOR 0x4A49
    #endif

    #ifdef MARAUDER_REV_FEATHER
      #define SCREEN_CHAR_WIDTH 40
      //#define TFT_MISO 37
      //#define TFT_MOSI 35
      //#define TFT_SCLK 36
      #define TFT_CS 42
      #define TFT_DC 40
      #define TFT_RST 41
      #define TFT_BL 45
      //#define TOUCH_CS 21
      #define SD_CS 4

      #define SCREEN_BUFFER

      #define MAX_SCREEN_BUFFER 9

      #define BANNER_TEXT_SIZE 1

      #ifndef TFT_WIDTH
        #define TFT_WIDTH 240
      #endif

      #ifndef TFT_HEIGHT
        #define TFT_HEIGHT 135
      #endif

      #define CHAR_WIDTH 6
      #define SCREEN_WIDTH TFT_WIDTH // Originally 240
      #define SCREEN_HEIGHT TFT_HEIGHT // Originally 320
      #define HEIGHT_1 TFT_WIDTH
      #define WIDTH_1 TFT_WIDTH
      #define STANDARD_FONT_CHAR_LIMIT (TFT_WIDTH/6) // number of characters on a single line with normal font
      #define TEXT_HEIGHT (TFT_HEIGHT/10) // Height of text to be printed and scrolled
      #define BOT_FIXED_AREA 0 // Number of lines in bottom fixed area (lines counted from bottom of screen)
      #define TOP_FIXED_AREA 48 // Number of lines in top fixed area (lines counted from top of screen)
      #define YMAX TFT_HEIGHT // Bottom of screen area
      #define minimum(a,b)     (((a) < (b)) ? (a) : (b))
      //#define MENU_FONT NULL
      #define MENU_FONT &FreeMono9pt7b // Winner
      //#define MENU_FONT &FreeMonoBold9pt7b
      //#define MENU_FONT &FreeSans9pt7b
      //#define MENU_FONT &FreeSansBold9pt7b
      #define BUTTON_SCREEN_LIMIT 5
      #define BUTTON_ARRAY_LEN 100
      #define STATUS_BAR_WIDTH (TFT_HEIGHT/16)
      #define LVGL_TICK_PERIOD 6

      #define FRAME_X 100
      #define FRAME_Y 64
      #define FRAME_W 120
      #define FRAME_H 50

      // Red zone size
      #define REDBUTTON_X FRAME_X
      #define REDBUTTON_Y FRAME_Y
      #define REDBUTTON_W (FRAME_W/2)
      #define REDBUTTON_H FRAME_H

      // Green zone size
      #define GREENBUTTON_X (REDBUTTON_X + REDBUTTON_W)
      #define GREENBUTTON_Y FRAME_Y
      #define GREENBUTTON_W (FRAME_W/2)
      #define GREENBUTTON_H FRAME_H
    
      #define STATUSBAR_COLOR 0x4A49
    #endif

  #endif
  //// END DISPLAY DEFINITIONS

  //// MENU DEFINITIONS
  #ifdef MARAUDER_V4
    #define BANNER_TIME 100
    
    #define COMMAND_PREFIX "!"
    
    // Keypad start position, key sizes and spacing
    #define KEY_X 120 // Centre of key
    #define KEY_Y 50
    #define KEY_W 240 // Width and height
    #define KEY_H 22
    #define KEY_SPACING_X 0 // X and Y gap
    #define KEY_SPACING_Y 1
    #define KEY_TEXTSIZE 1   // Font size multiplier
    #define ICON_W 22
    #define ICON_H 22
    #define BUTTON_PADDING 22
    //#define BUTTON_ARRAY_LEN 5
  #endif

  #if defined(MARAUDER_V6) || defined(MARAUDER_V6_1)
    #define BANNER_TIME 100
    
    #define COMMAND_PREFIX "!"
    
    // Keypad start position, key sizes and spacing
    #define KEY_X 120 // Centre of key
    #define KEY_Y 50
    #define KEY_W 240 // Width and height
    #define KEY_H 22
    #define KEY_SPACING_X 0 // X and Y gap
    #define KEY_SPACING_Y 1
    #define KEY_TEXTSIZE 1   // Font size multiplier
    #define ICON_W 22
    #define ICON_H 22
    #define BUTTON_PADDING 22
    //#define BUTTON_ARRAY_LEN 5
  #endif

  #ifdef MARAUDER_KIT
    #define BANNER_TIME 100
    
    #define COMMAND_PREFIX "!"
    
    // Keypad start position, key sizes and spacing
    #define KEY_X 120 // Centre of key
    #define KEY_Y 50
    #define KEY_W 240 // Width and height
    #define KEY_H 22
    #define KEY_SPACING_X 0 // X and Y gap
    #define KEY_SPACING_Y 1
    #define KEY_TEXTSIZE 1   // Font size multiplier
    #define ICON_W 22
    #define ICON_H 22
    #define BUTTON_PADDING 22
    //#define BUTTON_ARRAY_LEN 5
  #endif
  
  #ifdef MARAUDER_MINI
    #define BANNER_TIME 50
    
    #define COMMAND_PREFIX "!"
    
    // Keypad start position, key sizes and spacing
    #define KEY_X (TFT_WIDTH/2) // Centre of key
    #define KEY_Y (TFT_HEIGHT/4.5)
    #define KEY_W TFT_WIDTH // Width and height
    #define KEY_H (TFT_HEIGHT/12.8)
    #define KEY_SPACING_X 0 // X and Y gap
    #define KEY_SPACING_Y 1
    #define KEY_TEXTSIZE 1   // Font size multiplier
    #define ICON_W 22
    #define ICON_H 22
    #define BUTTON_PADDING 10
  #endif

  #ifdef MARAUDER_REV_FEATHER
    #define BANNER_TIME 50
    
    #define COMMAND_PREFIX "!"
    
    // Keypad start position, key sizes and spacing
    #define KEY_X (TFT_WIDTH/2) // Centre of key
    #define KEY_Y (TFT_HEIGHT/4.5)
    #define KEY_W TFT_WIDTH // Width and height
    #define KEY_H (TFT_HEIGHT/12.8)
    #define KEY_SPACING_X 0 // X and Y gap
    #define KEY_SPACING_Y 1
    #define KEY_TEXTSIZE 1   // Font size multiplier
    #define ICON_W 22
    #define ICON_H 22
    #define BUTTON_PADDING 10
  #endif

  #ifdef MARAUDER_M5STICKC
    #define BANNER_TIME 50
    
    #define COMMAND_PREFIX "!"
    
    // Keypad start position, key sizes and spacing
    #define KEY_X (TFT_WIDTH/2) // Centre of key
    #define KEY_Y (TFT_HEIGHT/5)
    #define KEY_W TFT_HEIGHT // Width and height
    #define KEY_H (TFT_HEIGHT/17)
    #define KEY_SPACING_X 0 // X and Y gap
    #define KEY_SPACING_Y 1
    #define KEY_TEXTSIZE 1   // Font size multiplier
    #define ICON_W 22
    #define ICON_H 22
    #define BUTTON_PADDING 60
  #endif

  #ifdef MARAUDER_M5CARDPUTER
    #define BANNER_TIME 50
    
    #define COMMAND_PREFIX "!"
    
    // Keypad start position, key sizes and spacing
    #define KEY_X (TFT_WIDTH/2) // Centre of key
    #define KEY_Y (TFT_HEIGHT/5)
    #define KEY_W TFT_HEIGHT // Width and height
    #define KEY_H (TFT_HEIGHT/17)
    #define KEY_SPACING_X 0 // X and Y gap
    #define KEY_SPACING_Y 1
    #define KEY_TEXTSIZE 1   // Font size multiplier
    #define ICON_W 22
    #define ICON_H 22
    #define BUTTON_PADDING 60
  #endif
  //// END MENU DEFINITIONS

  //// SD DEFINITIONS
  #if defined(USE_SD)

    #ifdef MARAUDER_V4
      #define SD_CS 12
    #endif

    #ifdef MARAUDER_V6
      #define SD_CS 12
    #endif

    #ifdef MARAUDER_V6_1
      #define SD_CS 14
    #endif

    #ifdef MARAUDER_KIT
      #define SD_CS 12
    #endif

    #ifdef MARAUDER_MINI
      #define SD_CS 4
    #endif

    #ifdef MARAUDER_REV_FEATHER
      #define SD_CS 5
    #endif

    #ifdef MARAUDER_M5STICKC
      /* Set up SPI SD Card using external pin header
      StickCPlus Header - SPI SD Card Reader
                  3v3   -   3v3
                  GND   -   GND
                   G0   -   CLK
              G36/G25   -   MISO
                  G26   -   MOSI
                        -   CS (jumper to SD Card GND Pin)
      */

      #define SD_CS -1
      #define SD_SCK 0
      #define SD_MISO 36
      #define SD_MOSI 26
    #endif

    #ifdef MARAUDER_M5CARDPUTER
      #define SD_CS 12
      #define SD_SCK 40
      #define SD_MISO 39
      #define SD_MOSI 14
    #endif

    #ifdef MARAUDER_FLIPPER
      #define SD_CS 10
    #endif

    #ifdef ESP32_LDDB
      #define SD_CS 4
    #endif

    #ifdef MARAUDER_DEV_BOARD_PRO
      #define SD_CS 4
    #endif

    #ifdef XIAO_ESP32_S3
      #define SD_CS 3
    #endif

  #endif
  //// END SD DEFINITIONS

  //// SCREEN STUFF
  #ifndef HAS_SCREEN

    #define TFT_WHITE 0
    #define TFT_CYAN 0
    #define TFT_BLUE 0
    #define TFT_RED 0
    #define TFT_GREEN 0
    #define TFT_GREY 0
    #define TFT_GRAY 0
    #define TFT_MAGENTA 0
    #define TFT_VIOLET 0
    #define TFT_ORANGE 0
    #define TFT_YELLOW 0
    #define STANDARD_FONT_CHAR_LIMIT 40
    #define FLASH_BUTTON -1

    #include <FS.h>
    #include <functional>
    #include <LinkedList.h>
    #include "SPIFFS.h"
    #include "Assets.h"

  #endif
  //// END SCREEN STUFF

  //// MEMORY LOWER LIMIT STUFF
  // These values are in bytes
  #ifdef MARAUDER_M5STICKC
    #define MEM_LOWER_LIM 20000
  #elif defined(MARAUDER_M5CARDPUTER)
    #define MEM_LOWER_LIM 20000
  #elif defined(MARAUDER_MINI)
    #define MEM_LOWER_LIM 20000
  #elif defined(MARAUDER_REV_FEATHER)
    #define MEM_LOWER_LIM 20000
  #elif defined(MARAUDER_V4)
    #define MEM_LOWER_LIM 20000
  #elif defined(MARAUDER_V6) || defined(MARAUDER_V6_1)
    #define MEM_LOWER_LIM 20000
  #elif defined(MARAUDER_KIT)
    #define MEM_LOWER_LIM 20000
  #elif defined(GENERIC_ESP32)
    #define MEM_LOWER_LIM 20000
  #elif defined(MARAUDER_FLIPPER)
    #define MEM_LOWER_LIM 20000
  #elif defined(ESP32_LDDB)
    #define MEM_LOWER_LIM 20000
  #elif defined(MARAUDER_DEV_BOARD_PRO)
    #define MEM_LOWER_LIM 20000
  #elif defined(XIAO_ESP32_S3)
    #define MEM_LOWER_LIM 20000
  #endif
  //// END MEMORY LOWER LIMIT STUFF

  //// NEOPIXEL STUFF  
  #ifdef HAS_NEOPIXEL_LED
    
    #if defined(ESP32_LDDB)
      #define PIN 17
    #elif defined(MARAUDER_DEV_BOARD_PRO)
      #define PIN 16
    #elif defined(MARAUDER_REV_FEATHER)
      #define PIN 33
    #else
      #define PIN 25
    #endif
  
  #endif
  //// END NEOPIXEL STUFF

  //// EVIL PORTAL STUFF
  #ifdef MARAUDER_M5STICKC
    #define MAX_HTML_SIZE 11400
  #elif defined(MARAUDER_M5CARDPUTER)
    #define MAX_HTML_SIZE 11400
  #elif defined(MARAUDER_MINI)
    #define MAX_HTML_SIZE 11400
  #elif defined(MARAUDER_REV_FEATHER)
    #define MAX_HTML_SIZE 11400
  #elif defined(MARAUDER_V4)
    #define MAX_HTML_SIZE 11400
  #elif defined(MARAUDER_V6) || defined(MARAUDER_V6_1)
    #define MAX_HTML_SIZE 11400
  #elif defined(MARAUDER_KIT)
    #define MAX_HTML_SIZE 11400
  #elif defined(GENERIC_ESP32)
    #define MAX_HTML_SIZE 20000
  #elif defined(MARAUDER_FLIPPER)
    #define MAX_HTML_SIZE 20000
  #elif defined(ESP32_LDDB)
    #define MAX_HTML_SIZE 20000
  #elif defined(MARAUDER_DEV_BOARD_PRO)
    #define MAX_HTML_SIZE 20000
  #elif defined(XIAO_ESP32_S3)
    #define MAX_HTML_SIZE 20000
  #else
    #define MAX_HTML_SIZE 20000
  #endif
  //// END EVIL PORTAL STUFF

  //// GPS STUFF
  #ifdef HAS_GPS
    #if defined(MARAUDER_V6) || defined(MARAUDER_V6_1)
      #define GPS_SERIAL_INDEX 2
      #define GPS_TX 4
      #define GPS_RX 13
      #define mac_history_len 512
    #elif defined(MARAUDER_V4)
      #define GPS_SERIAL_INDEX 2
      #define GPS_TX 4
      #define GPS_RX 13
      #define mac_history_len 512
    #elif defined(MARAUDER_KIT)
      #define GPS_SERIAL_INDEX 2
      #define GPS_TX 4
      #define GPS_RX 13
      #define mac_history_len 512
    #elif defined(MARAUDER_DEV_BOARD_PRO)
      #define GPS_SERIAL_INDEX 2
      #define GPS_TX 21
      #define GPS_RX 17
      #define mac_history_len 512
    #elif defined(MARAUDER_MINI)
      #define GPS_SERIAL_INDEX 2
      #define GPS_TX 21
      #define GPS_RX 22
      #define mac_history_len 512
    #elif defined(MARAUDER_FLIPPER)
      #define GPS_SERIAL_INDEX 1
      #define GPS_TX 9
      #define GPS_RX 21
      #define mac_history_len 512
    #elif defined(MARAUDER_M5STICKC)
      #define GPS_SERIAL_INDEX 1
      #define GPS_TX 33
      #define GPS_RX 32
      #define mac_history_len 512
    #elif defined(MARAUDER_M5CARDPUTER)
      #define GPS_SERIAL_INDEX 1
      #define GPS_TX 1
      #define GPS_RX 2
      #define mac_history_len 512
    #elif defined(MARAUDER_REV_FEATHER)
      #define GPS_SERIAL_INDEX 1
      #define GPS_TX 6
      #define GPS_RX 9
      #define mac_history_len 512
    #endif
  #else
    #define mac_history_len 512
  #endif
  //// END GPS STUFF

  //// MARAUDER TITLE STUFF
  #ifdef MARAUDER_V4
    #define MARAUDER_TITLE_BYTES 13578
  #elif defined(MARAUDER_V6) || defined(MARAUDER_V6_1)
    #define MARAUDER_TITLE_BYTES 13578
  #elif defined(MARAUDER_KIT)
    #define MARAUDER_TITLE_BYTES 13578
  #elif defined(MARAUDER_MINI)
    #define MARAUDER_TITLE_BYTES 13578
  #elif defined(MARAUDER_REV_FEATHER)
    #define MARAUDER_TITLE_BYTES 13578
  #else
    #define MARAUDER_TITLE_BYTES 13578
  #endif
  //// END MARAUDER TITLE STUFF

  //// SOFTWARE SERIAL
  #ifdef HAS_SOFTWARE_SERIAL
  
  #include <SoftwareSerial.h>
  extern EspSoftwareSerial::UART softwareSerial;
  #define Serial softwareSerial
  
  #if defined(MARAUDER_M5STICKC)
  #define SOFTWARE_SERIAL_RX 36
  #define SOFTWARE_SERIAL_TX 26
  #elif defined(MARAUDER_M5CARDPUTER)
  #define SOFTWARE_SERIAL_RX 1
  #define SOFTWARE_SERIAL_TX 2
  #endif
  
  #endif

#endif
