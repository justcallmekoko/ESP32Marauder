#pragma once

#ifdef MARAUDER_POOM

#include <Arduino.h>
#include "esp32-hal-rmt.h"

class PoomWs2812 {
 public:
  PoomWs2812(uint16_t count, uint8_t pin);

  bool begin();
  void setBrightness(uint8_t brightness);
  uint32_t Color(uint8_t red, uint8_t green, uint8_t blue) const;
  void setPixelColor(uint16_t pixel, uint32_t color);
  void show();

 private:
  static constexpr uint16_t kMaxPixels = 9;
  static constexpr uint16_t kBytesPerPixel = 3;
  static constexpr uint16_t kBitsPerByte = 8;

  uint16_t count_;
  uint8_t pin_;
  uint8_t brightness_ = 255;
  bool ready_ = false;
  uint8_t pixels_[kMaxPixels][kBytesPerPixel] = {};
  rmt_data_t symbols_[kMaxPixels * kBytesPerPixel * kBitsPerByte] = {};
};

#endif
