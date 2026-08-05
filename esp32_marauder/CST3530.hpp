/*
 * CST3530 - Arduino/PlatformIO library (single-header, ESP32)
 * Converted from Waveshare ESP-IDF component (Apache-2.0)
 * Original: https://github.com/waveshareteam/Waveshare-ESP32-components
 *
 * CST3530 capacitive touch controller - I2C address 0x58
 * Uses 32-bit register addresses (4-byte address phase over I2C)
 * Supports up to 5 simultaneous touch points
 *
 * Deduplication: compare each incoming point's x, y, and event against
 * the previous read for the same trackId. Suppress if all three match.
 * The chip's own finger-event field (bits[7:4] of buf[8]) is used directly:
 *   1 = Press  -> CST3530Event::DOWN
 *   2 = Hold   -> CST3530Event::MOVE
 *   0 = Lift   -> CST3530Event::UP
 *
 * Header-only: all methods are `inline` so this file can be included from
 * multiple translation units without violating ODR. Just:
 *
 *     #include "CST3530.hpp"
 *     CST3530 touch;
 *     void setup() { touch.begin(); }
 */

#pragma once


#ifdef HAS_CST3530

#ifndef CST3530_hpp
#define CST3530_hpp

#include <Arduino.h>
#include <Wire.h>
#include <cstring>


// -- I2C address ---------------------------------------------------------------
#define CST3530_I2C_ADDR        (0x58)

// -- Register map (32-bit addresses) ------------------------------------------
#define CST3530_REG_END_READ    (0xD00002ABul)
#define CST3530_REG_DATA        (0xD0070000ul)
#define CST3530_REG_COORD_NEXT  (0xD0070900ul)

// -- Debug / control registers -------------------------------------------------
#define CST3530_REG_SOFT_RESET  (0xD00003ul)
#define CST3530_REG_REPORT_MODE (0xD0000Cul)
#define CST3530_REG_SLEEP       (0xD00022ul)

// -- Limits --------------------------------------------------------------------
#define CST3530_MAX_POINTS      (5)

#ifndef TP_INT
  #define TP_INT -1
#endif

#ifndef TP_RST
  #define TP_RST -1
#endif

// -- Touch event : mapped directly from chip finger-event field ----------------
// buf[8] bits[7:4]: 1=Press(DOWN), 2=Hold(MOVE), 0=Lift(UP)
enum class CST3530Event : uint8_t {
    DOWN = 1,
    MOVE = 2,
    UP   = 0,
};

// -- Touch point ---------------------------------------------------------------
struct CST3530Point {
    uint16_t     x;
    uint16_t     y;
    uint16_t     strength;
    uint8_t      trackId;
    CST3530Event event;
};

// -- Event callback ------------------------------------------------------------
using CST3530Callback = void (*)(const CST3530Point &);

// -- Main class ----------------------------------------------------------------
class CST3530 {
public:
    inline CST3530(TwoWire &wire           = Wire,
                    uint8_t  addr           = CST3530_I2C_ADDR,
                    int8_t   rstPin         = TP_RST,
                    int8_t   intPin         = TP_INT,
                    uint8_t  rstActiveLevel = LOW)
        : _wire(wire), _addr(addr), _rstPin(rstPin), _intPin(intPin)
        , _rstActiveLevel(rstActiveLevel), _interruptEnabled(false)
        , _pointCount(0), _callback(nullptr)
    {
        memset(_points,    0, sizeof(_points));
        memset(_prev,      0, sizeof(_prev));
        memset(_prevValid, 0, sizeof(_prevValid));
    }

    inline bool begin(TwoWire &wire = Wire) {
        log_d("_intPin=%d  _rstPin=%d, _intPin, _rstPin");
        if (_intPin >= 0) ::pinMode(_intPin, INPUT);
        if (_rstPin >= 0) {
            ::pinMode(_rstPin, OUTPUT);
            ::digitalWrite(_rstPin, !_rstActiveLevel);
        }
        if (_intPin >= 0)
            enableInterrupt();
        _wire.beginTransmission(_addr);
        if (_wire.endTransmission() != 0) return false;
        if (_rstPin >= 0) reset();
        log_d("CST3530::begin: True");
        return true;
    }

    inline void reset() {
        if (_rstPin < 0) return;
        ::digitalWrite(_rstPin, _rstActiveLevel);
        delay(100);
        ::digitalWrite(_rstPin, !_rstActiveLevel);
        delay(500);
    }

    // -- Interrupt API ---------------------------------------------------------
    inline void enableInterrupt(int8_t iPin = -1) {
        if (iPin >= 0) {
            _intPin = iPin;
            pinMode(_intPin, INPUT);
        }
        if (_intPin < 0) return;
        _isrFlag = false;
        attachInterrupt(digitalPinToInterrupt(_intPin), _isrHandler, FALLING);
        log_d("CST3530::enableInterrupt : %d", _intPin);
        _interruptEnabled = true;
    }

    inline void disableInterrupt() {
        if (_intPin < 0) return;
        detachInterrupt(digitalPinToInterrupt(_intPin));
        _interruptEnabled = false;
    }

    inline bool available() const {
        if (_intPin < 0) return false;
        return _interruptEnabled ? _isrFlag : (::digitalRead(_intPin) == LOW);
    }

    inline void clearTouched() { _isrFlag = false; }

    // -- Event callback --------------------------------------------------------
    // Fired once per point inside readData() — only when x, y, or event differs
    // from the previous read for that trackId.
    inline void onEvent(CST3530Callback cb) { _callback = cb; }

    // -- Data API ----------------------------------------------------------------
    inline bool readData() {
        _pointCount = 0;
        _isrFlag    = false;

        uint8_t buf[9 + (CST3530_MAX_POINTS - 1) * 5] = {0};
        if (!readReg(CST3530_REG_DATA, buf, 9)) {
            writeReg(CST3530_REG_END_READ);
            return false;
        }

        uint8_t cnt = buf[3] & 0x0F;
        if (cnt == 0 || (buf[8] & 0xF0) == 0x00) {
            writeReg(CST3530_REG_END_READ);
            return false;
        }
        if (cnt > CST3530_MAX_POINTS) cnt = CST3530_MAX_POINTS;

        if (cnt > 1) {
            if (!readReg(CST3530_REG_COORD_NEXT, &buf[9], (cnt - 1) * 5)) {
                writeReg(CST3530_REG_END_READ);
                return false;
            }
        }
        writeReg(CST3530_REG_END_READ);

        for (uint8_t i = 0; i < cnt; i++) {
            uint8_t base = 4 + i * 5;

            CST3530Point p;
            p.x        = (uint16_t)(buf[base + 0]) | (uint16_t)((buf[base + 3] & 0x0F) << 8);
            p.y        = (uint16_t)(buf[base + 1]) | (uint16_t)((buf[base + 3] & 0xF0) << 4);
            p.strength = (uint16_t)(buf[base + 2]);
            p.trackId  = (uint8_t) (buf[base + 4] & 0x0F);
            p.event    = (CST3530Event)((buf[base + 4] >> 4) & 0x0F); // bits[7:4]

            // Deduplicate: suppress if x, y, and event all match previous read
            uint8_t id = p.trackId;
            if (id < CST3530_MAX_POINTS && _prevValid[id]) {
                if (p.x == _prev[id].x && p.y == _prev[id].y && p.event == _prev[id].event) {
                    continue;  // identical to last read — skip
                }
            }

            // Store as new previous state
            if (id < CST3530_MAX_POINTS) {
                _prev[id]      = p;
                _prevValid[id] = true;
                // Clear slot on UP so a re-touch of same id is treated as fresh
                if (p.event == CST3530Event::UP) _prevValid[id] = false;
            }

            if (_callback) _callback(p);
            _points[_pointCount++] = p;
        }

        return (_pointCount > 0);
    }

    inline uint8_t getPointCount() const { return _pointCount; }

    inline CST3530Point getPoint(uint8_t index) const {
        if (index >= _pointCount || index >= CST3530_MAX_POINTS)
            return CST3530Point{0, 0, 0, 0, CST3530Event::UP};
        return _points[index];
    }

    inline uint8_t getXY(uint16_t *x, uint16_t *y,
                          uint16_t *strength = nullptr,
                          uint8_t   maxPoints = CST3530_MAX_POINTS) {
        if (!x || !y || maxPoints == 0) return 0;
        uint8_t cnt = (_pointCount < maxPoints) ? _pointCount : maxPoints;
        for (uint8_t i = 0; i < cnt; i++) {
            x[i] = _points[i].x;
            y[i] = _points[i].y;
            if (strength) strength[i] = _points[i].strength;
        }
        _pointCount = 0;
        return cnt;
    }

    // legacy compatibility,  returns down point else first point
    inline uint8_t getTouch(uint16_t *x, uint16_t *y, uint16_t *strength = nullptr) {
        if (!x || !y || _pointCount == 0) return 0;
        for (uint8_t i = 0; i < _pointCount; i++) {
            if (_points[i].event == CST3530Event::DOWN) {         // Finger Down Event
                *x = _points[i].x;
                *y = _points[i].y;
                if (strength) strength[i] = _points[i].strength;
                log_d("X=%d Y=%d _pointCount=%d  Event::DOWN", _points[i].x, _points[i].y, _pointCount);
                return _pointCount;
            }
        }

        // would returning an avg be better ?
        if (_pointCount) {
            *x = _points[0].x;
            *y = _points[0].y;
            if (strength) strength[0] = _points[0].strength;
            log_d("X=%d Y=%d _pointCount=%d   _points[0]", _points[0].x, _points[0].y, _pointCount);
        } else {
            log_d("_pointCount=%d", _pointCount);
        }
        return _pointCount;
    }

    // -- Sleep / wake ------------------------------------------------------------
    inline bool sleep() {
        uint8_t val = 0xAB;
        return writeReg(CST3530_REG_SLEEP, &val, 1);
    }

    inline bool wake() {
        if (_rstPin < 0) return false;
        reset();
        return true;
    }

    // -- Mode control --------------------------------------------------------------
    inline bool setNormalMode() {
        uint8_t val = 0x00;
        return writeReg(CST3530_REG_REPORT_MODE, &val, 1);
    }

    inline bool setGestureMode() {
        uint8_t val = 0x01;
        return writeReg(CST3530_REG_REPORT_MODE, &val, 1);
    }

    inline bool softReset() {
        uint8_t val = 0xAB;
        return writeReg(CST3530_REG_SOFT_RESET, &val, 1);
    }

    // -- Low-level I2C -----------------------------------------------------------
    inline bool writeReg(uint32_t reg, const uint8_t *data = nullptr, uint8_t len = 0) {
        if (len > 16) return false;
        uint8_t tx[20];
        tx[0] = (reg >> 24) & 0xFF;  tx[1] = (reg >> 16) & 0xFF;
        tx[2] = (reg >>  8) & 0xFF;  tx[3] =  reg        & 0xFF;
        if (data && len > 0) memcpy(&tx[4], data, len);
        _wire.beginTransmission(_addr);
        _wire.write(tx, 4 + len);
        return (_wire.endTransmission() == 0);
    }

    inline bool readReg(uint32_t reg, uint8_t *buf, uint8_t len) {
        uint8_t ab[4] = { (uint8_t)(reg>>24), (uint8_t)(reg>>16),
                          (uint8_t)(reg>> 8), (uint8_t)(reg    ) };
        _wire.beginTransmission(_addr);
        _wire.write(ab, 4);
        if (_wire.endTransmission(false) != 0) return false;
        if (_wire.requestFrom((uint8_t)_addr, len) < len) return false;
        for (uint8_t i = 0; i < len; i++) buf[i] = _wire.read();
        return true;
    }

    struct CST3530Point *data = &_points[0];

    uint8_t  _addr;
    int8_t   _rstPin;
    int8_t   _intPin;

private:
    TwoWire &_wire;
    uint8_t  _rstActiveLevel;
    bool     _interruptEnabled;

    uint8_t      _pointCount;
    CST3530Point _points[CST3530_MAX_POINTS];

    // Last seen state per trackId — just x, y, event for comparison
    CST3530Point _prev[CST3530_MAX_POINTS];
    bool         _prevValid[CST3530_MAX_POINTS];

    CST3530Callback _callback;

    // `inline` static data member (C++17) avoids a separate .cpp definition.
    static inline volatile bool _isrFlag = false;

    static void IRAM_ATTR _isrHandler() { _isrFlag = true; }
};

#endif   //  CST3530_hpp

#endif    // HAS_CST3530
