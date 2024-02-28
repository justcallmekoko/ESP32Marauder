#pragma once

#ifndef stickcLED_H
#define stickcLED_H

#include "configs.h"
#include "settings.h"

#include <Arduino.h>

#ifdef MARAUDER_M5STICKCP2
    #define STICKC_LED_PIN 19
#else
    #define STICKC_LED_PIN 10
#endif

extern Settings settings_obj;

class stickcLED {

    public:
        void RunSetup();
        void main();
        void attackLED();
        void sniffLED();
        void offLED();
};

#endif  /* stickcLED_H */
