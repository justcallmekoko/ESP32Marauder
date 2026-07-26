#pragma once

#include "configs.h"

#if defined(CYD_2432S022)
  #include "LGFX_ESP32_2432S022.h"

  using MarauderTFT = LGFX_ESP32_2432S022;

  class MarauderButton : public LGFX_Button
  {
  public:
    using LGFX_Button::drawButton;
    using LGFX_Button::initButton;

    template <typename T1, typename T2, typename T3>
    void initButton(lgfx::LovyanGFX *gfx, int16_t x, int16_t y, uint16_t w, uint16_t h,
                    const T1& outline, const T2& fill, const T3& textcolor,
                    const char *label, float textsize_x = 1.0f, float textsize_y = 0.0f)
    {
      LGFX_Button::initButton(gfx, x, y, w, h,
                              static_cast<uint32_t>(outline),
                              static_cast<uint32_t>(fill),
                              static_cast<uint32_t>(textcolor),
                              label, textsize_x, textsize_y);
    }

    void drawButton(bool inverted, const String& long_name)
    {
      LGFX_Button::drawButton(inverted, long_name.c_str());
    }
  };
#else
  #include <TFT_eSPI.h>

  using MarauderTFT = TFT_eSPI;
  using MarauderButton = TFT_eSPI_Button;
#endif
