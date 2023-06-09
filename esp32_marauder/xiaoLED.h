#ifndef xiaoLED_H
#define xiaoLED_H

#include "configs.h"
#include "settings.h"

#include <Arduino.h>

#ifdef XIAO_ESP32_S3
    #define XIAO_LED_PIN 21
#endif

extern Settings settings_obj;

class xiaoLED {

    public:
        void RunSetup();
        void attackLED();
        void sniffLED();
        void offLED();
};

#endif  /* xiaoLED_H */