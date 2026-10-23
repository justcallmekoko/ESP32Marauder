// SPDX-License-Identifier: MIT
// Board helper for the Waveshare ESP32-C5-Touch-LCD-3.5 I/O expander (0x24).
// The expander owns LCD reset, touch reset and backlight PWM. Register map and
// bring-up sequence are taken from Waveshare's own example (Board_IO.cpp) and
// follow the board's verified bring-up sequence.
#pragma once
#ifdef MARAUDER_WS_C5_35
#include <Arduino.h>
#include <Wire.h>

#define WS_EXIO_ADDR        0x24
#define WS_EXIO_MODE_REG    0x02
#define WS_EXIO_OUTPUT_REG  0x03
#define WS_EXIO_PWM_REG     0x05
#define WS_EXIO_TP_RST      0   // EXIO 0 -> FT6336 reset
#define WS_EXIO_LCD_RST     1   // EXIO 1 -> ST7796 reset
#define WS_EXIO_POWER       5   // EXIO 5 -> panel power / backlight enable

static uint8_t ws_exio_output = 0;
static bool ws_exio_ready = false;

static bool ws_exio_write(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(WS_EXIO_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static bool ws_exio_set(uint8_t pin, bool level) {
  if (level) ws_exio_output |= (1U << pin);
  else ws_exio_output &= ~(1U << pin);
  return ws_exio_write(WS_EXIO_OUTPUT_REG, ws_exio_output);
}

// Brings up I2C, powers the panel, and hardware-resets the LCD and touch
// controller. Safe to call more than once. Must run before tft.init().
static bool ws_c5_exio_begin() {
  Wire.begin(CTP_SDA, CTP_SCL, 100000U);  // 100kHz: robust reads over the panel flex
  // Match Waveshare Board_IO_Init: configure the three signals as outputs, then
  // bring the rail and both resets up TOGETHER so power is present before any
  // reset pulse. Resetting the touch chip while its rail is off (the earlier
  // bug) left the FT6336 unresponsive.
  bool ok = ws_exio_write(WS_EXIO_MODE_REG, 0xFF) &&
            ws_exio_write(WS_EXIO_OUTPUT_REG, 0x00) &&
            ws_exio_write(WS_EXIO_PWM_REG, 0x00) &&
            ws_exio_write(WS_EXIO_MODE_REG,
                            (1U << WS_EXIO_TP_RST) |
                            (1U << WS_EXIO_LCD_RST) |
                            (1U << WS_EXIO_POWER));
  ws_exio_output = (1U << WS_EXIO_TP_RST) | (1U << WS_EXIO_LCD_RST);
  ok = ok && ws_exio_write(WS_EXIO_OUTPUT_REG, ws_exio_output); delay(50);
  ws_exio_output = 0x00;
  ok = ok && ws_exio_write(WS_EXIO_OUTPUT_REG, ws_exio_output); delay(50);
  ws_exio_output = (1U << WS_EXIO_TP_RST) | (1U << WS_EXIO_LCD_RST) |
                     (1U << WS_EXIO_POWER);
  ok = ok && ws_exio_write(WS_EXIO_OUTPUT_REG, ws_exio_output); delay(50);
  // Power is now on. Pulse each reset (active-low) with the rail up.
  ok = ok && ws_exio_set(WS_EXIO_LCD_RST, false); delay(20);
  ok = ok && ws_exio_set(WS_EXIO_LCD_RST, true);  delay(120);
  ok = ok && ws_exio_set(WS_EXIO_TP_RST, false);  delay(20);
  ok = ok && ws_exio_set(WS_EXIO_TP_RST, true);   delay(300);  // FT6336 boot
  ok = ok && ws_exio_write(WS_EXIO_PWM_REG, 0xFF);  // backlight full
  ws_exio_ready = ok;
  Serial.printf("[WS-C5] I/O expander 0x24 %s\n", ok ? "ready" : "NOT FOUND");
  return ok;
}
#endif
