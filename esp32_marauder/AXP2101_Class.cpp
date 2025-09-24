// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "AXP2101_Class.hpp"

#if __has_include(<esp_log.h>)
#include <esp_log.h>
#endif

#include <algorithm>

#define IS_BIT_SET(val,mask)            (((val)&(mask)) == (mask))

namespace m5
{
/*
  DCDC1 : 1.5-3.4V，                      2000mA
  DCDC2 : 0.5-1.2V，1.22-1.54V,           2000mA
  DCDC3 : 0.5-1.2V，1.22-1.54V, 1.6-3.4V, 2000mA
  DCDC4 : 0.5-1.2V, 1.22-1.84V,           1500mA
  DCDC5 : 1.2V    , 1.4-3.7V,             1000mA

  RTCLDO1/2 : 1.8V/2.5V/3V/3.3V,            30mA

  ALDO1~4 : 0.5-3.5V, 100mV/step            300mA
*/
  bool AXP2101_Class::begin(void)
  {
    std::uint8_t val;
    _init = readRegister(0x03, &val, 1);
    if (_init)
    {
      _init = (val == 0x4A);
#if defined (ESP_LOGV)
      // ESP_LOGV("AXP2101", "reg03h:%02x : init:%d", val, _init);
#endif
    }
    return _init;
  }

  // 0=ALDO1 ~ 3=ALDO4 / 4=BLDO1 / 5=BLDO2
  void AXP2101_Class::_set_LDO(std::uint8_t num, int voltage)
  {
    if (num > 5) return;
    std::uint8_t reg_volt = num + 0x92;
    voltage -= 500;
    /// convert voltage to value
    std::uint_fast8_t val = (voltage < 0) ? 0 : std::min(voltage / 100, 0x1E);
    writeRegister8(reg_volt, val);

    std::uint_fast8_t reg90bit = 1 << num;
    if (voltage < 0)
    {
      bitOff(0x90, reg90bit);
    }
    else
    {
      bitOn(0x90, reg90bit);
    }
  }

  void AXP2101_Class::_set_DLDO(std::uint8_t num, int voltage)
  {
    if (num > 1) return;

    std::uint8_t reg_volt = num + 0x99;
    voltage -= 500;
    /// convert voltage to value
    std::uint_fast8_t val = (voltage < 0) ? 0 : std::min(voltage / (num ? 50 : 100), num ? 0x13 : 0x1C);
    writeRegister8(reg_volt, val);

    uint8_t reg = 0x90 + num;
    uint8_t bit = num ? 0x01 : 0x80;
    if (voltage < 0)
    {
      bitOff(reg, bit);
    }
    else
    {
      bitOn(reg, bit);
    }
  }

  bool AXP2101_Class::_get_LDOEn(std::uint8_t num)
  {
    bool res = false;
    if (num <= 5) {
      std::uint_fast8_t reg90bit = 1 << num;
      res = readRegister8(0x90) & reg90bit;
    }
    return res;
  }

  void AXP2101_Class::setBatteryCharge(bool enable)
  {
    std::uint8_t val = 0;
    if (readRegister(0x18, &val, 1))
    {
      writeRegister8(0x18, (val & 0xFD) | (enable << 1));
    }
  }

  void AXP2101_Class::setPreChargeCurrent(std::uint16_t max_mA)
  {
    static constexpr std::uint8_t table[] = { 0, 25, 50, 75, 100, 125, 150, 175, 200, 255 };
    if (max_mA > 200) { max_mA = 200; }

    size_t i = 0;
    while (table[i] <= max_mA) { ++i; }
    i -= 1;
    writeRegister8(0x61, i); 
  }

  void AXP2101_Class::setChargeCurrent(std::uint16_t max_mA)
  {
    max_mA /= 5;
    if (max_mA > 1000/5) { max_mA = 1000/5; }
    static constexpr std::uint8_t table[] = { 125 / 5, 150 / 5, 175 / 5, 200 / 5, 300 / 5, 400 / 5, 500 / 5, 600 / 5, 700 / 5, 800 / 5, 900 / 5, 1000 / 5, 255 };

    size_t i = 0;
    while (table[i] <= max_mA) { ++i; }
    i += 4;
    writeRegister8(0x62, i);
  }

  void AXP2101_Class::setChargeVoltage(std::uint16_t max_mV)
  {
    max_mV = (max_mV / 10) - 400;
    if (max_mV > 460 - 400) { max_mV = 460 - 400; }
    static constexpr std::uint8_t table[] =
      { 410 - 400  /// 4100mV
      , 420 - 400  /// 4200mV
      , 435 - 400  /// 4350mV
      , 440 - 400  /// 4400mV
      , 460 - 400  /// 4600mV
      , 255
      };
    size_t i = 0;
    while (table[i] <= max_mV) { ++i; }

    if (++i >= 0b110) { i = 0; }
    writeRegister8(0x64, i);
  }

  std::int8_t AXP2101_Class::getBatteryLevel(void)
  {
    std::int8_t res = readRegister8(0xA4);
    return res;
  }

  /// @return -1:discharge / 0:standby / 1:charge
  int AXP2101_Class::getChargeStatus(void)
  {
    uint32_t val = (readRegister8(0x01) >> 5) & 0b11;
    // 0b01:charge / 0b10:dischage / 0b00:stanby
    return (val == 1) ? 1 : ((val == 2) ? -1 : 0);
  }

  bool AXP2101_Class::isCharging(void)
  {
    return (readRegister8(0x01) & 0b01100000) == 0b00100000;
  }

  void AXP2101_Class::powerOff(void)
  {
    bitOn(0x10, 0x01);
  }

  //enable all ADC channel control or set default values
  void AXP2101_Class::setAdcState(bool enable)
  {
    writeRegister8(0x30, enable == true ? 0b111111 : 0b11);
  }

  void AXP2101_Class::setAdcRate( std::uint8_t rate )
  {
  }

  void AXP2101_Class::setBACKUP(bool enable)
  {
/*
    static constexpr std::uint8_t add = 0x35;
    static constexpr std::uint8_t bit = 1 << 7;
    if (enable)
    { // Enable
      bitOn(add, bit);
    }
    else
    { // Disable
      bitOff(add, bit);
    }
//*/
  }

  bool AXP2101_Class::isACIN(void)
  {
return false;
  }
  bool AXP2101_Class::isVBUS(void)
  { // VBUS good indication
    return readRegister8(0x00) & 0x20;
  }

  bool AXP2101_Class::getBatState(void)
  { // Battery present state
    return readRegister8(0x00) & 0x08;
  }

  std::uint8_t AXP2101_Class::getPekPress(void)
  {
    std::uint8_t val = readRegister8(0x49) & 0x0C;
    if (val) { writeRegister8(0x49, val); }
    return val >> 2;
  }

  float AXP2101_Class::getACINVoltage(void)
  {
return 0;
  }

  float AXP2101_Class::getACINCurrent(void)
  {
return 0;
  }

  float AXP2101_Class::getVBUSVoltage(void)
  {
    if (isVBUS() == false) { return 0.0f; }
    
    float vBus = readRegister14(0x38);
    if (vBus >= 16375) { return 0.0f; }

    return vBus / 1000.0f;
  }

  float AXP2101_Class::getVBUSCurrent(void)
  {
return 0;
  }

  float AXP2101_Class::getTSVoltage(void)
  {
    float volt = readRegister14(0x36);
    if (volt >= 16375) { return 0.0f; }

    return volt / 2000.0f;
  }

  float AXP2101_Class::getInternalTemperature(void)
  {
    return 22 + ((7274 - readRegister16(0x3C)) / 20);
  }

  float AXP2101_Class::getBatteryPower(void)
  {
return 0;
  }

  float AXP2101_Class::getBatteryVoltage(void)
  {
    return readRegister14(0x34) / 1000.0f;
  }

  float AXP2101_Class::getBatteryChargeCurrent(void)
  {
return 0;
  }

  float AXP2101_Class::getBatteryDischargeCurrent(void)
  {
return 0;
  }

  float AXP2101_Class::getAPSVoltage(void)
  {
return 0;
  }

  bool AXP2101_Class::enableIRQ(std::uint64_t registerEn)
  {
    return setIRQEnRegister(registerEn, true);
  }

  bool AXP2101_Class::disableIRQ(std::uint64_t registerEn)
  {
    return setIRQEnRegister(registerEn, false);
  }

  bool AXP2101_Class::setIRQEnRegister(std::uint64_t registerEn, bool enable)
  {
    int res = 0;
    uint8_t data = 0, value = 0;
    if (registerEn & 0x0000FF) 
    {
      value = registerEn & 0xFF;
      data = readRegister8(AXP2101_IRQEN0);
      intRegister[0] =  enable ? (data | value) : (data & (~value));
      res |= writeRegister8(AXP2101_IRQEN0, intRegister[0]);
    }
    if (registerEn & 0x00FF00) 
    {
      value = registerEn >> 8;
      data = readRegister8(AXP2101_IRQEN1);
      intRegister[1] =  enable ? (data | value) : (data & (~value));
      res |= writeRegister8(AXP2101_IRQEN1, intRegister[1]);
    }
    if (registerEn & 0xFF0000) 
    {
      value = registerEn >> 16;
      data = readRegister8(AXP2101_IRQEN2);
      intRegister[2] =  enable ? (data | value) : (data & (~value));
      res |= writeRegister8(AXP2101_IRQEN2, intRegister[2]);
    }
    return res == 0;
  }

  std::uint64_t AXP2101_Class::getIRQStatuses(void)
  {
    statusRegister[0] = readRegister8(AXP2101_IRQSTAT0);
    statusRegister[1] = readRegister8(AXP2101_IRQSTAT1);
    statusRegister[2] = readRegister8(AXP2101_IRQSTAT2);
    return (uint32_t)(statusRegister[0] << 16) | (uint32_t)(statusRegister[1] << 8) | (uint32_t)(statusRegister[2]);
  }

  void AXP2101_Class::clearIRQStatuses()
  {
    for (int i = 0; i < AXP2101_IRQSTAT_CNT; i++) 
    {
      writeRegister8(AXP2101_IRQSTAT0 + i, 0xFF);
      statusRegister[i] = 0;
    }
  }

  bool AXP2101_Class::isDropWarningLevel2Irq(void)
  {
      uint8_t mask = AXP2101_IRQ_WARNING_LEVEL2;
      if (intRegister[0] & mask) 
      {
          return IS_BIT_SET(statusRegister[0], mask);
      }
      return false;
  }

  bool AXP2101_Class::isDropWarningLevel1Irq(void)
  {
      uint8_t mask = AXP2101_IRQ_WARNING_LEVEL1;
      if (intRegister[0] & mask) 
      {
          return IS_BIT_SET(statusRegister[0], mask);
      }
      return false;
  }

  bool AXP2101_Class::isGaugeWdtTimeoutIrq()
  {
      uint8_t mask = AXP2101_IRQ_GAUGE_WDT_TIMEOUT;
      if (intRegister[0] & mask) 
      {
          return IS_BIT_SET(statusRegister[0], mask);
      }
      return false;
  }

  bool AXP2101_Class::isBatChargerOverTemperatureIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_BAT_CHG_OVER_TEMP;
      if (intRegister[0] & mask) 
      {
          return IS_BIT_SET(statusRegister[0], mask);
      }
      return false;
  }

  bool AXP2101_Class::isBatChargerUnderTemperatureIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_BAT_CHG_UNDER_TEMP;
      if (intRegister[0] & mask) 
      {
          return IS_BIT_SET(statusRegister[0], mask);
      }
      return false;
  }

  bool AXP2101_Class::isBatWorkOverTemperatureIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_BAT_OVER_TEMP;
      if (intRegister[0] & mask) 
      {
          return IS_BIT_SET(statusRegister[0], mask);
      }
      return false;
  }

  bool AXP2101_Class::isBatWorkUnderTemperatureIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_BAT_UNDER_TEMP;
      if (intRegister[0] & mask) 
      {
          return IS_BIT_SET(statusRegister[0], mask);
      }
      return false;
  }

  bool AXP2101_Class::isVbusInsertIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_VBUS_INSERT  >> 8;
      if (intRegister[1] & mask) 
      {
          return IS_BIT_SET(statusRegister[1], mask);
      }
      return false;
  }

  bool AXP2101_Class::isVbusRemoveIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_VBUS_REMOVE  >> 8;
      if (intRegister[1] & mask) 
      {
          return IS_BIT_SET(statusRegister[1], mask);
      }
      return false;
  }

  bool AXP2101_Class::isBatInsertIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_BAT_INSERT  >> 8;
      if (intRegister[1] & mask) 
      {
          return IS_BIT_SET(statusRegister[1], mask);
      }
      return false;
  }

  bool AXP2101_Class::isBatRemoveIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_BAT_REMOVE  >> 8;
      if (intRegister[1] & mask)
      {
          return IS_BIT_SET(statusRegister[1], mask);
      }
      return false;
  }

  bool AXP2101_Class::isPekeyShortPressIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_PKEY_SHORT_PRESS  >> 8;
      if (intRegister[1] & mask)
      {
          return IS_BIT_SET(statusRegister[1], mask);
      }
      return false;

  }

  bool AXP2101_Class::isPekeyLongPressIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_PKEY_LONG_PRESS  >> 8;
      if (intRegister[1] & mask)
      {
          return IS_BIT_SET(statusRegister[1], mask);
      }
      return false;
  }

  bool AXP2101_Class::isPekeyNegativeIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_PKEY_NEGATIVE_EDGE  >> 8;
      if (intRegister[1] & mask)
      {
          return IS_BIT_SET(statusRegister[1], mask);
      }
      return false;
  }

  bool AXP2101_Class::isPekeyPositiveIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_PKEY_POSITIVE_EDGE  >> 8;
      if (intRegister[1] & mask)
      {
          return IS_BIT_SET(statusRegister[1], mask);
      }
      return false;
  }

  bool AXP2101_Class::isWdtExpireIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_WDT_EXPIRE  >> 16;
      if (intRegister[2] & mask)
      {
          return IS_BIT_SET(statusRegister[2], mask);
      }
      return false;
  }

  bool AXP2101_Class::isLdoOverCurrentIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_LDO_OVER_CURR  >> 16;
      if (intRegister[2] & mask)
      {
          return IS_BIT_SET(statusRegister[2], mask);
      }
      return false;
  }

  bool AXP2101_Class::isBatfetOverCurrentIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_BATFET_OVER_CURR  >> 16;
      if (intRegister[2] & mask)
      {
          return IS_BIT_SET(statusRegister[2], mask);
      }
      return false;
  }

  bool AXP2101_Class::isBatChagerDoneIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_BAT_CHG_DONE  >> 16;
      if (intRegister[2] & mask)
      {
          return IS_BIT_SET(statusRegister[2], mask);
      }
      return false;
  }

  bool AXP2101_Class::isBatChagerStartIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_BAT_CHG_START  >> 16;
      if (intRegister[2] & mask)
      {
          return IS_BIT_SET(statusRegister[2], mask);
      }
      return false;
  }

  bool AXP2101_Class::isBatDieOverTemperatureIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_DIE_OVER_TEMP  >> 16;
      if (intRegister[2] & mask)
      {
          return IS_BIT_SET(statusRegister[2], mask);
      }
      return false;
  }

  bool AXP2101_Class::isChagerOverTimeoutIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_CHAGER_TIMER  >> 16;
      if (intRegister[2] & mask)
      {
          return IS_BIT_SET(statusRegister[2], mask);
      }
      return false;
  }

  bool AXP2101_Class::isBatOverVoltageIrq(void)
  {
      uint8_t mask = AXP2101_IRQ_BAT_OVER_VOLTAGE  >> 16;
      if (intRegister[2] & mask)
      {
          return IS_BIT_SET(statusRegister[2], mask);
      }
      return false;
  }

  std::size_t AXP2101_Class::readRegister12(std::uint8_t addr)
  {
    std::uint8_t buf[2] = {0};
    readRegister(addr, buf, 2);
    return (buf[0] & 0x0F) << 8 | buf[1];
  }
  std::size_t AXP2101_Class::readRegister14(std::uint8_t addr)
  {
    std::uint8_t buf[2] = {0};
    readRegister(addr, buf, 2);
    return (buf[0] & 0x3F) << 8 | buf[1];
  }
  std::size_t AXP2101_Class::readRegister16(std::uint8_t addr)
  {
    std::uint8_t buf[2] = {0};
    readRegister(addr, buf, 2);
    return buf[0] << 8 | buf[1];
  }

}
