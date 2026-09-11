/*
 * PCF85063 - Arduino/PlatformIO RTC library
 * NXP PCF85063A/TP, I2C address 0x51
 * RTClib RTC_PCF8523-compatible interface
 * Register map from Waveshare BSP PCF85063Constants.h
 *
 * Single-header, header-only version.
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

#ifndef PCF85063_HPP
#define PCF85063_HPP  1

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
        log_d("Class::DateTime unix_epoch: tm=%d _year=%d", ti->tm_year, _year);
        Serial.println(ti, "%F %T");
    }

    // DateTime(struct tm*)
    DateTime(struct tm *ti) {
        _year   = (uint16_t)(ti->tm_year + 1900);
        _month  = (uint8_t) (ti->tm_mon  + 1);
        _day    = (uint8_t)  ti->tm_mday;
        _hour   = (uint8_t)  ti->tm_hour;
        _minute = (uint8_t)  ti->tm_min;
        _second = (uint8_t)  ti->tm_sec;
        log_d("Class::DateTime struct tm: tm=%d _year=%d", ti->tm_year, _year);
        Serial.println(ti, "%F %T");
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
        log_d("Class::DateTime iso8601: %s  _year=%d", iso8601, _year);
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
        log_d("Datetime unixtime:  tm_y=%d _y=%d: ", t.tm_year, _year);
        Serial.println(&t, "%F %T");
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
        log_d("Datetime toTm tm_y=%d _y=%d: ", t.tm_year, _year);
        Serial.println(&t, "%F %T");
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
    PCF85063() : _wire(&Wire) {}

    // RTClib-compatible begin - takes TwoWire pointer
    bool begin(TwoWire *wire = &Wire) {
        _wire = wire ? wire : &Wire;
        return _initImpl();
    }

    // Extended begin - takes TwoWire reference + optional SDA/SCL pins
    // Matches Waveshare BSP: rtc.begin(Wire, SDA, SCL)
    bool begin(TwoWire &wire, int sda = -1, int scl = -1) {
        _wire = &wire;
        if (sda != -1 && scl != -1) {
            _wire->setPins(sda, scl);
        }
        _wire->begin();
        return _initImpl();
    }

    // -- RTClib RTC_PCF8523 interface ------------------------------------------

    DateTime now() {
        uint8_t buf[7];
        if (!readRegs(REG_SEC, buf, 7)) return DateTime();

        return DateTime(
            (uint16_t)_bcd2dec(buf[6])          + 2000,  // YEAR  (REG_YEAR  = 0x0A, offset 6)
            _bcd2dec(buf[5] & 0x1F),                      // MONTH (REG_MONTH = 0x09, offset 5)
            _bcd2dec(buf[3] & 0x3F),                      // DAY   (REG_DAY   = 0x07, offset 3)
            _bcd2dec(buf[2] & 0x3F),                      // HOUR  (REG_HOUR  = 0x06, offset 2)
            _bcd2dec(buf[1] & 0x7F),                      // MIN   (REG_MIN   = 0x05, offset 1)
            _bcd2dec(buf[0] & 0x7F)                       // SEC   (REG_SEC   = 0x04, offset 0)
        );
    }

    void adjust(const DateTime &dt) {
        uint8_t buf[7];
        buf[0] = _dec2bcd(dt.second())          & 0x7F;  // SEC   - clear OS bit
        buf[1] = _dec2bcd(dt.minute());                   // MIN
        buf[2] = _dec2bcd(dt.hour());                     // HOUR
        buf[3] = _dec2bcd(dt.day());                      // DAY
        buf[4] = dt.dayOfTheWeek();                       // WEEKDAY
        buf[5] = _dec2bcd(dt.month());                    // MONTH
        buf[6] = _dec2bcd((uint8_t)(dt.year() % 100));    // YEAR
        log_d("PCF85063::adjust  DateTime year bug[6]=%d, dt.year=%d", buf[6], dt.year());

        _wire->beginTransmission(PCF85063_ADDR);
        _wire->write(REG_SEC);
        _wire->write(buf, 7);
        bool ok = (_wire->endTransmission() == 0);
        log_d("PCF85063::adjust %s  %04u-%02u-%02u %02u:%02u:%02u",
              ok ? "OK" : "FAIL",
              dt.year(), dt.month(), dt.day(),
              dt.hour(), dt.minute(), dt.second());
    }

    // OS bit clear = clock has been running continuously = initialized
    bool initialized() {
        int val = readReg(REG_SEC);
        if (val < 0) return false;
        return !(val & (1 << BIT_OS));
    }

    // inverse of initialized() - mirrors RTClib lostPower()
    bool lostPower() {
        return !initialized();
    }

    void start() {
        int val = readReg(REG_CTRL1);
        if (val >= 0) writeReg(REG_CTRL1, (uint8_t)(val & ~(1 << BIT_STOP)));
    }

    void stop() {
        int val = readReg(REG_CTRL1);
        if (val >= 0) writeReg(REG_CTRL1, (uint8_t)(val | (1 << BIT_STOP)));
    }

    uint8_t isrunning() {
        int val = readReg(REG_CTRL1);
        if (val < 0) return 0;
        return !(val & (1 << BIT_STOP));
    }

    // -- Low-level -------------------------------------------------------------

    bool writeReg(uint8_t reg, uint8_t value) {
        _wire->beginTransmission(PCF85063_ADDR);
        _wire->write(reg);
        _wire->write(value);
        return (_wire->endTransmission() == 0);
    }

    int readReg(uint8_t reg) {
        uint8_t value = 0;
        _wire->beginTransmission(PCF85063_ADDR);
        _wire->write(reg);
        _wire->endTransmission(true);
        _wire->requestFrom(PCF85063_ADDR, (uint8_t)1);
        if (_wire->readBytes(&value, 1) != 1) return -1;
        return value;
    }

    bool readRegs(uint8_t reg, uint8_t *buf, uint8_t len) {
        _wire->beginTransmission(PCF85063_ADDR);
        _wire->write(reg);
        _wire->endTransmission(true);
        _wire->requestFrom(PCF85063_ADDR, len);
        return (_wire->readBytes(buf, len) == len);
    }

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

    // Shared init - mirrors SensorPCF85063::initImpl()
    bool _initImpl() {
        // Chip detection via RAM register R/W test
        int val = readReg(REG_RAM);
        if (val < 0) {
            log_e("PCF85063: not responding");
            return false;
        }
        uint8_t tmp = (uint8_t)val;

        writeReg(REG_RAM, tmp | 0x80);
        if (!(readReg(REG_RAM) & 0x80)) {
            log_e("PCF85063: RAM bit7 set failed - may be PCF8563");
            return false;
        }
        writeReg(REG_RAM, tmp & ~0x80);
        if (readReg(REG_RAM) & 0x80) {
            log_e("PCF85063: RAM bit7 clear failed");
            return false;
        }
        writeReg(REG_RAM, tmp);  // restore

        // Force 24H mode
        int ctrl1 = readReg(REG_CTRL1);
        if (ctrl1 < 0) return false;
        if (ctrl1 & (1 << BIT_12H)) {
            writeReg(REG_CTRL1, (uint8_t)(ctrl1 & ~(1 << BIT_12H)));
            log_d("PCF85063: forced 24H mode");
        }

        start();

        // begin() returns true = chip found and clock running
        // begin() returns false = chip not on bus or hardware fault
        // OS bit / time validity is the caller's responsibility via
        // initialized() / lostPower() - checked separately after begin()
        return isrunning();
    }
};

#endif  //  HAS_PCF85063
#endif  //  PCF85063_HPP
