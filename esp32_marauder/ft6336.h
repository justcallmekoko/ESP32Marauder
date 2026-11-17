#pragma once
#ifndef ft6336_h
#define ft6336_h

#ifdef HAS_CAP_TOUCH

#include <Wire.h>
#include "ws_c5_exio.h"

#define FT6336_ADDR      0x38
#define FT6336_TD_STATUS 0x02
#define FT6336_T1_XH     0x03
#define FT6336_G_MODE    0xA4

#ifdef MARAUDER_WS_C5_35
static bool _ft6336_int_gate_active = false;
#endif

static bool _ft6336_read(uint8_t reg, uint8_t *buf, uint8_t len) {
    Wire.beginTransmission(FT6336_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((int)FT6336_ADDR, (int)len) != len) return false;
    for (uint8_t i = 0; i < len; i++) {
        if (!Wire.available()) return false;
        buf[i] = Wire.read();
    }
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
    // Range 0–255. The Waveshare C5 panel is hardware-validated at 12;
    // the prior value of 40 made some bare panels effectively unresponsive.
    Wire.beginTransmission(FT6336_ADDR);
    Wire.write(0x80);
#ifdef MARAUDER_WS_C5_35
    Wire.write(12);
#else
    Wire.write(40);
#endif
    Wire.endTransmission();

#ifdef MARAUDER_WS_C5_35
    // INT gate. G_MODE (0xA4) = 0 selects polling mode, where INT is held LOW
    // for as long as a finger is down instead of pulsing once per report.
    // ft6336_read_raw() then trusts the controller only while INT is LOW,
    // which stops phantom and stuck touches. TP_INT has an external pull-up.
    pinMode(CTP_INT, INPUT);
    Wire.beginTransmission(FT6336_ADDR);
    Wire.write(FT6336_G_MODE);
    Wire.write(0x00);
    const bool modeWriteOk = Wire.endTransmission() == 0;
    uint8_t gMode = 0xFF;
    const bool modeReadOk = modeWriteOk &&
        _ft6336_read(FT6336_G_MODE, &gMode, 1);
    _ft6336_int_gate_active = modeReadOk && gMode == 0x00;
    Serial.printf("[Touch] FT6336 G_MODE: 0x%02X%s\n",
                  gMode, _ft6336_int_gate_active
                      ? " (polling, INT gate active)"
                      : " (polling fallback active)");
#endif
}

// Reads raw panel coordinates (panel native portrait space: 0..319 × 0..479).
// Call this when you need to apply a rotation-specific transform.
// Returns 1 for an active touch, 0 for a confirmed release, and -1 when the
// controller could not be read. Keeping transport errors distinct prevents a
// momentary I2C miss from being interpreted as a new button tap.
static int8_t ft6336_read_raw(uint16_t *raw_x, uint16_t *raw_y) {
#ifdef MARAUDER_WS_C5_35
    // The C5 needs the bus reopened before a touch transaction, but reopening
    // it on every UI loop starves the FT6336 and can collapse input completely.
    // Pace physical reads to about 80 Hz, retry one malformed/failed transfer,
    // and bridge brief gaps with the last valid point. This matches the
    // hardware-validated Waveshare BSP behavior.
    static uint16_t last_x = 0, last_y = 0;
    static uint32_t last_read_ms = 0, last_ok_ms = 0;
    const uint32_t now = millis();
    int8_t sample = -1;

    if ((uint32_t)(now - last_read_ms) >= 12) {
      last_read_ms = now;
      if (_ft6336_int_gate_active && digitalRead(CTP_INT) != LOW) {
        sample = 0;
      } else {
        Wire.end();
        Wire.begin(CTP_SDA, CTP_SCL, 100000U);
        for (uint8_t attempt = 0; attempt < 2; ++attempt) {
          uint8_t data[7];
          if (!_ft6336_read(FT6336_TD_STATUS, data, 7)) continue;
          const uint8_t points = data[0] & 0x0F;
          if (points == 0) {
            sample = 0;
            break;
          }
          if (points > 2) continue;
          const uint16_t rx = ((uint16_t)(data[1] & 0x0F) << 8) | data[2];
          const uint16_t ry = ((uint16_t)(data[3] & 0x0F) << 8) | data[4];
          if (rx >= TFT_WIDTH || ry >= TFT_HEIGHT) continue;
          last_x = rx;
          last_y = ry;
          last_ok_ms = now;
          sample = 1;
          break;
        }
      }
    }

    if (sample == 1 || (last_ok_ms && (uint32_t)(now - last_ok_ms) < 90)) {
      *raw_x = last_x;
      *raw_y = last_y;
      return 1;
    }
    return sample == 0 ? 0 : -1;
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
