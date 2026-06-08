#pragma once

#ifdef HAS_BATTERY

#ifndef BatteryInterface_h
#define BatteryInterface_h

#include <Arduino.h>

#include "configs.h"

#ifdef HAS_MAX1704X
  #include "Adafruit_MAX1704X.h"
#endif

#ifdef HAS_AXP2101
  #define XPOWERS_CHIP_AXP2101
  #include "XPowersLib.h"
#endif


#ifndef BATTERY_ADC_PIN  // not 12c
  #include <Wire.h>
#endif

#define IP5306_ADDR 0x75
#define MAX17048_ADDR 0x36

class BatteryInterface {
  private:
    uint32_t initTime = 0;

    #ifdef HAS_MAX1704X
      Adafruit_MAX17048 maxlipo;
    #endif

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
    bool has_adc_battery = false;

    BatteryInterface();

    void RunSetup();
    void main(uint32_t currentTime);
    int8_t getBatteryLevel();
};

#endif  // ifndef BatteryInterface_h

#endif // HAS_BATTERY

