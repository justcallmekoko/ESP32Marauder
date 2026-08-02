/*
 * SHTC3 - Arduino/PlatformIO library
 * Sensirion SHTC3 temperature + humidity sensor, I2C address 0x70
 * Converted from Waveshare ESP32-C5 BSP example (05_shtc3.ino)
 *
 * Protocol:
 *   - All commands are 16-bit, sent MSB first
 *   - Sensor sleeps by default; must be woken before every transaction
 *   - All read words are followed by a CRC-8 byte (poly 0x31, init 0xFF)
 *   - Measurement packet: [T_hi][T_lo][T_crc][RH_hi][RH_lo][RH_crc]
 */

#pragma once

#include "configs.h"

#ifdef HAS_SHTC3

#ifndef SHTC3_h
#define SHTC3_h

#include <Arduino.h>
#include <Wire.h>

#define SHTC3_I2C_ADDR        (0x70)

// -- Commands (16-bit, MSB first) ----------------------------------------------
#define SHTC3_CMD_WAKE        (0x3517)
#define SHTC3_CMD_SLEEP       (0xB098)
#define SHTC3_CMD_SOFT_RESET  (0x805D)
#define SHTC3_CMD_READ_ID     (0xEFC8)
// T first, no clock stretching, normal power mode
#define SHTC3_CMD_MEASURE_T_RH (0x7866)

class SHTC3 {
public:
    // explicit SHTC3(TwoWire &wire = Wire, uint8_t addr = SHTC3_I2C_ADDR);
    // SHTC3();

    // Probe device, issue soft reset. Returns false if not found.
    // bool begin();

    // RTClib-compatible begin - takes TwoWire pointer
    // bool begin(TwoWire *wire = &Wire);

    // Extended begin - takes TwoWire reference + optional SDA/SCL pins
    // Matches Waveshare BSP: rtc.begin(Wire, SDA, SCL)
    bool begin(TwoWire *wire, int sda = -1, int scl = -1);

    // Read temperature (°C) and relative humidity (%).
    // Wakes sensor, measures, sleeps. Returns false on I2C or CRC error.
    bool read(float &temperature, float &humidity);

    // Read the SHTC3 product ID word. Returns false on error.
    bool readID(uint16_t &id);

    // Explicit sleep / wake (normally handled internally by read()).
    bool wake();
    void sleep();

    // Software reset - sensor re-wakes in ~1 ms (datasheet: max 240 µs).
    bool softReset();

    bool inited = false;
    bool supported = false;

private:
    TwoWire *_wire = &Wire;

    uint8_t  _addr = SHTC3_I2C_ADDR;

    bool    _writeCmd(uint16_t cmd);
    bool    _readWord(uint16_t &value);   // reads 2 data bytes + 1 CRC byte
    uint8_t _crc8(const uint8_t *data, size_t len);
};

#endif  //  SHTC3_h
#endif   // HAS_SHTC3
