#include "PoomWs2812.h"

#ifdef MARAUDER_POOM

PoomWs2812::PoomWs2812(uint16_t count, uint8_t pin)
    : count_(count > kMaxPixels ? kMaxPixels : count), pin_(pin) {}

bool PoomWs2812::begin() {
  ready_ = rmtInit(pin_, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, 10000000);
  return ready_;
}

void PoomWs2812::setBrightness(uint8_t brightness) {
  brightness_ = brightness;
}

uint32_t PoomWs2812::Color(uint8_t red, uint8_t green, uint8_t blue) const {
  return (static_cast<uint32_t>(red) << 16) |
         (static_cast<uint32_t>(green) << 8) | blue;
}

void PoomWs2812::setPixelColor(uint16_t pixel, uint32_t color) {
  if (pixel >= count_) return;
  pixels_[pixel][0] = static_cast<uint8_t>(color >> 16);
  pixels_[pixel][1] = static_cast<uint8_t>(color >> 8);
  pixels_[pixel][2] = static_cast<uint8_t>(color);
}

void PoomWs2812::show() {
  if (!ready_) return;

  size_t symbol_index = 0;
  for (uint16_t pixel = 0; pixel < count_; ++pixel) {
    // POOM's WS2812 chain uses the standard GRB wire order.
    const uint8_t wire_bytes[] = {
        static_cast<uint8_t>((pixels_[pixel][1] * brightness_) / 255),
        static_cast<uint8_t>((pixels_[pixel][0] * brightness_) / 255),
        static_cast<uint8_t>((pixels_[pixel][2] * brightness_) / 255),
    };
    for (uint8_t value : wire_bytes) {
      for (int bit = 7; bit >= 0; --bit) {
        const bool one = value & (1U << bit);
        symbols_[symbol_index].level0 = 1;
        symbols_[symbol_index].duration0 = one ? 8 : 4;
        symbols_[symbol_index].level1 = 0;
        symbols_[symbol_index].duration1 = one ? 4 : 8;
        ++symbol_index;
      }
    }
  }

  rmtWrite(pin_, symbols_, symbol_index, RMT_WAIT_FOR_EVER);
}

#endif
