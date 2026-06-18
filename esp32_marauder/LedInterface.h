#pragma once

#ifndef LedInterface_h
#define LedInterface_h

#include "configs.h"

#ifdef HAS_NEOPIXEL_LED

#include "settings.h"
#include <Arduino.h>

#include <Adafruit_NeoPixel.h>

#define Pixels 1

extern Settings settings_obj;

// #ifdef HAS_NEOPIXEL_LED
//   extern Adafruit_NeoPixel strip;
// #endif

class LedInterface {


  private:
    uint32_t initTime = 0;

    int current_fade_itter = 1;
    int wheel_pos = 255;
    int wheel_speed = 1; // lower = slower

    uint32_t Wheel(byte WheelPos);

    uint8_t current_mode = MODE_OFF;


    Adafruit_NeoPixel strip = Adafruit_NeoPixel(Pixels, PIN, NEO_GRB + NEO_KHZ800);
  
  public:
    LedInterface();

    void RunSetup();
    void main(uint32_t currentTime);
    void attackLED();
    void sniffLED();
    void offLED();
    void rainbow();



    void setMode(uint8_t);
    void setColor(int r, int g, int b);
    uint8_t getMode();
    
};

#endif    // HAS_NEOPIXEL_LED
#endif
