
#pragma once

#ifndef Led_h
#define Led_h

#include "configs.h"

// HAS_FLIPPER_LED XIAO_ESP32_S3 HAS_XIAO_LED HAS_STICKC_LED HAS_NEOPIXEL_LED

#ifdef HAS_FLIPPER_LED
  #include "flipperLED.h"
  extern flipperLED led_obj;
#elif defined(HAS_XIAO_LED) || defined(XIAO_ESP32_S3)
  #include "xiaoLED.h"
  extern xiaoLED led_obj;
#elif defined(HAS_STICKC_LED) || defined(MARAUDER_M5STICKC) || defined(MARAUDER_M5STICKCP2)
  #include "stickcLED.h"
  extern stickcLED led_obj;
#elif defined(HAS_NEOPIXEL_LED)
  #include "LedInterface.h"
  extern LedInterface led_obj;
#endif


#endif  // Led_h




