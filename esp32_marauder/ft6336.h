#pragma once

#ifndef ft6336_h
#define ft6336_h

#ifdef HAS_FT6336

#include <Wire.h>
#ifdef MARAUDER_WS_C5_35
  #include "CH32V00X_IOExpander.hpp"
#endif

#define FT6336_ADDR      0x38
#define FT6336_TD_STATUS 0x02
#define FT6336_T1_XH     0x03

#if !defined(CTP_SDA) && defined(I2C_SDA)
  #define CTP_SDA I2C_SDA
  #define CTP_SCL I2C_SCL
  #if defined(I2C_RST)
    #define CTP_RST I2C_RST
  #endif
#endif

#ifndef CTP_FREQ
  #ifdef I2C_FREQ
    #define CTP_FREQ I2C_FREQ
  #else
    #define CTP_FREQ 400000U
  #endif
#endif

static bool _ft6336_read(uint8_t reg, uint8_t *buf, uint8_t len) {
    Wire.beginTransmission(FT6336_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    Wire.requestFrom((int)FT6336_ADDR, (int)len);
    for (uint8_t i = 0; i < len; i++)
        buf[i] = Wire.available() ? Wire.read() : 0;
    return true;
}

#ifdef MARAUDER_WS_C5_35
// Waveshare ESP32-C5-Touch-LCD-3.5 panel bring-up via the CH32V00X (CH32V007)
// I/O expander at 0x24 (the generic CH32V00X_IOExpander driver; same device,
// registers and address as the former ws_c5_exio.h helper). EXIO0 = touch
// reset, EXIO1 = LCD reset, EXIO5 = panel power / backlight enable.
//
// The expander registers are driven directly (writeReg) rather than through
// lcdReset()/touchReset(), to preserve Waveshare's verified bring-up exactly:
// configure the three signals as outputs, then raise the power rail and BOTH
// resets TOGETHER before pulsing either reset. Resetting the FT6336 while its
// rail is still off (the earlier bug) left the touch chip unresponsive. The
// FT6336 also needs ~300 ms after its reset is released before it answers.
// Must run before tft.init(); safe to call more than once.
static bool ws_c5_exio_begin() {
    Wire.begin(CTP_SDA, CTP_SCL, 100000U);  // 100kHz: robust reads over the panel flex
    const uint8_t TP  = CH32V00X_PIN_0;     // EXIO0 -> FT6336 reset
    const uint8_t LCD = CH32V00X_PIN_1;     // EXIO1 -> ST7796 reset
    const uint8_t PWR = CH32V00X_PIN_5;     // EXIO5 -> panel power / backlight enable
    CH32V00X_IOExpander& io = CH32V00X_obj;
    bool ok = io.writeReg(CH32V00X_REG_DIR, 0xFF) &&            // all pins output
              io.writeReg(CH32V00X_REG_OUT, 0x00) &&            // all low
              io.writeReg(CH32V00X_REG_PWM, 0x00) &&            // backlight off
              io.writeReg(CH32V00X_REG_DIR, TP | LCD | PWR);    // drive the 3 signals
    ok = ok && io.writeReg(CH32V00X_REG_OUT, TP | LCD);        delay(50);
    ok = ok && io.writeReg(CH32V00X_REG_OUT, 0x00);           delay(50);
    ok = ok && io.writeReg(CH32V00X_REG_OUT, TP | LCD | PWR); delay(50);  // rail + resets up together
    // Power is on. Pulse each reset (active-low) with the rail up.
    ok = ok && io.writeReg(CH32V00X_REG_OUT, TP | PWR);       delay(20);   // LCD reset low
    ok = ok && io.writeReg(CH32V00X_REG_OUT, TP | LCD | PWR); delay(120);  // LCD reset high
    ok = ok && io.writeReg(CH32V00X_REG_OUT, LCD | PWR);      delay(20);   // touch reset low
    ok = ok && io.writeReg(CH32V00X_REG_OUT, TP | LCD | PWR); delay(300);  // touch reset high (FT6336 boot)
    ok = ok && io.writeReg(CH32V00X_REG_PWM, 0xFF);           // backlight full
    Serial.printf("[WS-C5] CH32V00X expander 0x24 %s\n", ok ? "ready" : "NOT FOUND");
    return ok;
}
#endif

static void ft6336_init() {
    log_d("ft6336 init");
#ifdef MARAUDER_WS_C5_35
    // Panel power, LCD reset and touch reset are on the CH32V00X I/O expander
    // (0x24), which also opens the shared I2C bus. Must run before tft.init().
    CH32V00X_obj.touchReset();
#elif CTP_RST >= 0
      pinMode(CTP_RST, OUTPUT);
      digitalWrite(CTP_RST, LOW);
      delay(10);
      digitalWrite(CTP_RST, HIGH);
      delay(300);
#endif

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

#endif // HAS_FT6336
#endif // ft6336_h
