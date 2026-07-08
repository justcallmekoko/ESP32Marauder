#pragma once
#ifndef ft6336_h
#define ft6336_h

#ifdef HAS_CAP_TOUCH

#include <Wire.h>

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
    pinMode(CTP_RST, OUTPUT);
    digitalWrite(CTP_RST, LOW);
    delay(10);
    digitalWrite(CTP_RST, HIGH);
    delay(300);
    Wire.begin(CTP_SDA, CTP_SCL, 400000U);

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
static uint8_t ft6336_read_raw(uint16_t *raw_x, uint16_t *raw_y) {
    uint8_t data[7];
    if (!_ft6336_read(FT6336_TD_STATUS, data, 7)) return 0;
    if ((data[0] & 0x0F) == 0) return 0;
    *raw_x = ((uint16_t)(data[1] & 0x0F) << 8) | data[2];
    *raw_y = ((uint16_t)(data[3] & 0x0F) << 8) | data[4];
    return 1;
}

// Portrait-only convenience wrapper (rotation 0).
// For rotation-aware transforms, use ft6336_read_raw() via Display::updateTouch().
static uint8_t ft6336_update(uint16_t *x, uint16_t *y) {
    uint16_t raw_x, raw_y;
    if (!ft6336_read_raw(&raw_x, &raw_y)) return 0;
    *x = (raw_x < SCREEN_WIDTH)  ? raw_x : (uint16_t)(SCREEN_WIDTH  - 1);
    *y = (raw_y < SCREEN_HEIGHT) ? raw_y : (uint16_t)(SCREEN_HEIGHT - 1);
    return 1;
}

#endif // HAS_CAP_TOUCH
#endif // ft6336_h
