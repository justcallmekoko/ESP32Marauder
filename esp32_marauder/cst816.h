#pragma once
#ifndef cst816_h
#define cst816_h

// CST816T capacitive touch driver for the Waveshare ESP32-S3-Touch-LCD-1.69.
//
// Drop-in replacement for ft6336.h: it exposes cst816_init() and
// cst816_read_raw() with the SAME contract as the FT6336 module, then aliases
// the FT6336 symbol names so Display.cpp's existing HAS_CAP_TOUCH read path
// (ft6336_init / ft6336_read_raw) works unchanged. Select it from Display.h
// with HAS_CST816_TOUCH (see WS_LCD_169_PORT.md).
//
// Pins come from configs.h (CTP_SDA/CTP_SCL/CTP_RST/CTP_INT). On the 1.69 board
// those map to the Waveshare pin_config.h values: SDA=11, SCL=10, RST=13, INT=14.

#ifdef HAS_CAP_TOUCH
#ifdef HAS_CST816_TOUCH

#include <Wire.h>

#define CST816_ADDR       0x15
#define CST816_REG_GESTURE 0x01  // gesture id
#define CST816_REG_FINGER  0x02  // number of fingers (0 or 1)
#define CST816_REG_XH      0x03  // X[11:8] in low nibble
#define CST816_REG_CHIPID  0xA7  // 0xB4=CST816S, 0xB5=CST816T, 0xB6=CST816D
#define CST816_REG_SLEEP   0xFE  // DisAutoSleep: write 1 to keep awake

static bool _cst816_read(uint8_t reg, uint8_t *buf, uint8_t len) {
    Wire.beginTransmission(CST816_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((int)CST816_ADDR, (int)len) != len) return false;
    for (uint8_t i = 0; i < len; i++) {
        if (!Wire.available()) return false;
        buf[i] = Wire.read();
    }
    return true;
}

static void _cst816_write(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(CST816_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

static void cst816_init() {
    // Hardware reset: CST816 needs a LOW pulse then ~50ms to boot.
    #if CTP_RST >= 0
      pinMode(CTP_RST, OUTPUT);
      digitalWrite(CTP_RST, LOW);
      delay(10);
      digitalWrite(CTP_RST, HIGH);
      delay(60);
    #endif
    #if CTP_INT >= 0
      pinMode(CTP_INT, INPUT_PULLUP);
    #endif

    Wire.begin(CTP_SDA, CTP_SCL, 400000U);

    uint8_t chipId = 0;
    if (_cst816_read(CST816_REG_CHIPID, &chipId, 1)) {
        const char *name = chipId == 0xB5 ? " (CST816T)" :
                           chipId == 0xB4 ? " (CST816S)" :
                           chipId == 0xB6 ? " (CST816D)" : " (unexpected)";
        Serial.printf("[Touch] CST816 ID: 0x%02X%s\n", chipId, name);
    } else {
        Serial.println("[Touch] CST816 not responding on I2C 0x15");
    }

    // Disable auto-sleep so the panel keeps reporting during long idle screens.
    _cst816_write(CST816_REG_SLEEP, 0x01);
}

// Reads raw panel coordinates in panel-native portrait space (0..TFT_WIDTH-1 x
// 0..TFT_HEIGHT-1). Returns 1 for an active touch, 0 for a confirmed release,
// and -1 when the controller could not be read. Matches ft6336_read_raw() so
// Display::updateTouch() applies the same rotation transform.
static int8_t cst816_read_raw(uint16_t *raw_x, uint16_t *raw_y) {
    // Burst-read gesture..Y (regs 0x01..0x06): 6 bytes.
    uint8_t d[6];
    if (!_cst816_read(CST816_REG_GESTURE, d, 6)) return -1;
    const uint8_t fingers = d[1] & 0x0F;          // reg 0x02
    if (fingers == 0) return 0;
    if (fingers > 1) return -1;
    const uint16_t rx = ((uint16_t)(d[2] & 0x0F) << 8) | d[3];  // regs 0x03/0x04
    const uint16_t ry = ((uint16_t)(d[4] & 0x0F) << 8) | d[5];  // regs 0x05/0x06
    if (rx >= TFT_WIDTH || ry >= TFT_HEIGHT) return -1;
    *raw_x = rx;
    *raw_y = ry;
    return 1;
}

// Alias the FT6336 entry points so the core touch path needs no edits.
#define ft6336_init     cst816_init
#define ft6336_read_raw cst816_read_raw

#endif // HAS_CST816_TOUCH
#endif // HAS_CAP_TOUCH
#endif // cst816_h
