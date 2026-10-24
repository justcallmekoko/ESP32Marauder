
#pragma once

#include <Wire.h>

#ifdef MARAUDER_M5STACK_CORE2_AWS

  inline void core2AxpWrite(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(0x34);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
  }

  inline uint8_t core2AxpRead(uint8_t reg) {
    Wire.beginTransmission(0x34);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(0x34, 1);
    return Wire.available() ? Wire.read() : 0;
  }

  inline void core2SetBacklight(uint8_t duty) {
    uint8_t outputs = core2AxpRead(0x12);
    if (duty == 0) {
      // Core2 drives the LCD backlight from DCDC3 (bit 1), not LDO2.
      core2AxpWrite(0x12, outputs & ~(1 << 1));
      return;
    }

    const uint16_t millivolts = map(duty, 1, 255, 2500, 3300);
    const uint8_t step = (millivolts - 700) / 25;

    core2AxpWrite(0x27, (core2AxpRead(0x27) & 0x80) | (step & 0x7f));
    core2AxpWrite(0x12, outputs | (1 << 1));
  }

  inline void core2PowerInit() {
    Wire.begin(I2C_SDA, I2C_SCL, 400000U);
    // LDO2 supplies LCD logic/SD at 3.3 V. LDO3 is the vibration motor and
    // must remain disabled during normal operation.
    core2AxpWrite(0x28, 0xf2);
    // DCDC3 supplies the Core2 LCD backlight; start at M5Stack's 2.8 V.
    core2AxpWrite(0x27, (core2AxpRead(0x27) & 0x80) | 0x54);
    core2AxpWrite(0x82, 0xff); // battery and power ADCs
    // DCDC1 (ESP32), DCDC3 (backlight), and LDO2 (LCD logic/SD) on;
    // explicitly force LDO3 (vibration) off.
    core2AxpWrite(0x12, (core2AxpRead(0x12) & ~(1 << 3)) | 0x07);

    // GPIO4 resets both the LCD and touch controller on Core2.
    core2AxpWrite(0x95, (core2AxpRead(0x95) & 0x72) | 0x84);
    core2AxpWrite(0x96, core2AxpRead(0x96) & ~0x02);
    delay(100);
    core2AxpWrite(0x96, core2AxpRead(0x96) | 0x02);
    delay(100);
  }

#endif   //  MARAUDER_M5STACK_CORE2_AWS
