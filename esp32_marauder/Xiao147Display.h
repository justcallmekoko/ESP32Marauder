#pragma once
#include <TFT_eSPI.h>

// Keep the saved JD9853A mirror correction local to this board. Every screen
// rotation must apply it, including landscape graphs and returning to menus.
class Xiao147Display : public TFT_eSPI {
 public:
  void setRotation(uint8_t rotation) {
    TFT_eSPI::setRotation(rotation);
    static const uint8_t orientation[] = {
      TFT_MAD_MX,
      TFT_MAD_MV,
      TFT_MAD_MY,
      TFT_MAD_MX | TFT_MAD_MV | TFT_MAD_MY
    };
    writecommand(TFT_MADCTL);
    writedata(orientation[rotation & 3] | TFT_MAD_COLOR_ORDER);
  }
};
