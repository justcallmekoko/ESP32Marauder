#pragma once
#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <atomic>
#include "StickyModel.h"
#include "M5PaperModel.h"

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

class M5PaperDisplay : public Adafruit_GFX {
 public:
  M5PaperDisplay() : Adafruit_GFX(540, 960) {}
  static void prepareBoard();
  void init();
  void display(bool force = false);
  void drawPixel(int16_t x, int16_t y, uint16_t color) override;
  void fillScreen(uint16_t color) override;
  void setFreeFont(const GFXfont* f) { setFont(f); }
  void setTextFont(uint8_t) { setFont(nullptr); }
  void setTextColor(uint16_t fg, uint16_t bg = TFT_BLACK) {
    if (fg != bg && ink(fg) == ink(bg)) fg = ink(bg) ? TFT_BLACK : TFT_WHITE;
    Adafruit_GFX::setTextColor(fg, bg);
  }
  void setTextColor(uint16_t fg, uint16_t bg, bool) { setTextColor(fg, bg); }
  void setTextWrap(bool x, bool = false) { Adafruit_GFX::setTextWrap(x); }
  int16_t textWidth(const String& s, uint8_t font = 1);
  int16_t textWidth(const char* s, uint8_t font = 1) { return textWidth(String(s), font); }
  int16_t drawString(const String& s, int32_t x, int32_t y, uint8_t font = 1);
  int16_t drawCentreString(const String& s, int32_t x, int32_t y, uint8_t font = 1);
  int16_t drawRightString(const String& s, int32_t x, int32_t y, uint8_t font = 1);
  void drawXBitmap(int16_t x, int16_t y, const uint8_t* data, int16_t w, int16_t h, uint16_t fg);
  void drawXBitmap(int16_t x, int16_t y, const uint8_t* data, int16_t w, int16_t h, uint16_t fg, uint16_t bg);
  bool getTouch(uint16_t* x, uint16_t* y, uint16_t = 600);
  void setTouch(uint16_t*) {}
  void diagnostics();
  void touchDiagnostics();
  void resetTouch() { initTouch(); }
  void dumpFrame();
  void drawHeading(String title);
  void showPowerOff();
  void startDiagnostic();
  void diagnosticTick();
  bool diagnosticActive() const { return diagnostic_; }
  void stopDiagnostic() { diagnostic_ = false; }
  void serviceDiagnostic();
  void requestClean() { cleanRequested_ = true; }
  bool ready() const { return initialized_; }
  bool inputFrameReady() const {
    return initialized_ && !busy_.load(std::memory_order_acquire) && !dirty_;
  }
 private:
  uint8_t *canvas_ = nullptr, *snapshot_ = nullptr, *previous_ = nullptr;
  bool initialized_ = false, dirty_ = false, cleanRequested_ = true;
  bool diagnostic_ = false, diagnosticDown_ = false, diagnosticInk_ = false;
  std::atomic<bool> busy_{false};
  bool panelValid_ = false, lastOk_ = true, jobFull_ = true;
  uint32_t lastSubmit_ = 0, lastDraw_ = 0, lastTouchPoll_ = 0;
  uint32_t refreshes_ = 0, partials_ = 0, errors_ = 0, lastRefreshMs_ = 0;
  uint8_t partialCount_ = 0, touchAddress_ = 0;
  uint16_t controllerVcom_ = 0;
  uint16_t touchMaxX_ = 540, touchMaxY_ = 960;
  uint32_t touchPolls_ = 0, touchReady_ = 0, touchErrors_ = 0, touchRejected_ = 0;
  uint16_t rawTouchX_ = 0, rawTouchY_ = 0;
  uint8_t lastTouchStatus_ = 0;
  sticky::Area jobArea_{};
  sticky::TouchState touch_{};
  TaskHandle_t worker_ = nullptr;
  TwoWire touchBus_{1};
  static void workerEntry(void* argument);
  bool refresh();
  uint8_t* transfer_ = nullptr;
  bool touchRead(uint16_t reg, uint8_t* data, size_t len);
  bool touchWrite(uint16_t reg, uint8_t value);
  void initTouch();
  static bool ink(uint16_t color) { return color != TFT_BLACK; }
};

class M5PaperButton {
 public:
  void initButton(M5PaperDisplay *gfx, int16_t x, int16_t y, uint16_t w, uint16_t h,
                  uint16_t outline, uint16_t fill, uint16_t textcolor,
                  char *label, uint8_t textsize);
  void setLabelDatum(int16_t x_delta, int16_t y_delta, uint8_t datum = MC_DATUM);
  void drawButton(bool inverted = false, String long_name = "");
  bool contains(int16_t x, int16_t y) const;
  void press(bool pressed);
  bool isPressed() const { return currstate_; }
  bool justReleased() const { return !currstate_ && laststate_; }

 private:
  M5PaperDisplay *gfx_ = nullptr;
  int16_t x_ = 0, y_ = 0, w_ = 0, h_ = 0;
  int16_t label_dx_ = 0, label_dy_ = 0;
  uint16_t outline_ = TFT_WHITE, fill_ = TFT_BLACK, text_ = TFT_WHITE;
  uint8_t text_size_ = 1;
  uint8_t datum_ = MC_DATUM;
  String label_;
  bool currstate_ = false, laststate_ = false;
};

using TFT_eSPI = M5PaperDisplay;
using TFT_eSPI_Button = M5PaperButton;
