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

    #ifndef HAS_AXP2101
      Wire.begin(I2C_SDA, I2C_SCL);

      Wire.beginTransmission(IP5306_ADDR);
      error = Wire.endTransmission();

      if (error == 0) {
        Serial.println(F("Detected IP5306"));
        this->has_ip5306 = true;
        this->i2c_supported = true;
      }

      Wire.beginTransmission(MAX17048_ADDR);
      error = Wire.endTransmission();

      if (error == 0) {
        if (maxlipo.begin()) {
          Serial.println(F("Detected MAX17048"));
          this->has_max17048 = true;
          this->i2c_supported = true;
        }
      }
    #else
      bool result = this->power.begin(Wire, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL);

      if (!result)
        return;

      Serial.println(F("Detected AXP2101"));

      this->i2c_supported = true;
      this->has_axp2101 = true;
    #endif
    
    this->initTime = millis();
  #endif
}

int8_t BatteryInterface::getBatteryLevel() {

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

  #ifdef HAS_AXP2101
    if (this->has_axp2101) {
      return this->power.getBatteryPercent();
    }
  #endif
}
