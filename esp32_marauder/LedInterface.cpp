#include "LedInterface.h"


LedInterface::LedInterface() {

}

void LedInterface::RunSetup() {
  //Serial.println("Setting neopixel to black...");
  #ifdef HAS_NEOPIXEL_LED
    strip.setBrightness(0);
    strip.begin();
    strip.setPixelColor(0, strip.Color(0, 0, 0));
    strip.show();
    //delay(100);
    strip.setBrightness(50);
    strip.setPixelColor(0, strip.Color(0, 0, 0));
    strip.show();
  #endif

  this->initTime = millis();
}

void LedInterface::main(uint32_t currentTime) {
  if ((!settings_obj.loadSetting<bool>("EnableLED")) ||
      (this->current_mode == MODE_OFF)) {
    this->ledOff();
    return;
  }

  else if (this->current_mode == MODE_RAINBOW) {
    this->rainbow();
  }
  else if (this->current_mode == MODE_ATTACK) {
    this->attackLed();
  }
  else if (this->current_mode == MODE_SNIFF) {
    this->sniffLed();
  }
  else if (this->current_mode == MODE_CUSTOM) {
    return;
  }
  else {
    this->ledOff();
  }
};

void LedInterface::setMode(uint8_t new_mode) {
  this->current_mode = new_mode;
}

uint8_t LedInterface::getMode() {
  return this->current_mode;
}

void LedInterface::setColor(int r, int g, int b) {
  #ifdef HAS_NEOPIXEL_LED
    strip.setPixelColor(0, strip.Color(r, g, b));
    strip.show();
  #endif
}

void LedInterface::sniffLed() {
  this->setColor(0, 0, 255);
}

void LedInterface::attackLed() {
  this->setColor(255, 0, 0);
}

void LedInterface::ledOff() {
  this->setColor(0, 0, 0);
}

void LedInterface::rainbow() {
  #ifdef HAS_NEOPIXEL_LED
    strip.setPixelColor(0, this->Wheel((0 * 256 / 100 + this->wheel_pos) % 256));
    strip.show();

    this->current_fade_itter++;

    this->wheel_pos = this->wheel_pos - this->wheel_speed;
    if (this->wheel_pos < 0)
      this->wheel_pos = 255;
  #endif
}

uint32_t LedInterface::Wheel(byte WheelPos) {
  #ifdef HAS_NEOPIXEL_LED
    WheelPos = 255 - WheelPos;
    if(WheelPos < 85) {
      return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if(WheelPos < 170) {
      WheelPos -= 85;
      return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  #endif
}