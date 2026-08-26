#pragma once
#ifndef HeltecOLED_h
#define HeltecOLED_h

#include "configs.h"

#ifdef HELTEC_WIFI_LORA_32_V4

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Heltec WiFi LoRa 32 V4 OLED wiring (from Heltec core V4 variant)
#define HELTEC_OLED_SDA   17
#define HELTEC_OLED_SCL   18
#define HELTEC_OLED_RST   21
#define HELTEC_VEXT       36   // active-LOW power enable for the peripheral rail
#define HELTEC_OLED_W     128
#define HELTEC_OLED_H     64
#define HELTEC_OLED_ADDR  0x3C

class HeltecOLED {
  public:
    void begin();
    void update(uint32_t now);
  private:
    Adafruit_SSD1306 _d = Adafruit_SSD1306(HELTEC_OLED_W, HELTEC_OLED_H, &Wire, HELTEC_OLED_RST);
    bool _ok = false;
    uint32_t _last = 0;
    uint8_t _spin = 0;
    void draw();
    const char* modeStr(uint8_t m);
};

extern HeltecOLED heltec_oled;

#endif // HELTEC_WIFI_LORA_32_V4
#endif // HeltecOLED_h
