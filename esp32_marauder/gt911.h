#pragma once
#ifndef gt911_h
#define gt911_h

#ifdef HAS_GT911_TOUCH

#include <Wire.h>

#define GT911_ADDR1      0x5D
#define GT911_ADDR2      0x14
#define GT911_POINT_INFO 0x814E
#define GT911_POINT_1    0x814F

static uint8_t _gt911_addr = GT911_ADDR1;

static bool _gt911_read_block(uint16_t reg, uint8_t *buf, uint8_t len) {
    Wire.beginTransmission(_gt911_addr);
    Wire.write((uint8_t)(reg >> 8));
    Wire.write((uint8_t)(reg & 0xFF));
    if (Wire.endTransmission(false) != 0) return false;
    Wire.requestFrom((int)_gt911_addr, (int)len);
    for (uint8_t i = 0; i < len; i++)
        buf[i] = Wire.available() ? Wire.read() : 0;
    return true;
}

static bool _gt911_write_reg(uint16_t reg, uint8_t val) {
    Wire.beginTransmission(_gt911_addr);
    Wire.write((uint8_t)(reg >> 8));
    Wire.write((uint8_t)(reg & 0xFF));
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

static void gt911_init() {
    pinMode(GT911_INT, INPUT);
    Wire.begin(GT911_SDA, GT911_SCL, 400000U);

    // GT911's I2C address depends on the INT pin level during the chip's
    // own power-on reset, which we don't control here, so probe both.
    uint8_t probe;
    _gt911_addr = GT911_ADDR1;
    if (!_gt911_read_block(GT911_POINT_INFO, &probe, 1)) {
        _gt911_addr = GT911_ADDR2;
    }
    Serial.printf("[Touch] GT911 @ 0x%02X\n", _gt911_addr);
}

static uint16_t _gt911_last_x = 0;
static uint16_t _gt911_last_y = 0;
static bool _gt911_last_touched = false;

// Reads raw panel coordinates as reported by the GT911 (native panel
// space, before any rotation/axis mapping). Call this when you need to
// apply a rotation-specific transform, same convention as ft6336_read_raw().
//
// The GT911 only raises its "new data" bit once per its own internal scan
// cycle. Our main loop polls faster than that, so a naive "not ready yet
// -> not touched" read flickers true/false many times over a single
// physical touch, which the button widget's edge-detection reads as
// several rapid taps. We hold the last known touched state across any
// "not ready" poll and only update it when the chip reports fresh data.
static uint8_t gt911_read_raw(uint16_t *raw_x, uint16_t *raw_y) {
    uint8_t status;
    if (!_gt911_read_block(GT911_POINT_INFO, &status, 1)) {
        *raw_x = _gt911_last_x;
        *raw_y = _gt911_last_y;
        return _gt911_last_touched ? 1 : 0;
    }

    if ((status & 0x80) == 0) {
        // No fresh sample since our last ack - not a release, just an
        // in-between poll. Report the last known state instead of 0.
        *raw_x = _gt911_last_x;
        *raw_y = _gt911_last_y;
        return _gt911_last_touched ? 1 : 0;
    }

    uint8_t touches = status & 0x0F;
    if (touches == 0) {
        _gt911_write_reg(GT911_POINT_INFO, 0);
        _gt911_last_touched = false;
        return 0;
    }

    uint8_t data[7];
    _gt911_read_block(GT911_POINT_1, data, 7);
    _gt911_write_reg(GT911_POINT_INFO, 0);  // ack so the chip prepares the next sample

    _gt911_last_x = data[1] | ((uint16_t)data[2] << 8);
    _gt911_last_y = data[3] | ((uint16_t)data[4] << 8);
    _gt911_last_touched = true;

    *raw_x = _gt911_last_x;
    *raw_y = _gt911_last_y;
    return 1;
}

#endif // HAS_GT911_TOUCH
#endif // gt911_h
