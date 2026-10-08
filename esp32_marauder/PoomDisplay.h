#pragma once

#include <Arduino.h>
#include <Print.h>
#include <Wire.h>

#define TFT_BLACK       0x0000
#define TFT_NAVY        0x000F
#define TFT_DARKGREEN   0x03E0
#define TFT_DARKCYAN    0x03EF
#define TFT_MAROON      0x7800
#define TFT_PURPLE      0x780F
#define TFT_OLIVE       0x7BE0
#define TFT_LIGHTGREY   0xD69A
#define TFT_DARKGREY    0x7BEF
#define TFT_BLUE        0x001F
#define TFT_GREEN       0x07E0
#define TFT_CYAN        0x07FF
#define TFT_RED         0xF800
#define TFT_MAGENTA     0xF81F
#define TFT_YELLOW      0xFFE0
#define TFT_WHITE       0xFFFF
#define TFT_ORANGE      0xFDA0
#define TFT_GREENYELLOW 0xB7E0
#define TFT_PINK        0xFE19
#define TFT_BROWN       0x9A60
#define TFT_GOLD        0xFEA0
#define TFT_SILVER      0xC618
#define TFT_SKYBLUE     0x867D
#define TFT_VIOLET      0x915C

#define TL_DATUM 0
#define TC_DATUM 1
#define TR_DATUM 2
#define ML_DATUM 3
#define MC_DATUM 4
#define MR_DATUM 5
#define BL_DATUM 6
#define BC_DATUM 7
#define BR_DATUM 8

class PoomDisplay : public Print {
 public:
  PoomDisplay();
  void init();
  void display(bool force = false);
  void fillScreen(uint16_t color);
  void drawPixel(int32_t x, int32_t y, uint16_t color);
  void drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color);
  void drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color);
  void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color);
  void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);
  void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);
  void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t, uint16_t color);
  void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t, uint16_t color);
  void fillCircle(int32_t x, int32_t y, int32_t r, uint16_t color);
  void drawXBitmap(int32_t x, int32_t y, const uint8_t *bitmap, int32_t w, int32_t h, uint16_t color);
  void drawXBitmap(int32_t x, int32_t y, const uint8_t *bitmap, int32_t w, int32_t h, uint16_t fg, uint16_t bg);
  void setCursor(int32_t x, int32_t y);
  int32_t getCursorY() const { return cursor_y_; }
  void setTextColor(uint16_t fg, uint16_t bg = TFT_BLACK);
  void setTextColor(uint16_t fg, uint16_t bg, bool) { setTextColor(fg, bg); }
  void setTextSize(uint8_t size);
  void setTextFont(uint8_t) {}
  void setFreeFont(const void *) {}
  void setTextWrap(bool wrap_x, bool wrap_y = false);
  int16_t textWidth(const String &text, uint8_t = 1) const;
  int16_t textWidth(const char *text, uint8_t = 1) const;
  int16_t drawString(const String &text, int32_t x, int32_t y, uint8_t = 1);
  int16_t drawString(const char *text, int32_t x, int32_t y, uint8_t = 1);
  int16_t drawCentreString(const String &text, int32_t x, int32_t y, uint8_t = 1);
  int16_t drawCentreString(const char *text, int32_t x, int32_t y, uint8_t = 1);
  int16_t drawRightString(const String &text, int32_t x, int32_t y, uint8_t = 1);
  int16_t drawRightString(const char *text, int32_t x, int32_t y, uint8_t = 1);
  void setRotation(uint8_t rotation) { rotation_ = rotation & 3; }
  uint8_t getRotation() const { return rotation_; }
  int16_t width() const { return 128; }
  int16_t height() const { return 64; }
  bool getTouch(uint16_t *, uint16_t *, uint16_t = 600) { return false; }
  void setTouch(uint16_t *) {}
  size_t write(uint8_t value) override;

 private:
  uint8_t framebuffer_[1024];
  int16_t cursor_x_ = 0;
  int16_t cursor_y_ = 0;
  uint16_t text_fg_ = TFT_WHITE;
  uint16_t text_bg_ = TFT_BLACK;
  uint8_t text_size_ = 1;
  uint8_t rotation_ = 0;
  bool wrap_x_ = true;
  bool wrap_y_ = false;
  bool dirty_ = false;
  uint32_t last_flush_ms_ = 0;
  void command(uint8_t value);
  void drawChar(int16_t x, int16_t y, uint8_t c, uint16_t fg, uint16_t bg, uint8_t size);
  static bool white(uint16_t color) { return color != TFT_BLACK; }
};

class PoomButton {
 public:
  void initButton(PoomDisplay *gfx, int16_t x, int16_t y, uint16_t w, uint16_t h,
                  uint16_t outline, uint16_t fill, uint16_t textcolor,
                  char *label, uint8_t textsize);
  void setLabelDatum(int16_t x_delta, int16_t y_delta, uint8_t datum = MC_DATUM);
  void drawButton(bool inverted = false, String long_name = "");
  bool contains(int16_t x, int16_t y) const;
  void press(bool pressed);
  bool isPressed() const { return currstate_; }
  bool justReleased() const { return !currstate_ && laststate_; }

 private:
  PoomDisplay *gfx_ = nullptr;
  int16_t x_ = 0, y_ = 0, w_ = 0, h_ = 0;
  int16_t label_dx_ = 0, label_dy_ = 0;
  uint16_t outline_ = TFT_WHITE, fill_ = TFT_BLACK, text_ = TFT_WHITE;
  uint8_t text_size_ = 1;
  String label_;
  bool currstate_ = false, laststate_ = false;
};

using TFT_eSPI = PoomDisplay;
using TFT_eSPI_Button = PoomButton;
