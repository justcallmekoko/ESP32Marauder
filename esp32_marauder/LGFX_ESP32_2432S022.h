#pragma once

#define LGFX_USE_V1

#include <LovyanGFX.hpp>

class LGFX_ESP32_2432S022 : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7789 panel_instance_;
  lgfx::Bus_Parallel8 bus_instance_;
  lgfx::Light_PWM light_instance_;

  #if defined(CYD_2432S022C_TOUCH)
    lgfx::Touch_CST816S touch_instance_;
  #endif

public:
  using lgfx::LGFX_Device::drawXBitmap;
  using lgfx::LGFX_Device::setTextColor;

  template <typename T1, typename T2>
  void setTextColor(T1 fgcolor, T2 bgcolor, bool)
  {
    lgfx::LGFX_Device::setTextColor(fgcolor, bgcolor);
  }

  template <typename T1, typename T2>
  void drawXBitmap(int32_t x, int32_t y, const uint8_t* bitmap, int32_t w, int32_t h,
                   const T1& fgcolor, const T2& bgcolor)
  {
    lgfx::LGFX_Device::drawXBitmap(x, y, bitmap, w, h,
                                   static_cast<uint32_t>(fgcolor),
                                   static_cast<uint32_t>(bgcolor));
  }

  LGFX_ESP32_2432S022(void)
  {
    {
      auto cfg = bus_instance_.config();

      cfg.i2s_port = I2S_NUM_0;
      cfg.freq_write = 25000000;
      cfg.pin_wr = 4;    // WR, openHASP #606 / TFT_eSPI #3281 / Sunton JSON.
      cfg.pin_rd = 2;    // RD, openHASP #606 / TFT_eSPI #3281 / Sunton JSON.
      cfg.pin_rs = 16;   // DC/RS, openHASP #606 / TFT_eSPI #3281 / Sunton JSON.
      cfg.pin_d0 = 15;   // D0, openHASP #606 / TFT_eSPI #3281 / Sunton JSON.
      cfg.pin_d1 = 13;   // D1, openHASP #606 / TFT_eSPI #3281 / Sunton JSON.
      cfg.pin_d2 = 12;   // D2, openHASP #606 / TFT_eSPI #3281 / Sunton JSON.
      cfg.pin_d3 = 14;   // D3, openHASP #606 / TFT_eSPI #3281 / Sunton JSON.
      cfg.pin_d4 = 27;   // D4, openHASP #606 / TFT_eSPI #3281 / Sunton JSON.
      cfg.pin_d5 = 25;   // D5, openHASP #606 / TFT_eSPI #3281 / Sunton JSON.
      cfg.pin_d6 = 33;   // D6, openHASP #606 / TFT_eSPI #3281 / Sunton JSON.
      cfg.pin_d7 = 32;   // D7, openHASP #606 / TFT_eSPI #3281 / Sunton JSON.

      bus_instance_.config(cfg);
      panel_instance_.setBus(&bus_instance_);
    }

    {
      auto cfg = panel_instance_.config();

      cfg.pin_cs = 17;        // CS, openHASP #606 / TFT_eSPI #3281 / Sunton JSON.
      cfg.pin_rst = -1;       // RST not connected, openHASP #606 / Sunton JSON.
      cfg.pin_busy = -1;
      cfg.panel_width = 240;  // ESP32-2432S022 panel size.
      cfg.panel_height = 320; // ESP32-2432S022 panel size.
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = CYD_2432S022_OFFSET_ROTATION;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = false;
      cfg.invert = false;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;

      panel_instance_.config(cfg);
    }

    {
      auto cfg = light_instance_.config();

      cfg.pin_bl = 0;        // TFT_BL, TFT_eSPI #3281 / Sunton JSON DISPLAY_BCKL=0.
      cfg.invert = false;   // TFT_eSPI #3281 reports active-high backlight.
      cfg.freq = 44100;
      cfg.pwm_channel = 7;

      light_instance_.config(cfg);
      panel_instance_.setLight(&light_instance_);
    }

    #if defined(CYD_2432S022C_TOUCH)
      {
        auto cfg = touch_instance_.config();

        cfg.i2c_port = 0;
        cfg.i2c_addr = 0x15; // CST816S address, Sunton JSON / openHASP #606.
        cfg.pin_sda = 21;    // TOUCH_SDA, Sunton JSON / openHASP #606.
        cfg.pin_scl = 22;    // TOUCH_SCL, Sunton JSON / openHASP #606.
        cfg.pin_int = -1;    // INT not connected in Sunton JSON.
        cfg.pin_rst = -1;    // RST not connected in Sunton JSON.
        cfg.freq = 400000;
        cfg.x_min = 0;
        cfg.x_max = 239;
        cfg.y_min = 0;
        cfg.y_max = 319;
        cfg.offset_rotation = CYD_2432S022_TOUCH_OFFSET_ROTATION;

        touch_instance_.config(cfg);
        panel_instance_.setTouch(&touch_instance_);
      }
    #endif

    setPanel(&panel_instance_);
  }
};
