#pragma once

#ifndef BatteryInterface_h
#define BatteryInterface_h

#include <Arduino.h>

#include "configs.h"
#include "Adafruit_MAX1704X.h"

#ifdef HAS_AXP2101
  #define XPOWERS_CHIP_AXP2101
  #include "XPowersLib.h"
#endif

#include <Wire.h>

#define IP5306_ADDR 0x75
#define MAX17048_ADDR 0x36

class BatteryInterface {
  private:
    uint32_t initTime = 0;
    Adafruit_MAX17048 maxlipo;

    #ifdef HAS_AXP2101
      XPowersPMU power;
    #endif

  public:
    int8_t battery_level = 0;
    int8_t old_level = 0;
    bool i2c_supported = false;
    bool has_max17048 = false;
    bool has_ip5306 = false;
    bool has_axp2101 = false;

    BatteryInterface();

    void RunSetup();
    void main(uint32_t currentTime);
    int8_t getBatteryLevel();
};

#endif
