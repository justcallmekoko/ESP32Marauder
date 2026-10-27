#pragma once

// Seeed reTerminal Sticky: ESP32-S3R8, SSD1677, GT911, shared SPI SD.
#define HAS_SCREEN
#define HAS_FULL_SCREEN
#define HAS_TOUCH
// Historical upstream guard for the full touch UI (no ILI9341 driver here).
#define HAS_ILI9341
#define HAS_BUTTONS
#define HAS_BT
#define HAS_PSRAM
#define HAS_IDF_3
#define HAS_NIMBLE_2
#define HAS_SD
#define USE_SD
// Upstream's shared-SPI storage interface also supports ESP32-S3.
#define HAS_C5_SD
#define HAS_DIRECT_UPLOAD

#define L_BTN -1
#define R_BTN -1
#define C_BTN 4
#define U_BTN 5
#define D_BTN 6
#define HAS_C
#define HAS_U
#define HAS_D
#define L_PULL true
#define R_PULL true
#define C_PULL true
#define U_PULL true
#define D_PULL true

#define SD_CS 8
#define SD_SCK 13
#define SD_MISO 12
#define SD_MOSI 14
#define TFT_CS 15
#define TFT_BL -1 // No backlight; all Sticky LEDC paths are disabled.

#define TFT_WIDTH 480
#define TFT_HEIGHT 800
#define SCREEN_WIDTH TFT_WIDTH
#define SCREEN_HEIGHT TFT_HEIGHT
#define SCREEN_ORIENTATION 0
#define HEIGHT_1 TFT_WIDTH
#define WIDTH_1 TFT_HEIGHT
#define YMAX TFT_HEIGHT
#define GRAPH_VERT_LIM (TFT_HEIGHT / 2 - 1)
#define SCREEN_BUFFER
#define MAX_SCREEN_BUFFER 24
#define SCREEN_CHAR_WIDTH 40
#define STANDARD_FONT_CHAR_LIMIT (TFT_WIDTH / 12)
#define CHAR_WIDTH 12
#define TEXT_HEIGHT 24
#define BANNER_TEXT_SIZE 3
#define TOP_FIXED_AREA 208
#define BOT_FIXED_AREA 0
#define STATUS_BAR_WIDTH 24
#define EXT_BUTTON_WIDTH 48
#define BUTTON_SCREEN_LIMIT 8
#define BUTTON_ARRAY_LEN BUTTON_SCREEN_LIMIT
#define MENU_FONT NULL
#define minimum(a,b) (((a) < (b)) ? (a) : (b))
#define LVGL_TICK_PERIOD 6
#define CHAN_PER_PAGE 7
#define BANNER_TIME 1000
#define COMMAND_PREFIX "!"
#define KEY_X (TFT_WIDTH / 2)
#define KEY_Y 150
#define KEY_W (TFT_WIDTH - 32)
#define KEY_H 68
#define KEY_SPACING_X 0
#define KEY_SPACING_Y 6
#define KEY_TEXTSIZE 3
#define ICON_W 22
#define ICON_H 22
#define BUTTON_PADDING 30
#define FRAME_X 80
#define FRAME_Y 80
#define FRAME_W 320
#define FRAME_H 64
#define REDBUTTON_X FRAME_X
#define REDBUTTON_Y FRAME_Y
#define REDBUTTON_W (FRAME_W / 2)
#define REDBUTTON_H FRAME_H
#define GREENBUTTON_X (FRAME_X + REDBUTTON_W)
#define GREENBUTTON_Y FRAME_Y
#define GREENBUTTON_W (FRAME_W / 2)
#define GREENBUTTON_H FRAME_H
#define STATUSBAR_COLOR 0x4A49
