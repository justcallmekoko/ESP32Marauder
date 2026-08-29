#include "BootSplash.h"

namespace marauder {

BootSplashLayout bootSplashLayout(int16_t screen_width,
                                  int16_t screen_height) {
  const bool compact = screen_height <= 160 || screen_width <= 160;
  const bool v8_portrait = screen_width == 240 && screen_height == 320;
  BootSplashLayout layout{};
  layout.text_size = compact || v8_portrait ? 1 : 2;
  layout.title_y = compact ? 4 : screen_height / 16;
  layout.status_y = screen_height - (compact ? 12 : 24);
  layout.version_y = layout.status_y - (compact ? 14 : 24);

  layout.logo_y = layout.title_y + (compact ? 16 : 28);
  const int16_t logo_bottom = layout.version_y - (compact ? 4 : 10);
  const int16_t available_height = logo_bottom - layout.logo_y;
  const int16_t maximum_width = screen_width * (compact ? 55 : 68) / 100;
  const int16_t height_from_width = maximum_width * 316 / 240;

  layout.logo_height = available_height < height_from_width
      ? available_height : height_from_width;
  if (layout.logo_height < 24) layout.logo_height = 24;
  if (v8_portrait) {
    layout.logo_height /= 2;
    layout.logo_y += (available_height - layout.logo_height) / 2;
  }
  layout.logo_width = layout.logo_height * 240 / 316;
  layout.logo_x = (screen_width - layout.logo_width) / 2;
  return layout;
}

}  // namespace marauder
