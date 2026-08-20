#pragma once

#ifndef LedInterface_h
#define LedInterface_h

#include "configs.h"
#include "settings.h"
#include <Arduino.h>
#ifdef HAS_NEOPIXEL_LED
  #include <Adafruit_NeoPixel.h>
#endif
#ifdef HAS_T_DONGLE_LED
  #include <APA102.h>
#endif

#define Pixels 1

extern Settings settings_obj;

#ifdef HAS_NEOPIXEL_LED
  extern Adafruit_NeoPixel strip;
#endif

class LedInterface {

  private:
    uint32_t initTime = 0;

    #ifdef HAS_T_DONGLE_LED
      uint8_t last_t_dongle_mode = 0xFF;
      APA102<T_DONGLE_LED_DATA_PIN, T_DONGLE_LED_CLOCK_PIN> t_dongle_led;
    #endif

    int current_fade_itter = 1;
    int wheel_pos = 255;
    int wheel_speed = 1; // lower = slower

    uint32_t Wheel(byte WheelPos);

    uint8_t current_mode = MODE_OFF;

    void rainbow();
    void ledOff();
    void attackLed();
    void sniffLed();

    #ifdef HAS_T_DONGLE_LED
      void writeApa102Color(uint8_t red, uint8_t green, uint8_t blue);
    #endif
  
  public:
    LedInterface();

    void RunSetup();
    void main(uint32_t currentTime);

    #ifdef HAS_T_DONGLE_LED
      void refresh();
    #endif

    void setMode(uint8_t);
    void setColor(int r, int g, int b);
    uint8_t getMode();
    
  
};

#endif
