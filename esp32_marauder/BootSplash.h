#pragma once

#include <stdint.h>

namespace marauder {

struct BootSplashLayout {
  int16_t title_y;
  int16_t logo_x;
  int16_t logo_y;
  int16_t logo_width;
  int16_t logo_height;
  int16_t version_y;
  int16_t status_y;
  uint8_t text_size;
};

BootSplashLayout bootSplashLayout(int16_t screen_width,
                                  int16_t screen_height);

}  // namespace marauder
