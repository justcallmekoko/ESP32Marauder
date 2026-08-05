/*
 * CH32V003_IOExpander - Arduino/PlatformIO library (single-header, ESP32)
 * Converted from Waveshare ESP-IDF component (Apache-2.0)
 * Original: https://github.com/waveshareteam/Waveshare-ESP32-components
 *
 * Register map (from Waveshare source custom_io_expander_ch32v003.c):
 *   0x02  DIR  - direction, bit=1 input / bit=0 output
 *   0x03  OUT  - output levels
 *   0x04  IN   - input levels (read-only)
 *   0x05  PWM  - backlight duty 0-255
 *   0x06  ADC  - 2 bytes little-endian, 10-bit result
 *   0x07  INT  - interrupt / RTC status byte
 *
 * Header-only: all methods are `inline` so this file can be included from
 * multiple translation units without violating ODR.
 */

#pragma once
#include <Arduino.h>
#include <Wire.h>

// Default I2C address
#define CH32V003_DEFAULT_ADDR   (0x24)

// GPIO pin bitmasks - match esp_io_expander IO_EXPANDER_PIN_NUM_x
#define CH32V003_PIN_0   (1 << 0)
#define CH32V003_PIN_1   (1 << 1)
#define CH32V003_PIN_2   (1 << 2)
#define CH32V003_PIN_3   (1 << 3)
#define CH32V003_PIN_4   (1 << 4)
#define CH32V003_PIN_5   (1 << 5)
#define CH32V003_PIN_6   (1 << 6)
#define CH32V003_PIN_7   (1 << 7)
#define CH32V003_ALL_PINS (0xFF)

// Register map - from Waveshare source
#define CH32V003_REG_DIR    (0x02)   // direction: bit=1 input, bit=0 output
#define CH32V003_REG_OUT    (0x03)   // output level
#define CH32V003_REG_IN     (0x04)   // input level (read-only)
#define CH32V003_REG_PWM    (0x05)   // PWM duty 0-255
#define CH32V003_REG_ADC    (0x06)   // ADC - 2 bytes, little-endian (lo first)
#define CH32V003_REG_INT    (0x07)   // interrupt / RTC status

// Power-on defaults (matches CH32V003 reset state)
#define CH32V003_DIR_DEFAULT  (0xFF)  // all inputs
#define CH32V003_OUT_DEFAULT  (0x00)  // all low

#define BOARD_IO_EXTENSION_ADDR        0x24
#define BOARD_IO_EXTENSION_MODE_REG    0x02
#define BOARD_IO_EXTENSION_OUTPUT_REG  0x03
#define BOARD_IO_EXTENSION_PWM_REG     0x05

#define BOARD_TP_RST_EXIO  0
#define BOARD_LCD_RST_EXIO 1
#define BOARD_PA_CTRL_EXIO 3

class CH32V003_IOExpander {
public:
    inline explicit CH32V003_IOExpander(TwoWire &wire = Wire,
                                         uint8_t  addr = CH32V003_DEFAULT_ADDR)
        : _wire(wire)
        , _addr(addr)
        , _dirReg(CH32V003_DIR_DEFAULT)
        , _outReg(CH32V003_OUT_DEFAULT)
        , _invertPWM(false)
    {}

    // Call once after Wire.begin(). Returns false if device not ACK-ing.
    inline bool begin() {
        log_d("using addr: %d", _addr);
        _wire.beginTransmission(_addr);

        uint8_t r = _wire.endTransmission();
        if (r != 0) {
            log_d("Fail Ret Code: %d", r);
            return false;
        }
        return reset();
    }

    // Reset direction and output registers to power-on defaults.
    inline bool reset() {
        if (!writeReg(CH32V003_REG_DIR, CH32V003_DIR_DEFAULT)) return false;
        _dirReg = CH32V003_DIR_DEFAULT;
        if (!writeReg(CH32V003_REG_OUT, CH32V003_OUT_DEFAULT)) return false;
        _outReg = CH32V003_OUT_DEFAULT;
        log_d("reset true");
        return true;
    }

    inline void lcdReset() {
        this->pinMode(CH32V003_PIN_1, OUTPUT);
        this->digitalWrite(CH32V003_PIN_1, LOW);   // assert reset
        delay(50);
        this->digitalWrite(CH32V003_PIN_1, HIGH);  // enable display
        delay(120);
    }

    inline void touchReset() {
        this->pinMode(CH32V003_PIN_0, OUTPUT);
        this->digitalWrite(CH32V003_PIN_0, LOW);   // assert reset
        delay(50);
        this->digitalWrite(CH32V003_PIN_0, HIGH);  // release reset
        delay(120);
    }

    // -- GPIO ------------------------------------------------------------------
    // pinMask: one or more CH32V003_PIN_x OR'd together
    // mode:    OUTPUT or INPUT (Arduino constants)
    inline bool pinMode(uint8_t pinMask, uint8_t mode) {
        // DIR register: bit=1 → input, bit=0 → output  (same as Waveshare source)
        if (mode == OUTPUT) {
            _dirReg &= ~pinMask;
        } else {
            _dirReg |= pinMask;
        }
        return writeReg(CH32V003_REG_DIR, _dirReg);
    }

    // level: HIGH or LOW
    inline bool digitalWrite(uint8_t pinMask, uint8_t level) {
        if (level == HIGH) {
            _outReg |= pinMask;
        } else {
            _outReg &= ~pinMask;
        }
        return writeReg(CH32V003_REG_OUT, _outReg);
    }

    // Returns HIGH/LOW for the requested pin(s), or -1 on I2C error.
    // If pinMask covers multiple pins the return is the raw masked byte.
    inline int digitalRead(uint8_t pinMask) {
        int val = readReg(CH32V003_REG_IN);
        if (val < 0) return -1;
        uint8_t masked = (uint8_t)val & pinMask;
        // For single-pin reads return HIGH/LOW; for multi-pin return the raw mask.
        if ((pinMask & (pinMask - 1)) == 0) {
            return masked ? HIGH : LOW;
        }
        return masked;
    }

    // -- PWM backlight -----------------------------------------------------------
    // duty 0-255.  Some Waveshare boards invert this - call invertPWM(true).
    inline bool setPWM(uint8_t duty) {
        uint8_t val = _invertPWM ? (255 - duty) : duty;
        return writeReg(CH32V003_REG_PWM, val);
    }

    inline void invertPWM(bool invert) { _invertPWM = invert; }

    // -- ADC (battery voltage) ---------------------------------------------------
    // Returns raw 10-bit value (0-1023), or -1 on error.
    // ADC register returns 2 bytes little-endian per Waveshare source.
    inline int readADCRaw() {
        // Waveshare source: temp[1] << 8 | temp[0]  - little-endian, 2 bytes
        uint8_t buf[2] = {0, 0};
        if (!readReg2(CH32V003_REG_ADC, buf, 2)) return -1;
        return (int)((buf[1] << 8) | buf[0]);
    }

    // Converts raw reading to voltage assuming Waveshare's 3:1 resistor divider.
    inline float readBatteryVoltage(float vref = 3.3f) {
        int raw = readADCRaw();
        if (raw < 0) return -1.0f;
        // 10-bit ADC (0-1023), 3:1 resistor divider on Waveshare boards
        return ((float)raw / 1023.0f) * vref * 3.0f;
    }

    // -- Interrupt / RTC status ---------------------------------------------------
    inline int readINT() {
        return readReg(CH32V003_REG_INT);
    }

    // -- Low-level helpers ---------------------------------------------------------
    inline bool writeReg(uint8_t reg, uint8_t value) {
        _wire.beginTransmission(_addr);
        _wire.write(reg);
        _wire.write(value);
        return (_wire.endTransmission() == 0);
    }

    inline int readReg(uint8_t reg) {
        _wire.beginTransmission(_addr);
        _wire.write(reg);
        if (_wire.endTransmission(false) != 0) return -1;
        if (_wire.requestFrom((uint8_t)_addr, (uint8_t)1) < 1) return -1;
        return _wire.read();
    }

    inline bool readReg2(uint8_t reg, uint8_t *buf, uint8_t len) {
        _wire.beginTransmission(_addr);
        _wire.write(reg);
        if (_wire.endTransmission(false) != 0) return false;
        if (_wire.requestFrom((uint8_t)_addr, len) < len) return false;
        for (uint8_t i = 0; i < len; i++) buf[i] = _wire.read();
        return true;
    }

    // Dump all registers to Serial.
    inline void printState() {
        Serial.println("-- CH32V003 IO Expander --");
        Serial.printf("  DIR (0x%02X): 0x%02X %06b  (1=in 0=out per bit)\n",
                      CH32V003_REG_DIR, _dirReg, _dirReg);
        Serial.printf("  OUT (0x%02X): 0x%02X\n", CH32V003_REG_OUT, _outReg);
        int in  = readReg(CH32V003_REG_IN);
        Serial.printf("  IN  (0x%02X): 0x%02X\n", CH32V003_REG_IN,
                      in  >= 0 ? (uint8_t)in  : 0xFF);
        int pwm = readReg(CH32V003_REG_PWM);
        Serial.printf("  PWM (0x%02X): %d\n", CH32V003_REG_PWM,
                      pwm >= 0 ? pwm : -1);
        int adc = readADCRaw();
        Serial.printf("  ADC (0x%02X): raw=%d  %.2fV battery\n",
                      CH32V003_REG_ADC, adc,
                      adc >= 0 ? readBatteryVoltage() : 0.0f);
        int intr = readINT();
        Serial.printf("  INT (0x%02X): 0x%02X\n", CH32V003_REG_INT,
                      intr >= 0 ? (uint8_t)intr : 0xFF);
        Serial.println("--------------------------");
    }

private:
    TwoWire &_wire;
    uint8_t  _addr;
    uint8_t  _dirReg;
    uint8_t  _outReg;
    bool     _invertPWM;
};
