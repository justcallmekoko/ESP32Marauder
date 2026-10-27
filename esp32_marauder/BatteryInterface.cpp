
// #ifdef HAS_BATTERY

#include "BatteryInterface.h"
#include "lang_var.h"
BatteryInterface::BatteryInterface() {

}

void BatteryInterface::main(uint32_t currentTime) {
  if (currentTime != 0) {
    if (currentTime - initTime >= 3000) {
      //Serial.println("Checking Battery Level");
      this->initTime = millis();
      int8_t new_level = this->getBatteryLevel();
      //this->battery_level = this->getBatteryLevel();
      if (this->battery_level != new_level) {
        Serial.println(text00 + (String)new_level);
        this->battery_level = new_level;
        Serial.println("Battery Level: " + (String)this->battery_level);
      }
    }
  }
}

void BatteryInterface::RunSetup() {
  byte error;
  byte addr;

  #ifdef HAS_BATTERY

    #ifdef BATTERY_ADC_PIN
        analogReadResolution(12);
        pinMode(BATTERY_ADC_PIN, INPUT);
        this->has_adc_battery = true;
        // this->i2c_supported = true;
        Serial.println(F("Battery: ADC mode"));

    #elif defined(HAS_AXP2101) && defined(I2C_SDA)
        bool result = this->power.begin(Wire, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL);

        if (!result)
          return;

        Serial.println(F("Detected AXP2101"));

        this->i2c_supported = true;
        this->has_axp2101 = true;

    #elif defined(I2C_SDA)  // other i2c (shared)

        Wire.begin(I2C_SDA, I2C_SCL);

        #ifdef HAS_IP5306
          Wire.beginTransmission(IP5306_ADDR);
          error = Wire.endTransmission();

          if (error == 0) {
            Serial.println(F("Detected IP5306"));
            this->has_ip5306 = true;
            this->i2c_supported = true;
          }
        #endif

        #ifdef HAS_AXP192
            axp192_obj.begin();
        #endif

        #ifdef HAS_AXP2101_GAUGE
          this->detectAxp2101Gauge();
        #endif


        #ifdef HAS_MAX1704X
          Wire.beginTransmission(MAX17048_ADDR);
          error = Wire.endTransmission();

          if (error == 0) {
            if (maxlipo.begin()) {
              Serial.println(F("Detected MAX17048"));
              this->has_max17048 = true;
              this->i2c_supported = true;
            }
          }
        #endif

  #endif //  other i2c

    this->initTime = millis();
  #endif // HAS_BATTERY
}

int8_t BatteryInterface::getBatteryLevel() {

  #ifdef HAS_BATTERY

    #ifdef BATTERY_ADC_PIN
      if (this->has_adc_battery) {
        int voltage_mv = analogReadMilliVolts(BATTERY_ADC_PIN) * 2; // voltage divider ratio 2:1
        if (voltage_mv <= 3300) return 0;
        if (voltage_mv >= 4150) return 100;
        return (int8_t)(((voltage_mv - 3300) * 100) / 850);
      }
    #endif

    #ifdef HAS_IP5306
      if (this->has_ip5306) {
        Wire.beginTransmission(IP5306_ADDR);
        Wire.write(0x78);
        if (Wire.endTransmission(false) == 0 &&
            Wire.requestFrom(IP5306_ADDR, 1)) {
          this->i2c_supported = true;
          switch (Wire.read() & 0xF0) {
            case 0xE0: return 25;
            case 0xC0: return 50;
            case 0x80: return 75;
            case 0x00: return 100;
            default: return 0;
          }
        }
        this->i2c_supported = false;
        return -1;
      }
    #endif


    #ifdef HAS_AXP2101_GAUGE
      // Retry discovery after transient shared-I2C startup failures.
      if (!this->has_axp2101_gauge && !this->detectAxp2101Gauge()) return -1;
      if (this->has_axp2101_gauge) {
        int status = this->axp2101Read(AXP2101_REG_STATUS1);
        int percent = (status >= 0 && (status & 0x08))  // bit 3: cell fitted
                        ? this->axp2101Read(AXP2101_REG_BAT_PERCENT) : -1;
        // Hide the status-bar reading while no cell is fitted (or the read fails).
        this->i2c_supported = percent >= 0;
        if (percent < 0) return -1;
        return percent > 100 ? 100 : (int8_t)percent;
      }
    #endif

    #ifdef HAS_MAX1704X
      if (this->has_max17048) {
        float percent = this->maxlipo.cellPercent();

        // Sometimes we dumb
        if (percent >= 100)
          return 100;
        else if (percent <= 0)
          return 0;
        else
          return percent;
      }
    #endif

    #ifdef HAS_AXP2101
      if (this->has_axp2101) {
        return this->power.getBatteryPercent();
      }
    #endif

  #endif // HAS_BATTERY

  return -1;
}

// #endif // HAS_BATTERY

#ifdef HAS_AXP2101_GAUGE
bool BatteryInterface::detectAxp2101Gauge() {
  if (this->axp2101Read(AXP2101_REG_CHIP_ID) != AXP2101_CHIP_ID) {
    this->i2c_supported = false;
    return false;
  }

  int ctrl = this->axp2101Read(AXP2101_REG_GAUGE_CTRL);
  if (ctrl >= 0 && !(ctrl & 0x08)) {
    Wire.beginTransmission(AXP2101_GAUGE_ADDR);
    Wire.write(AXP2101_REG_GAUGE_CTRL);
    Wire.write((uint8_t)(ctrl | 0x08));
    Wire.endTransmission();
  }
  if (!this->has_axp2101_gauge) Serial.println(F("Detected AXP2101"));
  this->has_axp2101_gauge = true;
  this->i2c_supported = true;
  return true;
}

// One-byte register read from the AXP2101; -1 on any I2C error.
int BatteryInterface::axp2101Read(uint8_t reg) {
  Wire.beginTransmission(AXP2101_GAUGE_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return -1;
  if (Wire.requestFrom(AXP2101_GAUGE_ADDR, 1) != 1) return -1;
  return Wire.read();
}
#endif
