/*
 * SHTC3.hpp - minimal Sensirion SHTC3 temperature / humidity driver for ESP32
 * (single-header, ESP32)
 *
 *   SHTC3 sht;
 *   sht.begin(0, 1);              // SDA, SCL
 *   if (sht.read()) { sht.temperature(); sht.humidity(); }
 *
 * Header-only: all methods are `inline` so this file can be included from
 * multiple translation units without violating ODR.
 */

#pragma once

#ifndef SHTC3_hpp
#define SHTC3_hpp

#include <Arduino.h>
#include <Wire.h>

class SHTC3 {
public:
    inline SHTC3() : _wire(nullptr) {}

    /// Start the I2C bus and check the sensor is there.
    inline bool begin(int sdaPin, int sclPin, uint32_t frequency = 400000) {
        if (sdaPin >= 0 && sclPin >= 0) {
            Wire.begin(sdaPin, sclPin);
        } else {
            Wire.begin();
        }

        if (frequency > 0) {
            Wire.setClock(frequency);
        }
        return begin(&Wire);
    }

    /// Same, but for a bus you already called Wire.begin() on.
    inline bool begin(TwoWire *wireInstance = &Wire) {
        _wire = wireInstance;

        if (!wake()) {
            return false;
        }

        sleep();
        return true;
    }

    /// Take a measurement (~13 ms). Returns false on I2C or CRC failure.
    inline bool read() {
        if (!wake()) {
            return false;
        }

        if (!writeCommand(SHTC3_CMD_MEASURE)) {
            sleep();
            return false;
        }

        delay(13); // datasheet: 12.1 ms max in normal mode

        uint8_t buf[6] = {0};

        if (_wire->requestFrom(SHTC3_ADDRESS, static_cast<uint8_t>(6)) != 6) {
            sleep();
            return false;
        }

        for (uint8_t i = 0; i < 6; i++) {
            buf[i] = static_cast<uint8_t>(_wire->read());
        }

        sleep();

        if (crc8(&buf[0], 2) != buf[2] || crc8(&buf[3], 2) != buf[5]) {
            return false;
        }

        uint16_t rawTemp     = static_cast<uint16_t>(buf[0]) << 8 | buf[1];
        uint16_t rawHumidity = static_cast<uint16_t>(buf[3]) << 8 | buf[4];

        _temperature = -45.0f + 175.0f * static_cast<float>(rawTemp) / 65535.0f;
        _humidity    = 100.0f * static_cast<float>(rawHumidity) / 65535.0f;
        return true;
    }

    inline bool read(float &temperatureC, float &relativeHumidity) {
        if (!read()) {
            return false;
        }

        temperatureC     = _temperature;
        relativeHumidity = _humidity;
        return true;
    }

    inline float temperature() const { return _temperature; } ///< deg C, last reading
    inline float humidity() const { return _humidity; }       ///< %RH, last reading

    float _temperature = 0.0f;
    float _humidity = 0.0f;

private:
    static constexpr uint8_t  SHTC3_ADDRESS     = 0x70;
    static constexpr uint16_t SHTC3_CMD_WAKE     = 0x3517;
    static constexpr uint16_t SHTC3_CMD_SLEEP    = 0xB098;
    static constexpr uint16_t SHTC3_CMD_MEASURE  = 0x7866; // T first, no clock stretch

    inline static uint8_t crc8(const uint8_t *data, size_t length) {
        uint8_t crc = 0xFF; // CRC-8, poly 0x31, init 0xFF

        for (size_t i = 0; i < length; i++) {
            crc ^= data[i];
            for (uint8_t bit = 0; bit < 8; bit++) {
                crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x31)
                                    : static_cast<uint8_t>(crc << 1);
            }
        }

        return crc;
    }

    inline bool writeCommand(uint16_t command) {
        _wire->beginTransmission(SHTC3_ADDRESS);
        _wire->write(static_cast<uint8_t>(command >> 8));
        _wire->write(static_cast<uint8_t>(command & 0xFF));
        return _wire->endTransmission() == 0;
    }

    inline bool wake() {
        // The first command after sleep is occasionally NACKed while the internal
        // oscillator starts, so allow one retry.
        for (uint8_t attempt = 0; attempt < 2; attempt++) {
            if (writeCommand(SHTC3_CMD_WAKE)) {
                delayMicroseconds(240);
                return true;
            }
            delayMicroseconds(240);
        }

        return false;
    }

    inline void sleep() {
        writeCommand(SHTC3_CMD_SLEEP);
    }

    TwoWire *_wire;
    // float _temperature = 0.0f;
    // float _humidity = 0.0f;
};


#endif  // SHTC3_hpp
