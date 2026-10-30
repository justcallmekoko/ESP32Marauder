#pragma once

// #ifdef HAS_BATTERY

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

#ifdef HAS_AXP192
  #include "AXP192.h"
  AXP192 axp192_obj;
#endif

#ifndef BATTERY_ADC_PIN  // not 12c
  #include <Wire.h>
#endif

#define IP5306_ADDR 0x75

// AXP2101 PMU read directly over I2C (no XPowersLib). Register map per the
// AXP2101 datasheet / XPowersLib: chip ID 0x03 == 0x4A, STATUS1 0x00 bit 3 =
// battery present, 0x18 bit 3 = fuel gauge enable, 0xA4 = battery percent.
#define AXP2101_GAUGE_ADDR      0x34
#define AXP2101_REG_STATUS1     0x00
#define AXP2101_REG_CHIP_ID     0x03
#define AXP2101_CHIP_ID         0x4A
#define AXP2101_REG_GAUGE_CTRL  0x18
#define AXP2101_REG_BAT_PERCENT 0xA4
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
    bool supported = false;     // is not all i2c
    bool has_max17048 = false;
    bool has_ip5306 = false;
    bool has_axp2101_gauge = false;
    bool has_axp2101 = false;
    bool has_adc_battery = false;

    BatteryInterface();

    void RunSetup();
    void main(uint32_t currentTime);
    int8_t getBatteryLevel();
    #ifdef HAS_AXP2101_GAUGE
      bool detectAxp2101Gauge();
      int axp2101Read(uint8_t reg);
    #endif
};

#endif  // ifndef BatteryInterface_h

// #endif // HAS_BATTERY

