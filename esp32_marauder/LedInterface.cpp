#include "LedInterface.h"


LedInterface::LedInterface() {

}

void LedInterface::RunSetup() {
  //Serial.println("Setting neopixel to black...");
  #ifdef HAS_NEOPIXEL_LED
    #ifdef MARAUDER_M5_NANO_C6
      pinMode(19, OUTPUT);
      delay(100);
      digitalWrite(19, HIGH);
    #endif
    strip.setBrightness(0);
    strip.begin();
    strip.setPixelColor(0, strip.Color(0, 0, 0));
    strip.show();
    //delay(100);
    strip.setBrightness(50);
    strip.setPixelColor(0, strip.Color(0, 0, 0));
    strip.show();
  #endif

  #ifdef HAS_T_DONGLE_LED
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);
    digitalWrite(4, LOW);
    digitalWrite(5, LOW);
    this->writeApa102Color(0, 0, 0);
  #endif

  this->initTime = millis();
}

void LedInterface::main(uint32_t currentTime) {
  uint8_t mode_to_render = settings_obj.loadSetting<bool>("EnableLED")
                             ? this->current_mode : MODE_OFF;

  #ifdef HAS_T_DONGLE_LED
    // Only clock a frame when the state changes.  Continuously sending idle
    // frames can make some APA102-compatible parts fall back to white.
    if (mode_to_render == this->last_t_dongle_mode) return;
    this->last_t_dongle_mode = mode_to_render;
  #endif

  if (mode_to_render == MODE_OFF) {
    this->ledOff();
    return;
  }

  else if (mode_to_render == MODE_RAINBOW) {
    this->rainbow();
  }
  else if (mode_to_render == MODE_ATTACK) {
    this->attackLed();
  }
  else if (mode_to_render == MODE_SNIFF) {
    this->sniffLed();
  }
  else if (mode_to_render == MODE_CUSTOM) {
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
  #ifdef HAS_T_DONGLE_LED
    this->writeApa102Color(static_cast<uint8_t>(r), static_cast<uint8_t>(g),
                          static_cast<uint8_t>(b));
  #endif
}

#ifdef HAS_T_DONGLE_LED
void LedInterface::writeApa102Byte(uint8_t value) {
  for (int bit = 7; bit >= 0; --bit) {
    digitalWrite(5, (value >> bit) & 0x01);
    digitalWrite(4, HIGH);
    digitalWrite(4, LOW);
  }
}

void LedInterface::writeApa102Color(uint8_t red, uint8_t green, uint8_t blue) {
  const uint8_t brightness = (red || green || blue) ? 8 : 0;
  noInterrupts();
  for (uint8_t i = 0; i < 4; ++i) writeApa102Byte(0x00);
  writeApa102Byte(0xE0 | brightness);
  writeApa102Byte(blue);
  writeApa102Byte(green);
  writeApa102Byte(red);
  // One LED latches after its color frame.  A legacy four-byte 0xFF end frame
  // is interpreted as additional full-white data by this board's APA102/SK9822
  // and leaves the LED white after the requested color is shown.
  digitalWrite(5, LOW);
  digitalWrite(4, LOW);
  interrupts();
}
#endif

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
  return 0;
}
