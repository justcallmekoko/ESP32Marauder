#pragma once
#ifndef ft6336_h
#define ft6336_h

#ifdef HAS_CAP_TOUCH

#include <Wire.h>
#include "ws_c5_exio.h"

#define FT6336_ADDR      0x38
#define FT6336_TD_STATUS 0x02
#define FT6336_T1_XH     0x03

static bool _ft6336_read(uint8_t reg, uint8_t *buf, uint8_t len) {
    Wire.beginTransmission(FT6336_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    Wire.requestFrom((int)FT6336_ADDR, (int)len);
    for (uint8_t i = 0; i < len; i++)
        buf[i] = Wire.available() ? Wire.read() : 0;
    return true;
}

static void ft6336_init() {
#ifdef MARAUDER_WS_C5_35
    // Panel power, LCD reset and touch reset are on the I/O expander (0x24),
    // which also opens the shared I2C bus. Must run before tft.init().
    ws_c5_exio_begin();
#else
    #if CTP_RST >= 0
      pinMode(CTP_RST, OUTPUT);
      digitalWrite(CTP_RST, LOW);
      delay(10);
      digitalWrite(CTP_RST, HIGH);
      delay(300);
    #endif
    Wire.begin(CTP_SDA, CTP_SCL, 400000U);
#endif

    uint8_t chipId = 0;
    Wire.beginTransmission(FT6336_ADDR);
    Wire.write(0xA3);
    if (Wire.endTransmission(false) == 0) {
        Wire.requestFrom((int)FT6336_ADDR, 1);
        if (Wire.available()) chipId = Wire.read();
    }
    Serial.printf("[Touch] FT6336U ID: 0x%02X%s\n",
                  chipId, chipId == 0x64 ? " (OK)" : " (unexpected)");

    // Raise touch threshold to reduce phantom touches.
    // Register 0x80 = IDTHRESHOLD, default 22. Higher = less sensitive.
    // Range 0–255; 40 is a good balance for panel-in-case use.
    Wire.beginTransmission(FT6336_ADDR);
    Wire.write(0x80);
    Wire.write(40);
    Wire.endTransmission();
}

// Reads raw panel coordinates (panel native portrait space: 0..319 × 0..479).
// Call this when you need to apply a rotation-specific transform.
// Returns 1 for an active touch, 0 for a confirmed release, and -1 when the
// controller could not be read. Keeping transport errors distinct prevents a
// momentary I2C miss from being interpreted as a new button tap.
static int8_t ft6336_read_raw(uint16_t *raw_x, uint16_t *raw_y) {
#ifdef MARAUDER_WS_C5_35
    // On the C5, the early Wire.begin() does not leave the I2C controller able
    // to complete reads, so re-open the bus each read. Validate the result
    // (1-2 points, on-panel coords) so a noisy read cannot register as a tap
    // and false-trigger STOP during scans. Scoped to this board; other FT6336
    // boards keep the original path below unchanged.
    Wire.end(); Wire.begin(CTP_SDA, CTP_SCL, 100000U);
    uint8_t data[7];
    if (!_ft6336_read(FT6336_TD_STATUS, data, 7)) return -1;
    const uint8_t points = data[0] & 0x0F;
    if (points == 0) return 0;
    if (points > 2) return -1;
    const uint16_t rx = ((uint16_t)(data[1] & 0x0F) << 8) | data[2];
    const uint16_t ry = ((uint16_t)(data[3] & 0x0F) << 8) | data[4];
    if (rx >= TFT_WIDTH || ry >= TFT_HEIGHT) return -1;
    *raw_x = rx;
    *raw_y = ry;
    return 1;
#else
    uint8_t data[7];
    if (!_ft6336_read(FT6336_TD_STATUS, data, 7)) return -1;
    if ((data[0] & 0x0F) == 0) return 0;
    *raw_x = ((uint16_t)(data[1] & 0x0F) << 8) | data[2];
    *raw_y = ((uint16_t)(data[3] & 0x0F) << 8) | data[4];
    return 1;
#endif
}

// Portrait-only convenience wrapper (rotation 0).
// For rotation-aware transforms, use ft6336_read_raw() via Display::updateTouch().
static uint8_t ft6336_update(uint16_t *x, uint16_t *y) {
    uint16_t raw_x, raw_y;
    if (ft6336_read_raw(&raw_x, &raw_y) != 1) return 0;
    *x = (raw_x < SCREEN_WIDTH)  ? raw_x : (uint16_t)(SCREEN_WIDTH  - 1);
    *y = (raw_y < SCREEN_HEIGHT) ? raw_y : (uint16_t)(SCREEN_HEIGHT - 1);
    return 1;
}

#endif // HAS_CAP_TOUCH
#endif // ft6336_h
