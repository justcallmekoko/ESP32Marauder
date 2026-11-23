#pragma once
#include <stdint.h>

#define TAB_COLOUR 0
#define TFT_MADCTL 0x36
#define TFT_MAD_MX 0x40
#define TFT_MAD_MV 0x20
#define TFT_MAD_MY 0x80
#define TFT_MAD_COLOR_ORDER 0x08

// Minimal register observer for the real XIAO display adapter. The base init
// deliberately invokes a nonvirtual rotation method, like TFT_eSPI 2.5.34.
class TFT_eSPI {
 public:
  uint8_t command = 0;
  uint8_t data = 0;
  unsigned init_count = 0;

  void init(uint8_t = TAB_COLOUR) {
    ++init_count;
    setRotation(rotation_);
  }
  void setRotation(uint8_t rotation) {
    rotation_ = rotation & 3;
    writecommand(TFT_MADCTL);
    writedata(0);  // The uncorrected base driver overwrites the register.
  }
  uint8_t getRotation() { return rotation_; }
  void writecommand(uint8_t value) { command = value; }
  void writedata(uint8_t value) { data = value; }

 private:
  uint8_t rotation_ = 0;
};
