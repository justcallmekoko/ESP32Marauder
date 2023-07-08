#include "BatteryInterface.h"
#include "lang_var.h"
BatteryInterface::BatteryInterface() {
  
}

void BatteryInterface::main(uint32_t currentTime) {
  #if !defined(MARAUDER_FLIPPER) && !defined(MARAUDER_ENDGAME_S3)
    if (currentTime != 0) {
      if (currentTime - initTime >= 3000) {
        //Serial.println("Checking Battery Level");
        this->initTime = millis();
        int8_t new_level = this->getBatteryLevel();
        //this->battery_level = this->getBatteryLevel();
        if (this->battery_level != new_level) {
          Serial.println(text00 + (String)new_level);
          this->battery_level = new_level;
        }
      }
    }
  #endif
}

void BatteryInterface::RunSetup() {
  #if !defined(MARAUDER_FLIPPER) && !defined(MARAUDER_ENDGAME_S3)
    Wire.begin(I2C_SDA, I2C_SCL);
    this->initTime = millis();
  #endif
}

int8_t BatteryInterface::getBatteryLevel() {
  #if !defined(MARAUDER_FLIPPER) && !defined(MARAUDER_ENDGAME_S3)
    Wire.beginTransmission(IP5306_ADDR);
    Wire.write(0x78);
    if (Wire.endTransmission(false) == 0 &&
        Wire.requestFrom(0x75, 1)) {
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
  #endif
}
