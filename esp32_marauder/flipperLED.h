#pragma once


#ifndef flipperLED_h
#define flipperLED_h


#include "configs.h"

#ifdef HAS_FLIPPER_LED
#include "settings.h"

#include <Arduino.h>

extern Settings settings_obj;

class flipperLED {

  public:
    void RunSetup();
    void main(uint32_t currentTime);
    void attackLED();
    void sniffLED();
    void offLED();

};

#endif  //  HAS_FLIPPER_LED

#endif
