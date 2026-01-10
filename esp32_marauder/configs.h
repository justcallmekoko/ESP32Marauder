#pragma once

#ifndef configs_h

  #define configs_h

  #define POLISH_POTATO

  //#define DEVELOPER

  //// BOARD TARGETS
  //#define MARAUDER_M5STICKC
  //#define MARAUDER_M5STICKCP2
  //#define MARAUDER_MINI
  //#define MARAUDER_V4
  //#define MARAUDER_V6
  //#define MARAUDER_V6_1
  //#define MARAUDER_V7
  //#define MARAUDER_V7_1
  //#define MARAUDER_KIT
  //#define GENERIC_ESP32
  //#define MARAUDER_FLIPPER
  //#define MARAUDER_MULTIBOARD_S3
  //#define ESP32_LDDB
  //#define MARAUDER_DEV_BOARD_PRO
  //#define XIAO_ESP32_S3
  //#define MARAUDER_REV_FEATHER
  //#define MARAUDER_CYD_MICRO // 2432S028
  //#define MARAUDER_CYD_2USB // Another 2432S028 but it has tWo UsBs OoOoOoO
  //#define MARAUDER_CYD_GUITION // ESP32-2432S024 GUITION
  //#define MARAUDER_CYD_3_5_INCH
  //#define MARAUDER_C5
  //#define MARAUDER_CARDPUTER
  //#define MARAUDER_V8
  //// END BOARD TARGETS

  #define MARAUDER_VERSION "v1.10.0"

  #define GRAPH_REFRESH   100

  #define TRACK_EVICT_SEC 90 // Seconds before marking tracked MAC as tombstone

  #define DUAL_BAND_CHANNELS 51

  //// HARDWARE NAMES
  #ifdef MARAUDER_M5STICKC
    #define HARDWARE_NAME "M5Stick-C Plus"
  #elif defined(MARAUDER_M5STICKCP2)
    #define HARDWARE_NAME "M5Stick-C Plus2"
  #elif defined(MARAUDER_CARDPUTER)
    #define HARDWARE_NAME "M5 Cardputer"
  #elif defined(MARAUDER_MINI)
    #define HARDWARE_NAME "Marauder Mini"
  #elif defined(MARAUDER_V7)
    #define HARDWARE_NAME "Marauder v7"
  #elif defined(MARAUDER_V7_1)
    #define HARDWARE_NAME "Marauder v7.1"
  #elif defined(MARAUDER_REV_FEATHER)
    #define HARDWARE_NAME "Adafruit Feather ESP32-S2 Reverse TFT"
  #elif defined(MARAUDER_V4)
    #define HARDWARE_NAME "Marauder v4"
  #elif defined(MARAUDER_V6)
    #define HARDWARE_NAME "Marauder v6"
  #elif defined(MARAUDER_V6_1)
    #define HARDWARE_NAME "Marauder v6.1"
  #elif defined(MARAUDER_CYD_MICRO)
    #define HARDWARE_NAME "CYD 2432S028"
  #elif defined(MARAUDER_CYD_2USB)
    #define HARDWARE_NAME "CYD 2432S028 2USB"
  #elif defined(MARAUDER_CYD_3_5_INCH)
    #define HARDWARE_NAME "CYD 3.5inch"
  #elif defined(MARAUDER_CYD_GUITION)
    #define HARDWARE_NAME "CYD 2432S024 GUITION"
  #elif defined(MARAUDER_KIT)
    #define HARDWARE_NAME "Marauder Kit"
  #elif defined(MARAUDER_FLIPPER)
    #define HARDWARE_NAME "Flipper Zero Dev Board"
  #elif defined(MARAUDER_MULTIBOARD_S3)
    #define HARDWARE_NAME "Flipper Zero Multi Board S3"
  #elif defined(ESP32_LDDB)
    #define HARDWARE_NAME "ESP32 LDDB"
  #elif defined(MARAUDER_DEV_BOARD_PRO)
    #define HARDWARE_NAME "Flipper Zero Dev Board Pro"
  #elif defined(XIAO_ESP32_S3)
    #define HARDWARE_NAME "XIAO ESP32 S3"
  #elif defined(MARAUDER_C5)
    #define HARDWARE_NAME "ESP32-C5 DevKit"
  #elif defined(MARAUDER_V8)
    #define HARDWARE_NAME "Marauder v8"
  #else
    #define HARDWARE_NAME "ESP32"
  #endif

  //// END HARDWARE NAMES

 //// BOARD FEATURES
  #if defined(MARAUDER_M5STICKC) || defined(MARAUDER_M5STICKCP2)
    //#define FLIPPER_ZERO_HAT
    #define HAS_MINI_KB
    #define HAS_BATTERY
    #define HAS_BT
    #define HAS_BUTTONS
    //#define HAS_NEOPIXEL_LED
    #define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_MINI_SCREEN
    #define HAS_SD
    #define USE_SD
    #define HAS_TEMP_SENSOR
    #define HAS_GPS
  #endif

  #if defined(MARAUDER_CARDPUTER)
    //#define FLIPPER_ZERO_HAT
    #define HAS_MINI_KB
    //#define HAS_BATTERY
    #define HAS_BT
    #define HAS_BUTTONS
    //#define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_MINI_SCREEN
    #define HAS_SD
    #define USE_SD
    #define HAS_TEMP_SENSOR
    #define HAS_GPS
  #endif

  #ifdef MARAUDER_MINI
    //#define FLIPPER_ZERO_HAT
    //#define HAS_BATTERY
    #define HAS_MINI_KB
    #define HAS_BT
    #define HAS_BUTTONS
    #define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_MINI_SCREEN
    #define HAS_SD
    #define USE_SD
    #define HAS_TEMP_SENSOR
    #define HAS_GPS
  #endif

  #ifdef MARAUDER_V7
    //#define FLIPPER_ZERO_HAT
    #define HAS_MINI_KB
    #define HAS_BATTERY
    #define HAS_BT
    #define HAS_BT_REMOTE
    #define HAS_BUTTONS
    #define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_FULL_SCREEN
    #define HAS_SD
    #define USE_SD
    #define HAS_TEMP_SENSOR
    #define HAS_GPS
  #endif

  #ifdef MARAUDER_V7_1
    //#define FLIPPER_ZERO_HAT
    #define HAS_MINI_KB
    #define HAS_BATTERY
    #define HAS_BT
    #define HAS_BT_REMOTE
    #define HAS_BUTTONS
    #define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_FULL_SCREEN
    #define HAS_SD
    #define USE_SD
    #define HAS_TEMP_SENSOR
    #define HAS_GPS
    #define HAS_PSRAM
  #endif

  #ifdef MARAUDER_REV_FEATHER
    //#define FLIPPER_ZERO_HAT
    //#define HAS_BATTERY
    //#define HAS_BT
    #define HAS_MINI_KB
    #define HAS_BUTTONS
    #define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_MINI_SCREEN
    #define HAS_SD
    #define USE_SD
    #define HAS_TEMP_SENSOR
    #define HAS_GPS
  #endif

  #ifdef MARAUDER_V4
    #define HAS_TOUCH
    //#define FLIPPER_ZERO_HAT
    #define HAS_BATTERY
    #define HAS_BT
    //#define HAS_BUTTONS
    #define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_FULL_SCREEN
    #define HAS_SD
    #define USE_SD
    #define HAS_TEMP_SENSOR
    #define HAS_GPS
  #endif

  #if defined(MARAUDER_V6) || defined(MARAUDER_V6_1)
    #define HAS_TOUCH
    //#define FLIPPER_ZERO_HAT
    #define HAS_BATTERY
    #define HAS_BT
    #define HAS_BT_REMOTE
    #define HAS_BUTTONS
    #define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_FULL_SCREEN
    #define HAS_SD
    #define USE_SD
    #define HAS_TEMP_SENSOR
    #define HAS_GPS
  #endif

  #ifdef MARAUDER_CYD_MICRO
    #define HAS_TOUCH
    #define HAS_FLIPPER_LED
    //#define FLIPPER_ZERO_HAT
    //#define HAS_BATTERY
    #define HAS_BT
    #define HAS_BT_REMOTE
    #define HAS_BUTTONS
    //#define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_FULL_SCREEN
    #define HAS_SD
    #define USE_SD
    #define HAS_TEMP_SENSOR
    #define HAS_GPS
    #define HAS_CYD_TOUCH
  #endif

  #ifdef MARAUDER_CYD_2USB
    #define HAS_TOUCH
    #define HAS_FLIPPER_LED
    //#define FLIPPER_ZERO_HAT
    //#define HAS_BATTERY
    #define HAS_BT
    #define HAS_BT_REMOTE
    #define HAS_BUTTONS
    //#define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_FULL_SCREEN
    #define HAS_SD
    #define USE_SD
    #define HAS_TEMP_SENSOR
    #define HAS_GPS
    #define HAS_CYD_TOUCH
    #define HAS_CYD_PORTRAIT
  #endif

  #ifdef MARAUDER_CYD_3_5_INCH
    #define HAS_TOUCH
    #define HAS_FLIPPER_LED
    //#define FLIPPER_ZERO_HAT
    //#define HAS_BATTERY
    #define HAS_BT
    #define HAS_BT_REMOTE
    #define HAS_BUTTONS
    //#define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_FULL_SCREEN
    #define HAS_SD
    #define USE_SD
    #define HAS_TEMP_SENSOR
    #define HAS_GPS
    //#define HAS_CYD_TOUCH
    #define HAS_SEPARATE_SD
    #define HAS_CYD_PORTRAIT
  #endif

  #ifdef MARAUDER_CYD_GUITION
    #define HAS_TOUCH
    #define HAS_FLIPPER_LED
    //#define FLIPPER_ZERO_HAT
    //#define HAS_BATTERY
    #define HAS_BT
    #define HAS_BT_REMOTE
    #define HAS_BUTTONS
    //#define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_FULL_SCREEN
    #define HAS_SD
    #define USE_SD
    #define HAS_TEMP_SENSOR
    #define HAS_GPS
    //#define HAS_CYD_TOUCH
  #endif

  #ifdef MARAUDER_KIT
    #define HAS_TOUCH
    //#define FLIPPER_ZERO_HAT
    #define HAS_BATTERY
    #define HAS_BT
    //#define HAS_BUTTONS
    #define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_FULL_SCREEN
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
    #define HAS_FLIPPER_LED
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
    #define HAS_PSRAM
    //#define HAS_TEMP_SENSOR
  #endif

  #ifdef MARAUDER_MULTIBOARD_S3
    #define HAS_FLIPPER_LED
    //#define FLIPPER_ZERO_HAT
    //#define HAS_BATTERY
    #define HAS_BT
    //#define HAS_BUTTONS
    //#define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    //#define HAS_SCREEN
    #define HAS_GPS
    #define HAS_SD
    #define USE_SD
    //#define HAS_PSRAM
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

  #ifdef MARAUDER_C5
    //#define HAS_FLIPPER_LED
    //#define FLIPPER_ZERO_HAT
    //#define HAS_BATTERY
    #define HAS_BT
    //#define HAS_BUTTONS
    #define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    //#define HAS_SCREEN
    #define HAS_GPS
    #define HAS_C5_SD
    #define HAS_SD
    #define USE_SD
    #define HAS_DUAL_BAND
    //#define HAS_PSRAM
    //#define HAS_TEMP_SENSOR
  #endif

  #ifdef MARAUDER_V8
    #define HAS_TOUCH
    //#define HAS_FLIPPER_LED
    //#define FLIPPER_ZERO_HAT
    #define HAS_BATTERY
    #define HAS_BT
    //#define HAS_BUTTONS
    //#define HAS_NEOPIXEL_LED
    //#define HAS_PWR_MGMT
    #define HAS_SCREEN
    #define HAS_FULL_SCREEN
    #define HAS_GPS
    #define HAS_C5_SD
    #define HAS_SD
    #define USE_SD
    #define HAS_DUAL_BAND
    #define HAS_PSRAM
    //#define HAS_TEMP_SENSOR
  #endif
  //// END BOARD FEATURES

  //// POWER MANAGEMENT
  #ifdef HAS_PWR_MGMT
    #if defined(MARAUDER_M5STICKC) || defined(MARAUDER_M5STICKCP2)
      #include "AXP192.h"
    #endif

    #ifdef MARAUDER_M5STICKCP2 
        // Prevent StickCP2 from turning off when disconnect USB cable
        #define POWER_HOLD_PIN 4
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

    #ifdef MARAUDER_V7
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

    #ifdef MARAUDER_V7_1
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

    #if defined(MARAUDER_M5STICKC) || defined(MARAUDER_M5STICKCP2)
      #define L_BTN -1
      #define C_BTN 37
      #if defined(MARAUDER_M5STICKCP2)
        #define U_BTN 35
      #else
        #define U_BTN -1
      #endif
      #define R_BTN -1
      #define D_BTN 39

      //#define HAS_L
      //#define HAS_R
      #if defined(MARAUDER_M5STICKCP2)
        #define HAS_U
      #endif
      #define HAS_D
      #define HAS_C

      #define L_PULL true
      #define C_PULL true
      #define U_PULL true
      #define R_PULL true
      #define D_PULL true
    #endif

    #ifdef MARAUDER_CARDPUTER
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

    #ifdef MARAUDER_CYD_MICRO
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

    #ifdef MARAUDER_CYD_2USB
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

    #ifdef MARAUDER_CYD_3_5_INCH
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

    #ifdef MARAUDER_CYD_GUITION
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
      #define CHAN_PER_PAGE 7

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

      #define GRAPH_VERT_LIM TFT_HEIGHT/2 - 1

      #define EXT_BUTTON_WIDTH 0

      #define SCREEN_ORIENTATION 1

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
      #define BUTTON_ARRAY_LEN BUTTON_SCREEN_LIMIT
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

  #if defined(MARAUDER_M5STICKCP2)
      #define CHAN_PER_PAGE 7

      #define MARAUDER_M5STICKC // From now on, everything is the same, except for one check in esp32_marauder.ino amd stickc_led.cpp/h

      #define SCREEN_CHAR_WIDTH 40
      #define TFT_MOSI 15
      #define TFT_SCLK 13
      #define TFT_CS 5
      #define TFT_DC 14
      #define TFT_RST 12
      #define TFT_BL 27
      #define TOUCH_CS -1

      #define SCREEN_BUFFER

      #define MAX_SCREEN_BUFFER 9

      #define BANNER_TEXT_SIZE 1

      #ifndef TFT_WIDTH
        #define TFT_WIDTH 135
      #endif

      #ifndef TFT_HEIGHT
        #define TFT_HEIGHT 240
      #endif

      #define GRAPH_VERT_LIM TFT_HEIGHT/2 - 1

      #define EXT_BUTTON_WIDTH 0

      #define SCREEN_ORIENTATION 0

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
      #define BUTTON_ARRAY_LEN BUTTON_SCREEN_LIMIT
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

    #ifdef MARAUDER_CARDPUTER
      #define CHAN_PER_PAGE 7

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

      #define GRAPH_VERT_LIM TFT_HEIGHT/2 - 1

      #define EXT_BUTTON_WIDTH 0

      #define SCREEN_ORIENTATION 1

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

    #ifdef MARAUDER_V4
      #define CHAN_PER_PAGE 7

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

      #define GRAPH_VERT_LIM TFT_HEIGHT/2 - 1

      #define EXT_BUTTON_WIDTH 20

      #define SCREEN_ORIENTATION 0

      #define CHAR_WIDTH 12
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
      #define BUTTON_ARRAY_LEN BUTTON_SCREEN_LIMIT
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
      #define CHAN_PER_PAGE 7

      #define SCREEN_CHAR_WIDTH 40
      #define HAS_ILI9341
    
      #define BANNER_TEXT_SIZE 2

      #ifndef TFT_WIDTH
        #define TFT_WIDTH 240
      #endif

      #ifndef TFT_HEIGHT
        #define TFT_HEIGHT 320
      #endif

      #ifndef MARAUDER_CYD_MICRO
        #define TFT_DIY      
      #endif

      #define GRAPH_VERT_LIM TFT_HEIGHT/2 - 1

      #define EXT_BUTTON_WIDTH 20

      #define SCREEN_BUFFER

      #define MAX_SCREEN_BUFFER 22

      #define SCREEN_ORIENTATION 0
    
      #define CHAR_WIDTH 12
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
      #define BUTTON_ARRAY_LEN BUTTON_SCREEN_LIMIT
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

    #if defined(MARAUDER_V8)
      #define CHAN_PER_PAGE 7

      #define SCREEN_CHAR_WIDTH 40
      #define HAS_ILI9341
    
      #define BANNER_TEXT_SIZE 2

      #ifndef TFT_WIDTH
        #define TFT_WIDTH 240
      #endif

      #ifndef TFT_HEIGHT
        #define TFT_HEIGHT 320
      #endif

      //#ifndef MARAUDER_CYD_MICRO
        //#define TFT_DIY
        //#define TFT_SHIELD
      //#endif

      #define GRAPH_VERT_LIM TFT_HEIGHT/2 - 1

      #define EXT_BUTTON_WIDTH 20

      #define SCREEN_BUFFER

      #define MAX_SCREEN_BUFFER 22

      #define SCREEN_ORIENTATION 0
    
      #define CHAR_WIDTH 12
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
      #define BUTTON_ARRAY_LEN BUTTON_SCREEN_LIMIT
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

    #if defined(MARAUDER_CYD_MICRO)
      #define CHAN_PER_PAGE 7

      #define SCREEN_CHAR_WIDTH 40
      #define HAS_ILI9341
    
      #define BANNER_TEXT_SIZE 2

      #ifndef TFT_WIDTH
        #define TFT_WIDTH 240
      #endif

      #ifndef TFT_HEIGHT
        #define TFT_HEIGHT 320
      #endif

      #define GRAPH_VERT_LIM TFT_HEIGHT/2 - 1

      #define EXT_BUTTON_WIDTH 20

      #define SCREEN_BUFFER

      #define MAX_SCREEN_BUFFER 22

      #define SCREEN_ORIENTATION 0
    
      #define CHAR_WIDTH 12
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
      #define BUTTON_ARRAY_LEN BUTTON_SCREEN_LIMIT
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

    #if defined(MARAUDER_CYD_2USB)
      #define CHAN_PER_PAGE 7

      #define SCREEN_CHAR_WIDTH 40
      #define HAS_ILI9341
      #define HAS_ST7789
    
      #define BANNER_TEXT_SIZE 2

      #ifndef TFT_WIDTH
        #define TFT_WIDTH 240
      #endif

      #ifndef TFT_HEIGHT
        #define TFT_HEIGHT 320
      #endif

      #define GRAPH_VERT_LIM TFT_HEIGHT/2 - 1

      #define EXT_BUTTON_WIDTH 20

      #define SCREEN_BUFFER

      #define MAX_SCREEN_BUFFER 22

      #define SCREEN_ORIENTATION 0
    
      #define CHAR_WIDTH 12
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
      #define BUTTON_ARRAY_LEN BUTTON_SCREEN_LIMIT
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

    #if defined(MARAUDER_CYD_3_5_INCH)
      #define CHAN_PER_PAGE 7

      #define SCREEN_CHAR_WIDTH 40
      #define HAS_ILI9341
      #define HAS_ST7796
    
      #define BANNER_TEXT_SIZE 2

      #ifndef TFT_WIDTH
        #define TFT_WIDTH 320
      #endif

      #ifndef TFT_HEIGHT
        #define TFT_HEIGHT 480
      #endif

      #define TFT_DIY

      #define GRAPH_VERT_LIM TFT_HEIGHT/2 - 1

      #define EXT_BUTTON_WIDTH 20

      #define SCREEN_BUFFER

      #define MAX_SCREEN_BUFFER 33

      #define SCREEN_ORIENTATION 0
    
      #define CHAR_WIDTH 12
      #define SCREEN_WIDTH TFT_WIDTH
      #define SCREEN_HEIGHT TFT_HEIGHT
      #define HEIGHT_1 TFT_WIDTH
      #define WIDTH_1 TFT_HEIGHT
      #define STANDARD_FONT_CHAR_LIMIT (TFT_WIDTH/6) // number of characters on a single line with normal font
      #define TEXT_HEIGHT 16 // Height of text to be printed and scrolled
      #define BOT_FIXED_AREA 0 // Number of lines in bottom fixed area (lines counted from bottom of screen)
      #define TOP_FIXED_AREA 48 // Number of lines in top fixed area (lines counted from top of screen)
      #define YMAX 480 // Bottom of screen area
      #define minimum(a,b)     (((a) < (b)) ? (a) : (b))
      //#define MENU_FONT NULL
      #define MENU_FONT &FreeMono9pt7b // Winner
      //#define MENU_FONT &FreeMonoBold9pt7b
      //#define MENU_FONT &FreeSans9pt7b
      //#define MENU_FONT &FreeSansBold9pt7b
      #define BUTTON_SCREEN_LIMIT 12
      #define BUTTON_ARRAY_LEN BUTTON_SCREEN_LIMIT
      #define STATUS_BAR_WIDTH 16
      #define LVGL_TICK_PERIOD 6

      #define FRAME_X 100
      #define FRAME_Y 64
      #define FRAME_W TFT_WIDTH / 2
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

    #if defined(MARAUDER_CYD_GUITION)
      #define CHAN_PER_PAGE 7

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

      #define GRAPH_VERT_LIM TFT_HEIGHT/2 - 1

      #define EXT_BUTTON_WIDTH 20

      #define SCREEN_BUFFER

      #define MAX_SCREEN_BUFFER 22

      #define SCREEN_ORIENTATION 0
    
      #define CHAR_WIDTH 12
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
      #define BUTTON_ARRAY_LEN BUTTON_SCREEN_LIMIT
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

    #ifdef MARAUDER_V7
      #define CHAN_PER_PAGE 7

      #define SCREEN_CHAR_WIDTH 40
      //#define HAS_ILI9341
    
      #define BANNER_TEXT_SIZE 2

      #ifndef TFT_WIDTH
        #define TFT_WIDTH 240
      #endif

      #ifndef TFT_HEIGHT
        #define TFT_HEIGHT 320
      #endif

      #define GRAPH_VERT_LIM TFT_HEIGHT/2 - 1

      #define TFT_DIY

      #define SCREEN_BUFFER

      #define MAX_SCREEN_BUFFER 22

      #define EXT_BUTTON_WIDTH 0

      #define SCREEN_ORIENTATION 0
    
      #define CHAR_WIDTH 12
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
      #define BUTTON_ARRAY_LEN BUTTON_SCREEN_LIMIT
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

    #ifdef MARAUDER_V7_1
      #define CHAN_PER_PAGE 7

      #define SCREEN_CHAR_WIDTH 40
      //#define HAS_ILI9341
    
      #define BANNER_TEXT_SIZE 2

      #ifndef TFT_WIDTH
        #define TFT_WIDTH 240
      #endif

      #ifndef TFT_HEIGHT
        #define TFT_HEIGHT 320
      #endif

      #define GRAPH_VERT_LIM TFT_HEIGHT/2 - 1

      #define TFT_DIY

      #define SCREEN_BUFFER

      #define MAX_SCREEN_BUFFER 22

      #define EXT_BUTTON_WIDTH 0

      #define SCREEN_ORIENTATION 0
    
      #define CHAR_WIDTH 12
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
      #define BUTTON_ARRAY_LEN BUTTON_SCREEN_LIMIT
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
      #define CHAN_PER_PAGE 7

      #define SCREEN_CHAR_WIDTH 40
      #define HAS_ILI9341
    
      #define BANNER_TEXT_SIZE 2

      #ifndef TFT_WIDTH
        #define TFT_WIDTH 240
      #endif

      #ifndef TFT_HEIGHT
        #define TFT_HEIGHT 320
      #endif

      #define GRAPH_VERT_LIM TFT_HEIGHT/2 - 1

      #define TFT_DIY
      #define KIT

      #define EXT_BUTTON_WIDTH 20

      #define SCREEN_ORIENTATION 0
    
      #define CHAR_WIDTH 12
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
      #define BUTTON_ARRAY_LEN BUTTON_SCREEN_LIMIT
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
      #define CHAN_PER_PAGE 7

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

      #define GRAPH_VERT_LIM TFT_HEIGHT/2 - 1

      #define EXT_BUTTON_WIDTH 0

      #define SCREEN_ORIENTATION 0

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
      #define BUTTON_ARRAY_LEN BUTTON_SCREEN_LIMIT
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
      #define CHAN_PER_PAGE 7

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

      #define GRAPH_VERT_LIM TFT_HEIGHT/2 - 1

      #define EXT_BUTTON_WIDTH 0

      #define SCREEN_ORIENTATION 1

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
      #define BUTTON_ARRAY_LEN BUTTON_SCREEN_LIMIT
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

  #if defined(MARAUDER_V8)
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

  #if defined(MARAUDER_CYD_MICRO)
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

  #if defined(MARAUDER_CYD_2USB)
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

  #if defined(MARAUDER_CYD_3_5_INCH)
    #define BANNER_TIME 100
    
    #define COMMAND_PREFIX "!"
    
    // Keypad start position, key sizes and spacing
    #define KEY_X 160 // Centre of key
    #define KEY_Y 50
    #define KEY_W 320 // Width and height
    #define KEY_H 22
    #define KEY_SPACING_X 0 // X and Y gap
    #define KEY_SPACING_Y 1
    #define KEY_TEXTSIZE 1   // Font size multiplier
    #define ICON_W 22
    #define ICON_H 22
    #define BUTTON_PADDING 22
    //#define BUTTON_ARRAY_LEN 5
  #endif

  #if defined(MARAUDER_CYD_GUITION)
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

  #ifdef MARAUDER_V7
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

  #ifdef MARAUDER_V7_1
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

  #ifdef MARAUDER_CARDPUTER
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

    #ifdef MARAUDER_CYD_MICRO
      #define SD_CS 5
    #endif

    #ifdef MARAUDER_CYD_2USB
      #define SD_CS 5
    #endif

    #ifdef MARAUDER_CYD_3_5_INCH
      #define SD_CS 5
    #endif

    #ifdef MARAUDER_CYD_GUITION
      #define SD_CS 5
    #endif

    #ifdef MARAUDER_KIT
      #define SD_CS 12
    #endif

    #ifdef MARAUDER_MINI
      #define SD_CS 4
    #endif

    #ifdef MARAUDER_V7
      #define SD_CS 4
    #endif

    #ifdef MARAUDER_V7_1
      #define SD_CS 4
    #endif

    #ifdef MARAUDER_REV_FEATHER
      #define SD_CS 5
    #endif

    #ifdef MARAUDER_M5STICKC
      #define SD_CS -1
    #endif

    #ifdef MARAUDER_CARDPUTER
      //#define SS      12
      #define SD_CS   12
      #define SD_SCK  40
      #define SD_MISO 39
      #define SD_MOSI 14
    #endif

    #ifdef MARAUDER_FLIPPER
      #define SD_CS 10
    #endif

    #ifdef MARAUDER_MULTIBOARD_S3
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

    #ifdef MARAUDER_C5
      #define SD_CS 10
    #endif

    #ifdef MARAUDER_V8
      #define SD_CS 10
    #endif

  #endif
  //// END SD DEFINITIONS

  //// SPACE SAVING COLORS
  #define TFTWHITE     1
  #define TFTCYAN      2
  #define TFTBLUE      3
  #define TFTRED       4
  #define TFTGREEN     5
  #define TFTGREY      6
  #define TFTGRAY      7
  #define TFTMAGENTA   8
  #define TFTVIOLET    9
  #define TFTORANGE    10
  #define TFTYELLOW    11
  #define TFTLIGHTGREY 12
  #define TFTPURPLE    13
  #define TFTNAVY      14
  #define TFTSILVER    15
  #define TFTDARKGREY  16
  #define TFTSKYBLUE   17
  #define TFTLIME      18
  //// END SPACE SAVING COLORS

  #define TFT_FARTGRAY 0x528a

  //// SCREEN STUFF
  #ifndef HAS_SCREEN

    #define BANNER_TIME GRAPH_REFRESH

    #define TFT_BLACK 0
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

    #define CHAN_PER_PAGE 7

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
    #define MEM_LOWER_LIM 10000
  #elif defined(MARAUDER_CARDPUTER)
    #define MEM_LOWER_LIM 10000
  #elif defined(MARAUDER_MINI)
    #define MEM_LOWER_LIM 10000
  #elif defined(MARAUDER_V7)
    #define MEM_LOWER_LIM 10000
  #elif defined(MARAUDER_V7_1)
    #define MEM_LOWER_LIM 10000
  #elif defined(MARAUDER_REV_FEATHER)
    #define MEM_LOWER_LIM 10000
  #elif defined(MARAUDER_V4)
    #define MEM_LOWER_LIM 10000
  #elif defined(MARAUDER_V6) || defined(MARAUDER_V6_1)
    #define MEM_LOWER_LIM 10000
  #elif defined(MARAUDER_CYD_MICRO)
    #define MEM_LOWER_LIM 10000
  #elif defined(MARAUDER_CYD_2USB)
    #define MEM_LOWER_LIM 10000
  #elif defined(MARAUDER_CYD_3_5_INCH)
    #define MEM_LOWER_LIM 10000
  #elif defined(MARAUDER_CYD_GUITION)
    #define MEM_LOWER_LIM 10000
  #elif defined(MARAUDER_KIT)
    #define MEM_LOWER_LIM 10000
  #elif defined(GENERIC_ESP32)
    #define MEM_LOWER_LIM 10000
  #elif defined(MARAUDER_FLIPPER)
    #define MEM_LOWER_LIM 10000
  #elif defined(MARAUDER_MULTIBOARD_S3)
    #define MEM_LOWER_LIM 10000
  #elif defined(ESP32_LDDB)
    #define MEM_LOWER_LIM 10000
  #elif defined(MARAUDER_DEV_BOARD_PRO)
    #define MEM_LOWER_LIM 10000
  #elif defined(XIAO_ESP32_S3)
    #define MEM_LOWER_LIM 10000
  #elif defined(MARAUDER_C5)
    #define MEM_LOWER_LIM 10000
  #elif defined(MARAUDER_V8)
    #define MEM_LOWER_LIM 10000
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
    #elif defined(MARAUDER_CYD_MICRO)
      #define PIN 4
    #elif defined(MARAUDER_CYD_2USB)
      #define PIN 4
    #elif defined(MARAUDER_CYD_3_5_INCH)
      #define PIN 22
    #elif defined(MARAUDER_C5)
      #define PIN 27
    #elif defined(MARAUDER_V8)
      #define PIN 27
    #else
      #define PIN 25
    #endif
  
  #endif
  //// END NEOPIXEL STUFF

  //// EVIL PORTAL STUFF

  #ifdef HAS_PSRAM
    #define MAX_HTML_SIZE 30000
  #else
    #define MAX_HTML_SIZE 11400
  #endif

  //// END EVIL PORTAL STUFF

  //// GPS STUFF
  #ifdef HAS_GPS
    #ifdef HAS_PSRAM
      #define mac_history_len 500
    #else
      #define mac_history_len 100
    #endif

    #if defined(MARAUDER_V6) || defined(MARAUDER_V6_1)
      #define GPS_SERIAL_INDEX 2
      #define GPS_TX 4
      #define GPS_RX 13
    #elif defined(MARAUDER_CYD_MICRO)
      #define GPS_SERIAL_INDEX 2
      #define GPS_TX 22 // Whoever thought it would be a good idea to use UART0 for GPS...
      #define GPS_RX 27 // Now maybe we will be able to use CLI
    #elif defined(MARAUDER_CYD_2USB)
      #define GPS_SERIAL_INDEX 2
      #define GPS_TX 22 // Whoever thought it would be a good idea to use UART0 for GPS...
      #define GPS_RX 27 // Now maybe we will be able to use CLI
    #elif defined(MARAUDER_CYD_3_5_INCH)
      #define GPS_SERIAL_INDEX 2
      #define GPS_TX 21
      #define GPS_RX 25
    #elif defined(MARAUDER_CYD_GUITION)
      #define GPS_SERIAL_INDEX 2
      #define GPS_TX 21 // Fits the extended I/O
      #define GPS_RX 22
    #elif defined(MARAUDER_V4)
      #define GPS_SERIAL_INDEX 2
      #define GPS_TX 4
      #define GPS_RX 13
    #elif defined(MARAUDER_KIT)
      #define GPS_SERIAL_INDEX 2
      #define GPS_TX 4
      #define GPS_RX 13
    #elif defined(MARAUDER_DEV_BOARD_PRO)
      #define GPS_SERIAL_INDEX 2
      #define GPS_TX 21
      #define GPS_RX 17
    #elif defined(MARAUDER_MINI)
      #define GPS_SERIAL_INDEX 2
      #define GPS_TX 21
      #define GPS_RX 22
    #elif defined(MARAUDER_V7)
      #define GPS_SERIAL_INDEX 2
      #define GPS_TX 21
      #define GPS_RX 22
    #elif defined(MARAUDER_V7_1)
      #define GPS_SERIAL_INDEX 2
      #define GPS_TX 21
      #define GPS_RX 22
    #elif defined(MARAUDER_FLIPPER)
      #define GPS_SERIAL_INDEX 1
      #define GPS_TX 9
      #define GPS_RX 21
    #elif defined(MARAUDER_MULTIBOARD_S3)
      #define GPS_SERIAL_INDEX 1
      #define GPS_TX 9
      #define GPS_RX 21
    #elif defined(MARAUDER_M5STICKC)
      #define GPS_SERIAL_INDEX 1
      #define GPS_TX 33
      #define GPS_RX 32
    #elif defined(MARAUDER_CARDPUTER)
      #define GPS_SERIAL_INDEX 1
      #define GPS_TX 1
      #define GPS_RX 2
    #elif defined(MARAUDER_REV_FEATHER)
      #define GPS_SERIAL_INDEX 1
      #define GPS_TX 6
      #define GPS_RX 9
    #elif defined(MARAUDER_C5)
      #define GPS_SERIAL_INDEX 1
      #define GPS_TX 14
      #define GPS_RX 13
    #elif defined(MARAUDER_V8)
      #define GPS_SERIAL_INDEX 1
      #define GPS_TX 14
      #define GPS_RX 13
    #endif
  #else
    #define mac_history_len 100
  #endif
  //// END GPS STUFF

  //// BATTERY STUFF
  #ifdef HAS_BATTERY

    #ifdef MARAUDER_V4
      #define I2C_SDA 33
      #define I2C_SCL 22
    #endif

    #ifdef MARAUDER_V6
      #define I2C_SDA 33
      #define I2C_SCL 22
    #endif

    #ifdef MARAUDER_V6_1
      #define I2C_SDA 33
      #define I2C_SCL 22
    #endif

    #ifdef MARAUDER_M5STICKC
      #define I2C_SDA 33
      #define I2C_SCL 22
    #endif

    #ifdef MARAUDER_KIT
      #define I2C_SDA 33
      #define I2C_SCL 22
    #endif

    #ifdef MARAUDER_MINI
      #define I2C_SDA 33
      #define I2C_SCL 26
    #endif

    #ifdef MARAUDER_V7
      #define I2C_SDA 33
      #define I2C_SCL 16
    #endif

    #ifdef MARAUDER_V7_1
      #define I2C_SDA 33
      #define I2C_SCL 27
    #endif

    #ifdef MARAUDER_CYD_MICRO
      #define I2C_SDA 22
      #define I2C_SCL 27
    #endif

    #ifdef MARAUDER_CYD_2USB
      #define I2C_SDA 22
      #define I2C_SCL 27
    #endif

    #ifdef MARAUDER_CYD_3_5_INCH
      #define I2C_SDA 32
      #define I2C_SCL 25
    #endif

    #ifdef MARAUDER_CYD_GUITION
      #define I2C_SDA 22
      #define I2C_SCL 21
    #endif

    #ifdef MARAUDER_V8
      #define I2C_SCL 4
      #define I2C_SDA 5
    #endif

  #endif

  //// MARAUDER TITLE STUFF
  #ifdef MARAUDER_V4
    #define MARAUDER_TITLE_BYTES 13578
  #elif defined(MARAUDER_V6) || defined(MARAUDER_V6_1)
    #define MARAUDER_TITLE_BYTES 13578
  #elif defined(MARAUDER_CYD_MICRO)
    #define MARAUDER_TITLE_BYTES 13578
  #elif defined(MARAUDER_CYD_2USB)
    #define MARAUDER_TITLE_BYTES 13578
  #elif defined(MARAUDER_CYD_3_5_INCH)
    #define MARAUDER_TITLE_BYTES 13578
  #elif defined(MARAUDER_CYD_GUITION)
    #define MARAUDER_TITLE_BYTES 13578
  #elif defined(MARAUDER_KIT)
    #define MARAUDER_TITLE_BYTES 13578
  #elif defined(MARAUDER_MINI)
    #define MARAUDER_TITLE_BYTES 13578
  #elif defined(MARAUDER_V7)
    #define MARAUDER_TITLE_BYTES 13578
  #elif defined(MARAUDER_V7_1)
    #define MARAUDER_TITLE_BYTES 13578
  #elif defined(MARAUDER_REV_FEATHER)
    #define MARAUDER_TITLE_BYTES 13578
  #elif defined(MARAUDER_C5)
    #define MARAUDER_TITLE_BYTES 13578
  #elif defined(MARAUDER_V8)
    #define MARAUDER_TITLE_BYTES 13578
  #else
    #define MARAUDER_TITLE_BYTES 13578
  #endif
  //// END MARAUDER TITLE STUFF

  //// PCAP BUFFER STUFF
  
  #ifdef HAS_PSRAM
    #define BUF_SIZE 8 * 1024 // Had to reduce buffer size to save RAM. GG @spacehuhn
    #define SNAP_LEN 1 * 4096 // max len of each recieved packet
  #elif !defined(HAS_ILI9341)
    #define BUF_SIZE 8 * 1024 // Had to reduce buffer size to save RAM. GG @spacehuhn
    #define SNAP_LEN 4096 // max len of each recieved packet
  #else
    #define BUF_SIZE 3 * 1024 // Had to reduce buffer size to save RAM. GG @spacehuhn
    #define SNAP_LEN 2324 // max len of each recieved packet
  #endif

  //// PCAP BUFFER STUFF

  //// STUPID CYD STUFF
  #if defined(HAS_CYD_TOUCH) || defined(HAS_C5_SD) || defined(HAS_SEPARATE_SD)
    #ifdef MARAUDER_CYD_MICRO
      #define XPT2046_IRQ  36
      #define XPT2046_MOSI 32
      #define XPT2046_MISO 39
      #define XPT2046_CLK  25
      #define XPT2046_CS   33

      #define SD_MISO      19
      #define SD_MOSI      23
      #define SD_SCK       18
    #endif

    #ifdef MARAUDER_CYD_2USB
      #define XPT2046_IRQ  36
      #define XPT2046_MOSI 32
      #define XPT2046_MISO 39
      #define XPT2046_CLK  25
      #define XPT2046_CS   33

      #define SD_MISO      19
      #define SD_MOSI      23
      #define SD_SCK       18
    #endif

    #ifdef MARAUDER_CYD_3_5_INCH
      #define SD_MISO      19
      #define SD_MOSI      23
      #define SD_SCK       18
    #endif

    #ifdef MARAUDER_C5
      #define SD_MISO 2
      #define SD_MOSI 7
      #define SD_SCK  6
    #endif

    #ifdef MARAUDER_V8
      #define SD_MISO TFT_MISO
      #define SD_MOSI TFT_MOSI
      #define SD_SCK  TFT_SCLK
    #endif
  #endif
  //// END STUPID CYD STUFF

  //// FUNNY FLIPPER LED STUFF

  #ifdef HAS_FLIPPER_LED
    #ifdef MARAUDER_FLIPPER
      #define B_PIN 4
      #define G_PIN 5
      #define R_PIN 6
    #endif

    #ifdef MARAUDER_MULTIBOARD_S3
      #define B_PIN 4
      #define G_PIN 5
      #define R_PIN 6
    #endif

    #ifdef MARAUDER_CYD_MICRO
      #define B_PIN 17
      #define G_PIN 16
      #define R_PIN 4
    #endif

    #ifdef MARAUDER_CYD_2USB
      #define B_PIN 17
      #define G_PIN 16
      #define R_PIN 4
    #endif

    #ifdef MARAUDER_CYD_3_5_INCH
      #define B_PIN 17
      #define G_PIN 16
      #define R_PIN 22
    #endif

    #ifdef MARAUDER_CYD_GUITION
      #define B_PIN 17
      #define G_PIN 16
      #define R_PIN 4
    #endif
  #endif

  //// END FUNNY FLIPPER LED STUFF

  //// WIFI STUFF

  #ifndef HAS_DUAL_BAND
    #define HOP_DELAY 1000
  #else
    #define HOP_DELAY 250
  #endif
#endif
