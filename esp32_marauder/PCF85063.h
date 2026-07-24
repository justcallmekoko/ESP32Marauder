/*
 * PCF85063 - Arduino/PlatformIO RTC library
 * NXP PCF85063A/TP, I2C address 0x51
 *
 * Drop-in replacement for RTClib RTC_PCF8523:
 *   rtc.begin(&Wire)
 *   rtc.initialized()
 *   rtc.lostPower()
 *   rtc.adjust(DateTime)
 *   rtc.now()          → DateTime
 *   rtc.start()
 *   rtc.stop()
 *   rtc.isrunning()
 *
 * DateTime mirrors RTClib:
 *   DateTime(year, month, day, hour, minute, second)
 *   DateTime(F(__DATE__), F(__TIME__))
 *   DateTime(uint32_t unix_epoch)
 *   dt.year(), dt.month(), dt.day()
 *   dt.hour(), dt.minute(), dt.second()
 *   dt.dayOfTheWeek()
 *   dt.unixtime()
 *
 * Register map (PCF85063Constants.h):
 *   0x00 CTRL1  0x01 CTRL2  0x02 OFFSET  0x03 RAM
 *   0x04 SEC    0x05 MIN    0x06 HOUR
 *   0x07 DAY    0x08 WEEKDAY 0x09 MONTH  0x0A YEAR
 */

#pragma once

#ifndef PCF85063_h
#define PCF85063_h

#include "configs.h"

#ifdef HAS_PCF85063

#include <Arduino.h>
#include <Wire.h>
#include <stdint.h>
#include <time.h>

// -- DateTime - mirrors RTClib -------------------------------------------------
class DateTime {
public:
    DateTime() : _year(2000), _month(1), _day(1),
                 _hour(0), _minute(0), _second(0) {}

    DateTime(uint16_t year, uint8_t month, uint8_t day,
             uint8_t hour = 0, uint8_t minute = 0, uint8_t second = 0)
        : _year(year), _month(month), _day(day),
          _hour(hour), _minute(minute), _second(second) {}

    // DateTime(F(__DATE__), F(__TIME__)) - compile-time stamp
    DateTime(const char *date, const char *time) {
        // date = "Oct 11 2026", time = "13:24:43"
        static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
        char mon[4] = {};
        int d, y;
        sscanf(date, "%3s %d %d", mon, &d, &y);
        _year  = (uint16_t)y;
        _day   = (uint8_t)d;
        _month = 1;
        for (uint8_t i = 0; i < 12; i++) {
            if (strncmp(mon, months + i * 3, 3) == 0) {
                _month = i + 1;
                break;
            }
        }
        int h, m, s;
        sscanf(time, "%d:%d:%d", &h, &m, &s);
        _hour   = (uint8_t)h;
        _minute = (uint8_t)m;
        _second = (uint8_t)s;
    }

    // DateTime(unix_epoch) - mirrors RTClib
    DateTime(uint32_t t) {
        time_t tt = (time_t)t;
        struct tm *ti = gmtime(&tt);
        _year   = (uint16_t)(ti->tm_year + 1900);
        _month  = (uint8_t) (ti->tm_mon  + 1);
        _day    = (uint8_t)  ti->tm_mday;
        _hour   = (uint8_t)  ti->tm_hour;
        _minute = (uint8_t)  ti->tm_min;
        _second = (uint8_t)  ti->tm_sec;
    }

    // DateTime(struct tm*)
    DateTime(struct tm *ti) {
        _year   = (uint16_t)(ti->tm_year + 1900);
        _month  = (uint8_t) (ti->tm_mon  + 1);
        _day    = (uint8_t)  ti->tm_mday;
        _hour   = (uint8_t)  ti->tm_hour;
        _minute = (uint8_t)  ti->tm_min;
        _second = (uint8_t)  ti->tm_sec;
    }

    // DateTime from ISO8601 string "2026-10-11 13:24:43" or T separator
    DateTime(const char *iso8601) {
        if (!iso8601 || strlen(iso8601) < 19) { *this = DateTime(); return; }
        auto c2 = [](const char *p) -> int {
            int v = 0;
            if (*p >= '0' && *p <= '9') v = (*p - '0') * 10;
            p++;
            if (*p >= '0' && *p <= '9') v += (*p - '0');
            return v;
        };
        _year   = (uint16_t)(2000 + c2(iso8601 + 2));
        _month  = (uint8_t)c2(iso8601 + 5);
        _day    = (uint8_t)c2(iso8601 + 8);
        _hour   = (uint8_t)c2(iso8601 + 11);
        _minute = (uint8_t)c2(iso8601 + 14);
        _second = (uint8_t)c2(iso8601 + 17);
    }

    // -- Accessors - identical to RTClib ---------------------------------------
    uint16_t year()   const { return _year;   }
    uint8_t  month()  const { return _month;  }
    uint8_t  day()    const { return _day;    }
    uint8_t  hour()   const { return _hour;   }
    uint8_t  minute() const { return _minute; }
    uint8_t  second() const { return _second; }

    // Day of week: 0=Sun … 6=Sat (Zeller's congruence)
    uint8_t dayOfTheWeek() const {
        uint16_t y = _year;
        uint8_t  m = _month;
        if (m < 3) { m += 12; y--; }
        uint32_t v = (_day
                      + ((m + 1) * 26) / 10
                      + y + y / 4
                      + 6 * (y / 100)
                      + y / 400) % 7;
        return (uint8_t)(v == 0 ? 6 : v - 1);
    }

    // Unix timestamp (seconds since 1970-01-01 00:00:00 UTC)
    uint32_t unixtime() const {
        struct tm t = {};
        t.tm_year = _year - 1900;
        t.tm_mon  = _month - 1;
        t.tm_mday = _day;
        t.tm_hour = _hour;
        t.tm_min  = _minute;
        t.tm_sec  = _second;
        return (uint32_t)mktime(&t);
    }

    // Populate a struct tm
    struct tm toTm() const {
        struct tm t = {};
        t.tm_year = _year - 1900;
        t.tm_mon  = _month - 1;
        t.tm_mday = _day;
        t.tm_hour = _hour;
        t.tm_min  = _minute;
        t.tm_sec  = _second;
        t.tm_wday = dayOfTheWeek();
        return t;
    }

    bool operator==(const DateTime &o) const {
        return _year == o._year && _month == o._month && _day == o._day
            && _hour == o._hour && _minute == o._minute && _second == o._second;
    }

private:
    uint16_t _year;
    uint8_t  _month, _day, _hour, _minute, _second;
};

// -- PCF85063 - RTClib RTC_PCF8523-compatible interface -----------------------
class PCF85063 {
public:
    PCF85063();

    // RTClib-compatible begin - takes TwoWire pointer
    bool begin(TwoWire *wire = &Wire);

    // Extended begin - takes TwoWire reference + optional SDA/SCL pins
    // Matches Waveshare BSP: rtc.begin(Wire, SDA, SCL)
    bool begin(TwoWire &wire, int sda = -1, int scl = -1);

    // -- RTClib RTC_PCF8523 interface ------------------------------------------
    DateTime now();
    void     adjust(const DateTime &dt);
    bool     initialized();     // true = clock has valid time (OS bit clear)
    bool     lostPower();       // true = clock lost power (OS bit set)  - inverse of initialized()
    void     start();
    void     stop();
    uint8_t  isrunning();       // matches RTClib spelling

    // -- Low-level -------------------------------------------------------------
    bool writeReg(uint8_t reg, uint8_t value);
    int  readReg(uint8_t reg);
    bool readRegs(uint8_t reg, uint8_t *buf, uint8_t len);

private:
    TwoWire *_wire;

    // Register addresses from PCF85063Constants.h
    static constexpr uint8_t PCF85063_ADDR    = 0x51;
    static constexpr uint8_t REG_CTRL1        = 0x00;
    static constexpr uint8_t REG_RAM          = 0x03;
    static constexpr uint8_t REG_SEC          = 0x04;
    static constexpr uint8_t REG_MIN          = 0x05;
    static constexpr uint8_t REG_HOUR         = 0x06;
    static constexpr uint8_t REG_DAY          = 0x07;
    static constexpr uint8_t REG_WEEKDAY      = 0x08;
    static constexpr uint8_t REG_MONTH        = 0x09;
    static constexpr uint8_t REG_YEAR         = 0x0A;

    static constexpr uint8_t BIT_STOP         = 5;
    static constexpr uint8_t BIT_12H          = 1;
    static constexpr uint8_t BIT_OS           = 7;   // SEC oscillator-stop flag

    static uint8_t _bcd2dec(uint8_t b) { return ((b >> 4) * 10) + (b & 0x0F); }
    static uint8_t _dec2bcd(uint8_t d) { return ((d / 10) << 4) | (d % 10);   }

    bool _initImpl();
};


#endif   // HAS_PCF85063

#endif  //  PCF85063_h
