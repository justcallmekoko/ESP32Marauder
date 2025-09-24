// Copyright (c) M5Stack. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __M5_AXP2101_CLASS_H__
#define __M5_AXP2101_CLASS_H__

#include "../I2C_Class.hpp"

//IRQ ENABLE REGISTER
#define AXP2101_IRQEN0                           0x40
#define AXP2101_IRQEN1                           0x41
#define AXP2101_IRQEN2                           0x42

//IRQ STATUS REGISTER
#define AXP2101_IRQSTAT0                         0x48
#define AXP2101_IRQSTAT1                         0x49
#define AXP2101_IRQSTAT2                         0x4A
#define AXP2101_IRQSTAT_CNT                       3


namespace m5
{
  typedef enum {
      AXP2101_IRQ_BAT_UNDER_TEMP            = 1 << 0,   // Battery Under Temperature in Work mode IRQ(bwut_irq)
      AXP2101_IRQ_BAT_OVER_TEMP             = 1 << 1,   // Battery Over Temperature in Work mode IRQ(bwot_irq)
      AXP2101_IRQ_BAT_CHG_UNDER_TEMP        = 1 << 2,   // Battery Under Temperature in Charge mode IRQ(bcut_irq)
      AXP2101_IRQ_BAT_CHG_OVER_TEMP         = 1 << 3,   // Battery Over Temperature in Charge mode IRQ(bcot_irq)
      AXP2101_IRQ_GAUGE_NEW_SOC             = 1 << 4,   // Gauge New SOC IRQ(lowsoc_irq)
      AXP2101_IRQ_GAUGE_WDT_TIMEOUT         = 1 << 5,   // Gauge Watchdog Timeout IRQ(gwdt_irq)
      AXP2101_IRQ_WARNING_LEVEL1            = 1 << 6,   // SOC drop to Warning Level1 IRQ(socwl1_irq)
      AXP2101_IRQ_WARNING_LEVEL2            = 1 << 7,   // SOC drop to Warning Level2 IRQ(socwl2_irq)

      // IRQ2 REG 41H
      AXP2101_IRQ_PKEY_POSITIVE_EDGE        = 1 << 8,   // POWERON Positive Edge IRQ(ponpe_irq_en)
      AXP2101_IRQ_PKEY_NEGATIVE_EDGE        = 1 << 9,   // POWERON Negative Edge IRQ(ponne_irq_en)
      AXP2101_IRQ_PKEY_LONG_PRESS           = 1 << 10,  // POWERON Long PRESS IRQ(ponlp_irq)
      AXP2101_IRQ_PKEY_SHORT_PRESS          = 1 << 11,  // POWERON Short PRESS IRQ(ponsp_irq_en)
      AXP2101_IRQ_BAT_REMOVE                = 1 << 12,  // Battery Remove IRQ(bremove_irq)
      AXP2101_IRQ_BAT_INSERT                = 1 << 13,  // Battery Insert IRQ(binsert_irq)
      AXP2101_IRQ_VBUS_REMOVE               = 1 << 14,  // VBUS Remove IRQ(vremove_irq)
      AXP2101_IRQ_VBUS_INSERT               = 1 << 15,  // VBUS Insert IRQ(vinsert_irq)

      // IRQ3 REG 42H
      AXP2101_IRQ_BAT_OVER_VOLTAGE          = 1 << 16,  // Battery Over Voltage Protection IRQ(bovp_irq)
      AXP2101_IRQ_CHAGER_TIMER              = 1 << 17,  // Charger Safety Timer1/2 expire IRQ(chgte_irq)
      AXP2101_IRQ_DIE_OVER_TEMP             = 1 << 18,  // DIE Over Temperature level1 IRQ(dotl1_irq)
      AXP2101_IRQ_BAT_CHG_START             = 1 << 19,  // Charger start IRQ(chgst_irq)
      AXP2101_IRQ_BAT_CHG_DONE              = 1 << 20,  // Battery charge done IRQ(chgdn_irq)
      AXP2101_IRQ_BATFET_OVER_CURR          = 1 << 21,  // BATFET Over Current Protection IRQ(bocp_irq)
      AXP2101_IRQ_LDO_OVER_CURR             = 1 << 22,  // LDO Over Current IRQ(ldooc_irq)
      AXP2101_IRQ_WDT_EXPIRE                = 1 << 23,   // Watchdog Expire IRQ(wdexp_irq)

      // ALL IRQ
      AXP2101_IRQ_ALL                      = (0xFFFFFFFFUL)
  } axp2101_irq_t;
  class AXP2101_Class : public I2C_Device
  {
  public:
    static constexpr uint8_t AXP2101_EFUS_OP_CFG   = 0xF0;
    static constexpr uint8_t AXP2101_EFREQ_CTRL    = 0xF1;
    static constexpr uint8_t AXP2101_TWI_ADDR_EXT  = 0xFF;

    static constexpr std::uint8_t DEFAULT_ADDRESS = 0x34;

    AXP2101_Class(std::uint8_t i2c_addr = DEFAULT_ADDRESS, std::uint32_t freq = 400000, I2C_Class* i2c = &In_I2C)
    : I2C_Device ( i2c_addr, freq, i2c )
    {}

    bool begin(void);

    /// Get the remaining battery power.
    /// @return 0-100 level
    std::int8_t getBatteryLevel(void);

    /// set battery charge enable.
    /// @param enable true=enable / false=disable
    void setBatteryCharge(bool enable);

    /// set battery precharge current
    /// @param max_mA milli ampere. (0 - 200).
    void setPreChargeCurrent(std::uint16_t max_mA);

    /// set battery charge current
    /// @param max_mA milli ampere. (100 - 1320).
    void setChargeCurrent(std::uint16_t max_mA);

    /// set battery charge voltage
    /// @param max_mV milli volt. (4100 - 4360).
    void setChargeVoltage(std::uint16_t max_mV);

    /// @return -1:discharge / 0:standby / 1:charge
    int getChargeStatus(void);

    /// Get whether the battery is currently charging or not.
    bool isCharging(void);


    inline void setALDO1(int voltage) { _set_LDO(0, voltage); }
    inline void setALDO2(int voltage) { _set_LDO(1, voltage); }
    inline void setALDO3(int voltage) { _set_LDO(2, voltage); }
    inline void setALDO4(int voltage) { _set_LDO(3, voltage); }
    inline void setBLDO1(int voltage) { _set_LDO(4, voltage); }
    inline void setBLDO2(int voltage) { _set_LDO(5, voltage); }
    inline void setDLDO1(int voltage) { _set_DLDO(0, voltage); }
    inline void setDLDO2(int voltage) { _set_DLDO(1, voltage); }

    inline bool getALDO1Enabled(void) { return _get_LDOEn(0); }
    inline bool getALDO2Enabled(void) { return _get_LDOEn(1); }
    inline bool getALDO3Enabled(void) { return _get_LDOEn(2); }
    inline bool getALDO4Enabled(void) { return _get_LDOEn(3); }
    inline bool getBLDO1Enabled(void) { return _get_LDOEn(4); }
    inline bool getBLDO2Enabled(void) { return _get_LDOEn(5); }

    void powerOff(void);

    void setAdcState(bool enable);
    void setAdcRate( std::uint8_t rate );

    void setBACKUP(bool enable);

    bool isACIN(void);
    bool isVBUS(void);
    bool getBatState(void);

    float getBatteryVoltage(void);
    float getBatteryDischargeCurrent(void);
    float getBatteryChargeCurrent(void);
    float getBatteryPower(void);
    float getACINVoltage(void);
    float getACINCurrent(void);
    float getVBUSVoltage(void);
    float getVBUSCurrent(void);
    float getTSVoltage(void);
    float getAPSVoltage(void);
    float getInternalTemperature(void);

    /// @return 0:none / 1:Long press / 2:Short press / 3:both
    std::uint8_t getPekPress(void);

    bool enableIRQ(std::uint64_t registerEn);
    bool disableIRQ(std::uint64_t registerEn);
    std::uint64_t getIRQStatuses(void);
    void clearIRQStatuses();
    //IRQ STATUS 0
    bool isDropWarningLevel2Irq(void);
    bool isDropWarningLevel1Irq(void);
    bool isGaugeWdtTimeoutIrq();
    bool isBatChargerUnderTemperatureIrq(void);
    bool isBatChargerOverTemperatureIrq(void);
    bool isBatWorkOverTemperatureIrq(void);
    bool isBatWorkUnderTemperatureIrq(void);
    //IRQ STATUS 1
    bool isVbusInsertIrq(void);
    bool isVbusRemoveIrq(void);
    bool isBatInsertIrq(void);
    bool isBatRemoveIrq(void);
    bool isPekeyShortPressIrq(void);
    bool isPekeyLongPressIrq(void);
    bool isPekeyNegativeIrq(void);
    bool isPekeyPositiveIrq(void);
    //IRQ STATUS 2
    bool isWdtExpireIrq(void);
    bool isLdoOverCurrentIrq(void);
    bool isBatfetOverCurrentIrq(void);
    bool isBatChagerDoneIrq(void);
    bool isBatChagerStartIrq(void);
    bool isBatDieOverTemperatureIrq(void);
    bool isChagerOverTimeoutIrq(void);
    bool isBatOverVoltageIrq(void);


  private:
    std::uint8_t statusRegister[AXP2101_IRQSTAT_CNT];
    std::uint8_t intRegister[AXP2101_IRQSTAT_CNT];

    std::size_t readRegister12(std::uint8_t addr);
    std::size_t readRegister14(std::uint8_t addr);
    std::size_t readRegister16(std::uint8_t addr);

    void _set_LDO(std::uint8_t num, int voltage);
    void _set_DLDO(std::uint8_t num, int voltage);
    bool _get_LDOEn(std::uint8_t num);

    bool setIRQEnRegister(std::uint64_t registerEn, bool enable);
  };
}

#endif
