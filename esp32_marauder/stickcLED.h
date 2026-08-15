#pragma once

#ifndef stickcLED_H
#define stickcLED_H

#include "configs.h"

#if defined(MARAUDER_M5STICKC) || defined(MARAUDER_M5STICKCP2)

#include "settings.h"

#include <Arduino.h>


extern Settings settings_obj;

class stickcLED {

    public:
      void RunSetup();
      void main(uint32_t currentTime);
      void attackLED();
      void sniffLED();
      void offLED();

};

#endif
#endif  /* stickcLED_H */
