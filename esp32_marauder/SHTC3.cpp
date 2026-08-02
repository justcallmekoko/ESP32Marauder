/*
 * SHTC3 - Arduino/PlatformIO library
 * Converted from Waveshare ESP32-C5 BSP example (05_shtc3.ino)
 */

#include "configs.h"
#include <Wire.h>

#ifdef HAS_SHTC3

#include "SHTC3.h"

// SHTC3::SHTC3(TwoWire &wire, uint8_t addr)
//     : _wire(wire), _addr(addr)
// {}


// SHTC3::SHTC3() : _wire(*Wire) {}

bool SHTC3::begin(TwoWire *wire, int sda, int scl) {
    _wire = wire ? wire : &Wire;
    if (sda != -1 && scl != -1) {
        _wire->setPins(sda, scl);
    }
    _wire->begin();
    inited = true;
    return softReset();
}

// bool SHTC3::begin() {
//     if (!wake()) return false;
//     inited = true;
//     return softReset();
// }

// -- Public API ----------------------------------------------------------------

bool SHTC3::read(float &temperature, float &humidity) {
    if (!wake()) return false;

    if (!_writeCmd(SHTC3_CMD_MEASURE_T_RH)) {
        sleep();
        return false;
    }

    // Datasheet: measurement time typ 10.8 ms, max 12.1 ms (normal mode)
    delay(20);

    uint8_t buf[6] = {};
    if (_wire->requestFrom(_addr, (uint8_t)6) != 6) {
        sleep();
        return false;
    }
    for (uint8_t i = 0; i < 6; i++) buf[i] = _wire->read();

    sleep();

    // Verify CRC for both words
    if (_crc8(&buf[0], 2) != buf[2] || _crc8(&buf[3], 2) != buf[5]) {
        return false;
    }

    uint16_t rawT  = ((uint16_t)buf[0] << 8) | buf[1];
    temperature = -45.0f + 175.0f * (float)rawT  / 65535.0f;

    uint16_t rawRH = ((uint16_t)buf[3] << 8) | buf[4];
    humidity    = 100.0f           * (float)rawRH / 65535.0f;

    return true;
}

bool SHTC3::readID(uint16_t &id) {
    if (!wake()) return false;
    bool ok = _writeCmd(SHTC3_CMD_READ_ID) && _readWord(id);
    sleep();
    return ok;
}

bool SHTC3::wake() {
    if (!_writeCmd(SHTC3_CMD_WAKE)) return false;
    delayMicroseconds(240);  // datasheet: t_IDLE max 240 µs
    return true;
}

void SHTC3::sleep() {
    _writeCmd(SHTC3_CMD_SLEEP);
}

bool SHTC3::softReset() {
    if (!_writeCmd(SHTC3_CMD_SOFT_RESET)) return false;
    delay(2);  // datasheet: t_RST max 240 µs, 2 ms is comfortable margin
    supported = true;
    return true;
}

// -- Private helpers -----------------------------------------------------------

bool SHTC3::_writeCmd(uint16_t cmd) {
    _wire->beginTransmission(_addr);
    _wire->write((uint8_t)(cmd >> 8));
    _wire->write((uint8_t)(cmd & 0xFF));
    return (_wire->endTransmission() == 0);
}

bool SHTC3::_readWord(uint16_t &value) {
    uint8_t buf[3] = {};
    if (_wire->requestFrom(_addr, (uint8_t)3) != 3) return false;
    for (uint8_t i = 0; i < 3; i++) buf[i] = _wire->read();
    if (_crc8(buf, 2) != buf[2]) return false;
    value = ((uint16_t)buf[0] << 8) | buf[1];
    return true;
}

uint8_t SHTC3::_crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
        }
    }
    return crc;
}

#endif  //  HAS_SHTC3
