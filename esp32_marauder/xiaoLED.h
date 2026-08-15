#pragma once


#include "configs.h"

#if  defined(HAS_XIAO_LED) || defined(XIAO_ESP32_S3)

#ifndef xiaoLED_H
#define xiaoLED_H

#include "settings.h"

#include <Arduino.h>

#define XIAO_LED_PIN 21

extern Settings settings_obj;

class xiaoLED {

    public:
        void RunSetup();
        void main(uint32_t currentTime);
        void attackLED();
        void sniffLED();
        void offLED();
};

#endif  /* xiaoLED_H */

#endif  // HAS_XIAO_LED
