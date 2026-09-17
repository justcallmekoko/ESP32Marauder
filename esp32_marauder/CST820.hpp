/*
 * CST820 - Arduino/PlatformIO library (single-header, ESP32)
 *
 * MIT License
 * Copyright (c) 2026 Peter Shipley
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction,
 *
 * Some parts borrowed from fbiego's CST816S driver (fbiego/CST816S)
 * https://pan.jczn1688.com/pd/1/HMI%20display/JC2432W328.zip
 *
 * Note: if the upper nibble of reg 0xFE is set, gesture "event types" are
 * disabled. How to re-enable gestures once that's set is not yet documented
 * here.
 *
 * Header-only: all methods are `inline` so this file can be included from
 * multiple translation units without violating ODR. Just:
 *
 *     #include "CST820.hpp"
 *     CST820 touch;
 *     void setup() { touch.begin(SDA_PIN, SCL_PIN, RST_PIN, INT_PIN); }
 *
 * This version has no dependency on a project-wide "configs.h" / HAS_CST820
 * gate - define HAS_CST820 before including this file if you still want to
 * conditionally compile it out in a shared codebase; it's on by default.
 */

#pragma once

#ifndef CST820_hpp
#define CST820_hpp 1

#ifdef HAS_CST820

#include <Arduino.h>
#include <Wire.h>
#include <cstring>

#ifndef I2C_ADDR_CST820
#define I2C_ADDR_CST820 0x15
#endif

enum GESTURE {
    None       = 0x00,
    SlideDown  = 0x01,
    SlideUp    = 0x02,
    SlideLeft  = 0x03,
    SlideRight = 0x04,
    SingleTap  = 0x05,
    DoubleTap  = 0x0B,
    LongPress  = 0x0C
};

//  from CST820
struct data_struct {
    byte    gestureID;   // Gesture ID
    byte    points;      // Number of touch points
    byte    event;       // Event (0 = Down, 1 = Up, 2 = Contact)
    short   x;
    short   y;
    uint8_t version;
    uint8_t versionInfo[3]; // Chip, proj, firmware ver
};

/*!
    @brief  CST820 I2C CTP controller driver
*/
class CST820 {
public:
    inline CST820() : _wire(nullptr), sda(-1), scl(-1) {}

    inline void begin(int8_t _sda = -1, int8_t _scl = -1, int8_t _rst = -1,
                       int8_t _int = -1, uint32_t freq = 0) {
        TwoWire *mywire;

        log_d("CST820::begin");
#ifdef I2C_SDA
        if (_sda != -1 && _sda != I2C_SDA) {
            mywire = &Wire1;
            log_d("CST820::begin using Wire1");
        } else {
            mywire = &Wire;
        }
#else
        mywire = &Wire;
#endif

        sda = _sda;
        scl = _scl;

        // Initialize I2C
        // https://github.com/esp8266/Arduino/issues/1025
        // https://www.forward.com.au/pfod/ArduinoProgramming/I2C_ClearBus/index.html
        if (_sda != -1 && _scl != -1) {
            pinMode(_scl, OUTPUT);
            log_d("CST820::begin SDA=%d SCL=%d, freq=%d", _sda, _scl, freq);
            mywire->begin(_sda, _scl, freq);
        } else {
            mywire->begin();
            if (freq) {
                mywire->setClock(freq);
                log_d("CST820::begin setClock %d", freq);
            }
        }

        int_pin = _int;
        // Int Pin Configuration
        if (_int != -1) {
            pinMode(_int, INPUT_PULLUP);
        }

        rst_pin = _rst;
        // Reset Pin Configuration
        if (_rst != -1) {
            pinMode(_rst, OUTPUT);
            digitalWrite(_rst, LOW);
            delay(10);
            digitalWrite(_rst, HIGH);
            delay(300);
        }

        begin(mywire);
    }

    inline void begin(TwoWire *mywire) {
        _wire = mywire;

        memset(&data, 0, sizeof(data));

        data.version = i2c_read(0x15);
        delay(5);
        int8_t x = i2c_read_continuous(0xA7, data.versionInfo, 3);
        if (x != 0) {
            Serial.print("Err reading data.versionInfo: ");
            Serial.println(x);
        }

        disable_auto_sleep();

        // Enable continuous gesture actions and double-click
        enable_double_click(0x07);

        /*  Untested
        i2c_write(0xEF, 30);   // MotionSlAngle
        i2c_write(0xFA, 0X79); // IrqCtl
        i2c_write(0xFB, 5);    // AutoReset
        i2c_write(0xFC, 10);   // LongPressTime
        */
    }

    /*!
        @brief  update data obj
    */
    inline void read_touch() {
        byte data_raw[8];
        int8_t x;
        do {
            x = i2c_read_continuous(0x01, data_raw, 6);  // -1 = err, 0 = good
        } while (x);

        data.gestureID = data_raw[0] & 0x0F;   // Gesture
        data.points    = data_raw[1];          // number of touch points
        data.event     = data_raw[2] >> 6;      // Event (0 = Down, 1 = Up, 2 = Contact)
        data.x         = ((data_raw[2] & 0xF) << 8) + data_raw[3];
        data.y         = ((data_raw[4] & 0xF) << 8) + data_raw[5];
    }

    /*
        Legacy call interface
    */
    inline uint8_t getTouch(uint16_t *x, uint16_t *y, uint16_t ignore = 0) {
        (void)ignore;
        read_touch();

        *x = data.x;
        *y = data.y;

        return data.points;
    }

    /*
        test for input
    */
    inline bool available() {
        read_touch();
        return static_cast<bool>(data.points);
    }

    /*
       - Bit 0: EnDClick (enable double-click)
       - Bit 1: EnConUD (enable continuous up/down swipe)
       - Bit 2: EnConLR (enable continuous left/right swipe)
    */
    inline int8_t enable_double_click(byte enable = 0x01) {
        return i2c_write(0xEC, enable & 0x07);
    }

    /*!
        @brief  Disable auto sleep mode
    */
    inline int8_t disable_auto_sleep() {
        byte disableAutoSleep = 0x01;  // 0x01 value disables auto sleep
        return i2c_write(0xFE, disableAutoSleep);
    }

    /*!
        @brief  Enable auto sleep mode
    */
    inline int8_t enable_auto_sleep() {
        byte enableAutoSleep = 0x00;  // 0 value enables auto sleep
        return i2c_write(0xFE, enableAutoSleep);
    }

    /*!
        @brief  Set the auto sleep time
        @param  seconds Time in seconds (1-255) before entering standby mode after inactivity
    */
    inline int8_t set_auto_sleep_time(int seconds = 1) {
        if (seconds < 1) {
            seconds = 1;    // Enforce minimum value of 1 second
        } else if (seconds > 255) {
            seconds = 255;  // Enforce maximum value of 255 seconds
        }
        byte sleepTime = static_cast<byte>(seconds);
        return i2c_write(0xF9, sleepTime);
    }

    /*!
        @brief  get the gesture event name
    */
    inline String gesture() {
        switch (data.gestureID) {
            case None:       return F("NONE");
            case SlideDown:  return F("SWIPE DOWN");
            case SlideUp:    return F("SWIPE UP");
            case SlideLeft:  return F("SWIPE LEFT");
            case SlideRight: return F("SWIPE RIGHT");
            case SingleTap:  return F("SINGLE CLICK");
            case DoubleTap:  return F("DOUBLE CLICK");
            case LongPress:  return F("LONG PRESS");
            default:         return (String)data.gestureID;
        }
    }

    inline String event_type() {
        static const char *const EVENT_TYPES[3] = {"Down", "Up", "Contact"};
        if (data.event > 2) return String("Unknown");
        return String(EVENT_TYPES[data.event]);
    }

    data_struct data;

    int8_t int_pin = -1;
    int8_t rst_pin = -1;


private:
    TwoWire *_wire;
    int8_t   sda, scl;

    inline uint8_t i2c_read(uint8_t addr) {
        uint8_t rdData = 0;
        uint8_t rdDataCount;
        do {
            _wire->beginTransmission(I2C_ADDR_CST820);
            _wire->write(addr);
            _wire->endTransmission(false);  // Restart
            rdDataCount = _wire->requestFrom(I2C_ADDR_CST820, 1);
        } while (rdDataCount == 0);
        while (_wire->available()) {
            rdData = _wire->read();
        }
        return rdData;
    }

    inline int8_t i2c_read_continuous(uint8_t addr, uint8_t *data, uint32_t length) {
        _wire->beginTransmission(I2C_ADDR_CST820);
        _wire->write(addr);
        if (_wire->endTransmission(true))
            return -1;
        _wire->requestFrom(I2C_ADDR_CST820, length);
        for (uint32_t i = 0; i < length; i++) {
            *data++ = _wire->read();
        }
        return 0;
    }

    inline int8_t i2c_write(uint8_t addr, uint8_t data) {
        _wire->beginTransmission(I2C_ADDR_CST820);
        _wire->write(addr);
        _wire->write(data);
        return _wire->endTransmission(true);
    }

    inline int8_t i2c_write_continuous(uint8_t addr, const uint8_t *data, uint32_t length) {
        _wire->beginTransmission(I2C_ADDR_CST820);
        _wire->write(addr);
        for (uint32_t i = 0; i < length; i++) {
            _wire->write(*data++);
        }
        return _wire->endTransmission(true);
    }
};

#endif   // HAS_CST820

#endif   // CST820_hpp

