/*!
 * @file PCF8563_RTC.hpp
 *
 * Header-only, Adafruit RTClib compatible driver for the NXP PCF8563 /
 * HYM8563 real time clock.
 *
 * The register level code is derived from the pcf8563-RTC library by
 * Chris Dirks (https://github.com/CDFER/pcf8563-RTC), which in turn derives
 * from the PCF8563_Library by Lewis he
 * (https://github.com/lewisxhe/PCF8563_Library).  The public API mirrors
 * Adafruit's RTClib (https://github.com/adafruit/RTClib) so sketches written
 * against RTClib work unchanged:
 *
 *     #include <PCF8563_RTC.hpp>
 *     RTC_PCF8563 rtc;
 *
 *     rtc.begin();
 *     rtc.initialized();   // is the stored time trustworthy?
 *     rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
 *     DateTime t = rtc.now();
 *     rtc.isrunning();
 *
 * Timer and alarm support is intentionally omitted.
 *
 * Everything is inline, so this header may be included from as many
 * translation units as you like without a matching .cpp.
 *
 * If RTClib.h is included *before* this header, RTClib's own DateTime and
 * TimeSpan classes are used and none are defined here.  Defining
 * PCF8563_RTC_EXTERNAL_DATETIME has the same effect.
 *
 * MIT License - see LICENSE.
 */
#ifndef _PCF8563_RTC_HPP_
#define _PCF8563_RTC_HPP_

#include <Arduino.h>
#include <Wire.h>
#include <time.h>

#define PCF8563_ADDRESS 0x51 ///< 7-bit I2C address of the PCF8563

#define PCF8563_CONTROL1_REG 0x00 ///< Control/status 1 (STOP bit lives here)
#define PCF8563_CONTROL2_REG 0x01 ///< Control/status 2
#define PCF8563_SEC_REG 0x02      ///< VL + seconds, first of the 7 clock regs
#define PCF8563_MIN_REG 0x03      ///< Minutes
#define PCF8563_HR_REG 0x04       ///< Hours
#define PCF8563_DAY_REG 0x05      ///< Day of month
#define PCF8563_WEEKDAY_REG 0x06  ///< Day of week
#define PCF8563_MONTH_REG 0x07    ///< Century + month
#define PCF8563_YEAR_REG 0x08     ///< Year (00-99)
#define PCF8563_SQW_REG 0x0D      ///< CLKOUT control

#define PCF8563_STOP_BIT 0x20     ///< Control1: 1 = clock halted
#define PCF8563_VOL_LOW_MASK 0x80 ///< Seconds: 1 = time integrity lost
#define PCF8563_SECONDS_MASK 0x7F ///< Seconds significant bits
#define PCF8563_MINUTES_MASK 0x7F ///< Minutes significant bits
#define PCF8563_HOUR_MASK 0x3F    ///< Hours significant bits
#define PCF8563_DAY_MASK 0x3F     ///< Day of month significant bits
#define PCF8563_WEEKDAY_MASK 0x07 ///< Day of week significant bits
#define PCF8563_MONTH_MASK 0x1F   ///< Month significant bits
#define PCF8563_CENTURY_MASK 0x80 ///< Month reg: 1 = 19xx, 0 = 20xx
#define PCF8563_CLK_ENABLE 0x80   ///< CLKOUT control: 1 = CLKOUT enabled

/** CLKOUT frequencies accepted by RTC_PCF8563::enableCLK(). */
enum Pcf8563ClkOut {
  PCF8563_CLK_32_768KHZ = 0, ///< 32.768 kHz
  PCF8563_CLK_1024KHZ = 1,   ///< 1.024 kHz
  PCF8563_CLK_32HZ = 2,      ///< 32 Hz
  PCF8563_CLK_1HZ = 3,       ///< 1 Hz
  PCF8563_CLK_MAX = 4        ///< Sentinel, not a valid frequency
};

#if defined(_RTCLIB_H_) || defined(PCF8563_RTC_EXTERNAL_DATETIME)
#define PCF8563_RTC_EXTERNAL_DATETIME_USED ///< DateTime/TimeSpan come from RTClib
#endif

#ifndef PCF8563_RTC_EXTERNAL_DATETIME_USED

/** Seconds between the Unix epoch and 2000-01-01T00:00:00. */
#define SECONDS_FROM_1970_TO_2000 946684800

/** Format selectors understood by DateTime::timestamp(). */
enum timestampOpt {
  TIMESTAMP_FULL, ///< `YYYY-MM-DDTHH:MM:SS`
  TIMESTAMP_TIME, ///< `HH:MM:SS`
  TIMESTAMP_DATE  ///< `YYYY-MM-DD`
};

class TimeSpan;

/*!
 * @brief A date and time, API compatible with RTClib's DateTime.
 */
class DateTime {
 public:
  DateTime(uint32_t t = SECONDS_FROM_1970_TO_2000);
  DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour = 0,
           uint8_t min = 0, uint8_t sec = 0);
  DateTime(const DateTime &copy);
  DateTime(const char *date, const char *time);
  DateTime(const __FlashStringHelper *date, const __FlashStringHelper *time);
  DateTime(const char *iso8601date);
  DateTime(const struct tm *timeinfo);

  /*! @brief Copy assignment. @param copy DateTime to copy.
      @return Reference to this DateTime. */
  DateTime &operator=(const DateTime &copy) = default;

  bool isValid() const;
  char *toString(char *buffer) const;

  /*! @brief Return the year. @return Year, e.g. 2026. */
  uint16_t year() const { return 2000U + yOff; }
  /*! @brief Return the month. @return Month, 1-12. */
  uint8_t month() const { return m; }
  /*! @brief Return the day of the month. @return Day, 1-31. */
  uint8_t day() const { return d; }
  /*! @brief Return the hour. @return Hour, 0-23. */
  uint8_t hour() const { return hh; }
  uint8_t twelveHour() const;
  /*! @brief Return whether the hour is PM. @return true if hour >= 12. */
  uint8_t isPM() const { return hh >= 12; }
  /*! @brief Return the minute. @return Minute, 0-59. */
  uint8_t minute() const { return mm; }
  /*! @brief Return the second. @return Second, 0-59. */
  uint8_t second() const { return ss; }

  uint8_t dayOfTheWeek() const;
  uint32_t secondstime() const;
  uint32_t unixtime() const;
  struct tm toTm() const;

  String timestamp(timestampOpt opt = TIMESTAMP_FULL) const;

  DateTime operator+(const TimeSpan &span) const;
  DateTime operator-(const TimeSpan &span) const;
  TimeSpan operator-(const DateTime &right) const;
  bool operator<(const DateTime &right) const;
  /*! @brief Test if this is later than `right`. @param right Other DateTime.
      @return true if later. */
  bool operator>(const DateTime &right) const { return right < *this; }
  /*! @brief Test if this is not later than `right`. @param right Other
      DateTime. @return true if earlier or equal. */
  bool operator<=(const DateTime &right) const { return !(*this > right); }
  /*! @brief Test if this is not earlier than `right`. @param right Other
      DateTime. @return true if later or equal. */
  bool operator>=(const DateTime &right) const { return !(*this < right); }
  bool operator==(const DateTime &right) const;
  /*! @brief Test if two DateTimes differ. @param right Other DateTime.
      @return true if they differ. */
  bool operator!=(const DateTime &right) const { return !(*this == right); }

protected:
  uint8_t yOff; ///< Year offset from 2000
  uint8_t m;    ///< Month 1-12
  uint8_t d;    ///< Day 1-31
  uint8_t hh;   ///< Hours 0-23
  uint8_t mm;   ///< Minutes 0-59
  uint8_t ss;   ///< Seconds 0-59
};

/*!
 * @brief A signed duration, API compatible with RTClib's TimeSpan.
 */
class TimeSpan {
public:
  TimeSpan(int32_t seconds = 0);
  TimeSpan(int16_t days, int8_t hours, int8_t minutes, int8_t seconds);
  TimeSpan(const TimeSpan &copy);

  /*! @brief Whole days in the span. @return Number of days. */
  int16_t days() const { return _seconds / 86400L; }
  /*! @brief Hours past the whole days. @return Hours, -23 to 23. */
  int8_t hours() const { return _seconds / 3600 % 24; }
  /*! @brief Minutes past the whole hours. @return Minutes, -59 to 59. */
  int8_t minutes() const { return _seconds / 60 % 60; }
  /*! @brief Seconds past the whole minutes. @return Seconds, -59 to 59. */
  int8_t seconds() const { return _seconds % 60; }
  /*! @brief Total length of the span. @return Number of seconds. */
  int32_t totalseconds() const { return _seconds; }

  TimeSpan operator+(const TimeSpan &right) const;
  TimeSpan operator-(const TimeSpan &right) const;

protected:
  int32_t _seconds; ///< Length of the span in seconds
};

#endif // PCF8563_RTC_EXTERNAL_DATETIME_USED

/*!
 * @brief RTClib style driver for the PCF8563 / HYM8563.
 */
class RTC_PCF8563 {
public:
  bool begin(TwoWire *wireInstance = &Wire, uint8_t addr = PCF8563_ADDRESS);
  bool begin(TwoWire &wireInstance, int sda = -1, int scl = -1,
             uint8_t addr = PCF8563_ADDRESS);
  bool initialized();
  /*! @brief Inverse of initialized(): the chip lost its time.
      @return true if the stored time cannot be trusted. */
  bool lostPower() { return !initialized(); }
  bool isrunning();
  void start();
  void stop();
  void adjust(const DateTime &dt);
  DateTime now();

  bool enableCLK(uint8_t freq);
  void disableCLK();

  // Raw register access, for callers that need to reach the chip directly.
  /*! @brief Write one register. @param reg Register address.
      @param value Value to write. @return true on success. */
  bool writeReg(uint8_t reg, uint8_t value) {
    return writeRegisters(reg, &value, 1);
  }
  /*! @brief Read one register. @param reg Register address.
      @return The register value, or -1 if the read failed. */
  int readReg(uint8_t reg) {
    uint8_t value = 0;
    if (!readRegisters(reg, &value, 1))
      return -1;
    return value;
  }
  /*! @brief Read consecutive registers. @param reg First register address.
      @param buf Buffer receiving the data. @param len Number of bytes.
      @return true on success. */
  bool readRegs(uint8_t reg, uint8_t *buf, uint8_t len) {
    return readRegisters(reg, buf, len);
  }

private:
  TwoWire *_wire = &Wire;          ///< I2C bus in use
  uint8_t _addr = PCF8563_ADDRESS; ///< I2C address in use

  bool readRegisters(uint8_t reg, uint8_t *buf, uint8_t len);
  uint8_t readRegister(uint8_t reg);
  bool writeRegisters(uint8_t reg, const uint8_t *buf, uint8_t len);
  /*! @brief Write a single register. @param reg Register address.
      @param val Value to write. @return true on success. */
  bool writeRegister(uint8_t reg, uint8_t val) {
    return writeRegisters(reg, &val, 1);
  }

  /*! @brief Convert a BCD value to binary. @param val BCD value.
      @return Binary value. */
  static uint8_t bcd2bin(uint8_t val) { return val - 6 * (val >> 4); }
  /*! @brief Convert a binary value to BCD. @param val Binary value.
      @return BCD value. */
  static uint8_t bin2bcd(uint8_t val) { return val + 6 * (val / 10); }
};

/////////////////////////////////////////////////////////////////////////////
// Implementation
/////////////////////////////////////////////////////////////////////////////

#ifndef PCF8563_RTC_EXTERNAL_DATETIME_USED

/** Number of days in each month of a non-leap year. */
const uint8_t pcf8563_days_in_month[] PROGMEM = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/*!
    @brief  Number of days since 2000-01-01 for a given date.
    @param y Year, either the full year or the offset from 2000.
    @param m Month, 1-12.
    @param d Day of the month, 1-31.
    @return Number of days.
*/
inline uint16_t pcf8563_date2days(uint16_t y, uint8_t m, uint8_t d) {
  if (y >= 2000U)
    y -= 2000U;
  uint16_t days = d;
  for (uint8_t i = 1; i < m; ++i)
    days += pgm_read_byte(pcf8563_days_in_month + i - 1);
  if (m > 2 && y % 4 == 0)
    ++days;
  return days + 365 * y + (y + 3) / 4 - 1;
}

/*!
    @brief  Convert a number of days and a time of day to seconds.
    @param days Number of days.
    @param h Hours.
    @param m Minutes.
    @param s Seconds.
    @return Total number of seconds.
*/
inline uint32_t pcf8563_time2ulong(uint16_t days, uint8_t h, uint8_t m,
                                   uint8_t s) {
  return ((days * 24UL + h) * 60 + m) * 60 + s;
}

/*!
    @brief  Parse a two digit decimal number, treating a leading space or
            other non-digit as a zero.
    @param p Pointer to the two characters.
    @return The parsed value.
*/
inline uint8_t pcf8563_conv2d(const char *p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';
}

/*!
    @brief  Construct a DateTime from a Unix timestamp.
    @param t Seconds since 1970-01-01T00:00:00 (UTC).
*/
inline DateTime::DateTime(uint32_t t) {
  t -= SECONDS_FROM_1970_TO_2000;

  ss = t % 60;
  t /= 60;
  mm = t % 60;
  t /= 60;
  hh = t % 24;
  uint16_t days = t / 24;
  uint8_t leap;
  for (yOff = 0;; ++yOff) {
    leap = yOff % 4 == 0;
    if (days < 365U + leap)
      break;
    days -= 365U + leap;
  }
  for (m = 1; m < 12; ++m) {
    uint8_t daysPerMonth = pgm_read_byte(pcf8563_days_in_month + m - 1);
    if (leap && m == 2)
      ++daysPerMonth;
    if (days < daysPerMonth)
      break;
    days -= daysPerMonth;
  }
  d = days + 1;
}

/*!
    @brief  Construct a DateTime from its components.
    @param year Year, either the full year or the offset from 2000.
    @param month Month, 1-12.
    @param day Day of the month, 1-31.
    @param hour Hour, 0-23.
    @param min Minute, 0-59.
    @param sec Second, 0-59.
*/
inline DateTime::DateTime(uint16_t year, uint8_t month, uint8_t day,
                          uint8_t hour, uint8_t min, uint8_t sec) {
  if (year >= 2000U)
    year -= 2000U;
  yOff = year;
  m = month;
  d = day;
  hh = hour;
  mm = min;
  ss = sec;
}

/*!
    @brief  Copy constructor.
    @param copy DateTime to copy.
*/
inline DateTime::DateTime(const DateTime &copy)
    : yOff(copy.yOff), m(copy.m), d(copy.d), hh(copy.hh), mm(copy.mm),
      ss(copy.ss) {}

/*!
    @brief  Construct from date and time strings in the format produced by
            the `__DATE__` and `__TIME__` macros, e.g.
            `DateTime(__DATE__, __TIME__)`.
    @param date Date string, "MMM DD YYYY".
    @param time Time string, "HH:MM:SS".
*/
inline DateTime::DateTime(const char *date, const char *time) {
  yOff = pcf8563_conv2d(date + 9);
  switch (date[0]) {
  case 'J':
    m = (date[1] == 'a') ? 1 : ((date[2] == 'n') ? 6 : 7);
    break;
  case 'F':
    m = 2;
    break;
  case 'A':
    m = (date[2] == 'r') ? 4 : 8;
    break;
  case 'M':
    m = (date[2] == 'r') ? 3 : 5;
    break;
  case 'S':
    m = 9;
    break;
  case 'O':
    m = 10;
    break;
  case 'N':
    m = 11;
    break;
  case 'D':
    m = 12;
    break;
  default:
    m = 0;
    break;
  }
  d = pcf8563_conv2d(date + 4);
  hh = pcf8563_conv2d(time);
  mm = pcf8563_conv2d(time + 3);
  ss = pcf8563_conv2d(time + 6);
}

/*!
    @brief  Construct from date and time strings held in flash, e.g.
            `DateTime(F(__DATE__), F(__TIME__))`.
    @param date Date string, "MMM DD YYYY".
    @param time Time string, "HH:MM:SS".
*/
inline DateTime::DateTime(const __FlashStringHelper *date,
                          const __FlashStringHelper *time) {
  char buff[11];
  memcpy_P(buff, date, 11);
  buff[10] = '\0';
  char timeBuff[9];
  memcpy_P(timeBuff, time, 9);
  timeBuff[8] = '\0';
  *this = DateTime(buff, timeBuff);
}

/*!
    @brief  Construct from an ISO8601 string such as
            "2026-09-22T17:30:00" (the separator may be any character).
    @param iso8601dateTime The string to parse.
*/
inline DateTime::DateTime(const char *iso8601dateTime) {
  char ref[] = "2000-01-01T00:00:00";
  memcpy(ref, iso8601dateTime, min(strlen(iso8601dateTime), sizeof(ref) - 1));
  ref[sizeof(ref) - 1] = '\0';
  yOff = pcf8563_conv2d(ref + 2);
  m = pcf8563_conv2d(ref + 5);
  d = pcf8563_conv2d(ref + 8);
  hh = pcf8563_conv2d(ref + 11);
  mm = pcf8563_conv2d(ref + 14);
  ss = pcf8563_conv2d(ref + 17);
}

/*!
    @brief  Construct from a standard C `struct tm`.  The `tm_wday`,
            `tm_yday` and `tm_isdst` fields are ignored; the rest are taken
            as a UTC wall clock reading.
    @param timeinfo The broken down time to copy.
*/
inline DateTime::DateTime(const struct tm *timeinfo) {
  if (!timeinfo) {
    *this = DateTime(2000U, 1, 1, 0, 0, 0);
    return;
  }
  int y = timeinfo->tm_year + 1900;
  yOff = (y >= 2000) ? (uint8_t)(y - 2000) : 0;
  m = (uint8_t)(timeinfo->tm_mon + 1);
  d = (uint8_t)timeinfo->tm_mday;
  hh = (uint8_t)timeinfo->tm_hour;
  mm = (uint8_t)timeinfo->tm_min;
  ss = (uint8_t)timeinfo->tm_sec;
}

/*!
    @brief  Check whether this object holds a valid date and time.
    @return true if the date and time are in range.
*/
inline bool DateTime::isValid() const {
  if (yOff >= 100 || m < 1 || m > 12 || d < 1 || hh > 23 || mm > 59 || ss > 59)
    return false;
  uint8_t daysPerMonth = pgm_read_byte(pcf8563_days_in_month + m - 1);
  if (m == 2 && yOff % 4 == 0)
    ++daysPerMonth;
  return d <= daysPerMonth;
}

/*!
    @brief  Format this date and time into a buffer that holds a format
            string on entry, RTClib style.  Supported tokens are YYYY, YY,
            MMMM, MMM, MM, DDDD, DDD, DD, hh, mm, ss and AP/ap.
    @param buffer In: the format string.  Out: the formatted result.
    @return The buffer, for chaining.
*/
inline char *DateTime::toString(char *buffer) const {
  uint8_t apTag =
      (strstr(buffer, "ap") != nullptr) || (strstr(buffer, "AP") != nullptr);
  for (size_t i = 0; i < strlen(buffer) - 1; i++) {
    if (buffer[i] == 'h' && buffer[i + 1] == 'h') {
      if (!apTag) {
        buffer[i] = '0' + hh / 10;
        buffer[i + 1] = '0' + hh % 10;
      } else {
        uint8_t hour = twelveHour();
        buffer[i] = '0' + hour / 10;
        buffer[i + 1] = '0' + hour % 10;
      }
      i++;
    } else if (buffer[i] == 'm' && buffer[i + 1] == 'm') {
      buffer[i] = '0' + mm / 10;
      buffer[i + 1] = '0' + mm % 10;
      i++;
    } else if (buffer[i] == 's' && buffer[i + 1] == 's') {
      buffer[i] = '0' + ss / 10;
      buffer[i + 1] = '0' + ss % 10;
      i++;
    } else if (buffer[i] == 'D' && buffer[i + 1] == 'D' &&
               buffer[i + 2] == 'D' && buffer[i + 3] == 'D') {
      static PROGMEM const char day_names[] =
          "SundayMondayTuesdayWednesdayThursdayFridaySaturday";
      static PROGMEM const uint8_t day_offsets[] = {0, 6, 12, 19, 28, 36, 42};
      static PROGMEM const uint8_t day_lengths[] = {6, 6, 7, 9, 8, 6, 8};
      uint8_t dow = dayOfTheWeek();
      uint8_t offset = pgm_read_byte(day_offsets + dow);
      uint8_t length = pgm_read_byte(day_lengths + dow);
      memmove(buffer + i + length, buffer + i + 4, strlen(buffer) - i - 3);
      memcpy_P(buffer + i, day_names + offset, length);
      i += length - 1;
    } else if (buffer[i] == 'D' && buffer[i + 1] == 'D' &&
               buffer[i + 2] == 'D') {
      static PROGMEM const char day_names[] = "SunMonTueWedThuFriSat";
      memcpy_P(buffer + i, day_names + dayOfTheWeek() * 3, 3);
      i += 2;
    } else if (buffer[i] == 'D' && buffer[i + 1] == 'D') {
      buffer[i] = '0' + d / 10;
      buffer[i + 1] = '0' + d % 10;
      i++;
    } else if (buffer[i] == 'M' && buffer[i + 1] == 'M' &&
               buffer[i + 2] == 'M' && buffer[i + 3] == 'M') {
      static PROGMEM const char month_names[] =
          "JanuaryFebruaryMarchAprilMayJuneJulyAugustSeptemberOctoberNovember"
          "December";
      static PROGMEM const uint8_t month_offsets[] = {0,  7,  15, 20, 25, 28,
                                                      32, 36, 42, 51, 58, 66};
      static PROGMEM const uint8_t month_lengths[] = {7, 8, 5, 5, 3, 4,
                                                      4, 6, 9, 7, 8, 8};
      uint8_t offset = pgm_read_byte(month_offsets + m - 1);
      uint8_t length = pgm_read_byte(month_lengths + m - 1);
      memmove(buffer + i + length, buffer + i + 4, strlen(buffer) - i - 3);
      memcpy_P(buffer + i, month_names + offset, length);
      i += length - 1;
    } else if (buffer[i] == 'M' && buffer[i + 1] == 'M' &&
               buffer[i + 2] == 'M') {
      static PROGMEM const char month_names[] =
          "JanFebMarAprMayJunJulAugSepOctNovDec";
      memcpy_P(buffer + i, month_names + (m - 1) * 3, 3);
      i += 2;
    } else if (buffer[i] == 'M' && buffer[i + 1] == 'M') {
      buffer[i] = '0' + m / 10;
      buffer[i + 1] = '0' + m % 10;
      i++;
    } else if (buffer[i] == 'Y' && buffer[i + 1] == 'Y' &&
               buffer[i + 2] == 'Y' && buffer[i + 3] == 'Y') {
      buffer[i] = '2';
      buffer[i + 1] = '0';
      buffer[i + 2] = '0' + yOff / 10;
      buffer[i + 3] = '0' + yOff % 10;
      i += 3;
    } else if (buffer[i] == 'Y' && buffer[i + 1] == 'Y') {
      buffer[i] = '0' + yOff / 10;
      buffer[i + 1] = '0' + yOff % 10;
      i++;
    } else if (buffer[i] == 'A' && buffer[i + 1] == 'P') {
      buffer[i] = isPM() ? 'P' : 'A';
      buffer[i + 1] = 'M';
      i++;
    } else if (buffer[i] == 'a' && buffer[i + 1] == 'p') {
      buffer[i] = isPM() ? 'p' : 'a';
      buffer[i + 1] = 'm';
      i++;
    }
  }
  return buffer;
}

/*!
    @brief  Return the hour in 12-hour form.
    @return Hour, 1-12.
*/
inline uint8_t DateTime::twelveHour() const {
  if (hh == 0 || hh == 12)
    return 12;
  return hh % 12;
}

/*!
    @brief  Return the day of the week.
    @return Day of week, 0 = Sunday through 6 = Saturday.
*/
inline uint8_t DateTime::dayOfTheWeek() const {
  uint16_t day = pcf8563_date2days(yOff, m, d);
  return (day + 6) % 7; // 2000-01-01 was a Saturday
}

/*!
    @brief  Return the time as seconds since 2000-01-01T00:00:00.
    @return Number of seconds.
*/
inline uint32_t DateTime::secondstime() const {
  return pcf8563_time2ulong(pcf8563_date2days(yOff, m, d), hh, mm, ss);
}

/*!
    @brief  Return the time as seconds since 1970-01-01T00:00:00 (UTC).
    @return Number of seconds.
*/
inline uint32_t DateTime::unixtime() const {
  return secondstime() + SECONDS_FROM_1970_TO_2000;
}

/*!
    @brief  Convert to a standard C `struct tm`, with `tm_wday`, `tm_yday`
            and `tm_isdst` filled in.  `tm_isdst` is set to -1 so that
            `mktime()` resolves DST itself; note that `mktime()` interprets
            the result in the local timezone, while this DateTime holds a
            UTC reading, so prefer unixtime() unless you want that
            conversion.
    @return The broken down time.
*/
inline struct tm DateTime::toTm() const {
  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_year = static_cast<int>(year() - 1900);
  t.tm_mon = m - 1;
  t.tm_mday = d;
  t.tm_hour = hh;
  t.tm_min = mm;
  t.tm_sec = ss;
  t.tm_wday = dayOfTheWeek();
  t.tm_yday = pcf8563_date2days(yOff, m, d) - pcf8563_date2days(yOff, 1, 1);
  t.tm_isdst = -1;
  return t;
}

/*!
    @brief  Format the date and time as an ISO8601 style String.
    @param opt Which part of the timestamp to return.
    @return The formatted String.
*/
inline String DateTime::timestamp(timestampOpt opt) const {
  char buffer[25];
  switch (opt) {
  case TIMESTAMP_TIME:
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hh, mm, ss);
    break;
  case TIMESTAMP_DATE:
    snprintf(buffer, sizeof(buffer), "%u-%02d-%02d", 2000U + yOff, m, d);
    break;
  default:
    snprintf(buffer, sizeof(buffer), "%u-%02d-%02dT%02d:%02d:%02d", 2000U + yOff, m, d, hh, mm,
            ss);
  }
  return String(buffer);
}

/*!
    @brief  Add a TimeSpan.
    @param span The span to add.
    @return The resulting DateTime.
*/
inline DateTime DateTime::operator+(const TimeSpan &span) const {
  return DateTime(unixtime() + span.totalseconds());
}

/*!
    @brief  Subtract a TimeSpan.
    @param span The span to subtract.
    @return The resulting DateTime.
*/
inline DateTime DateTime::operator-(const TimeSpan &span) const {
  return DateTime(unixtime() - span.totalseconds());
}

/*!
    @brief  Difference between two DateTimes.
    @param right The DateTime to subtract from this one.
    @return The elapsed TimeSpan.
*/
inline TimeSpan DateTime::operator-(const DateTime &right) const {
  return TimeSpan(unixtime() - right.unixtime());
}

/*!
    @brief  Test if this is earlier than `right`.
    @param right The DateTime to compare with.
    @return true if this is earlier.
*/
inline bool DateTime::operator<(const DateTime &right) const {
  return (yOff + 2000U < right.year() ||
          (yOff + 2000U == right.year() &&
           (m < right.month() ||
            (m == right.month() &&
             (d < right.day() ||
              (d == right.day() &&
               (hh < right.hour() ||
                (hh == right.hour() &&
                 (mm < right.minute() ||
                  (mm == right.minute() && ss < right.second()))))))))));
}

/*!
    @brief  Test if two DateTimes are equal.
    @param right The DateTime to compare with.
    @return true if they are equal.
*/
inline bool DateTime::operator==(const DateTime &right) const {
  return (right.year() == yOff + 2000U && right.month() == m &&
          right.day() == d && right.hour() == hh && right.minute() == mm &&
          right.second() == ss);
}

/*!
    @brief  Construct a TimeSpan from a number of seconds.
    @param seconds Number of seconds, may be negative.
*/
inline TimeSpan::TimeSpan(int32_t seconds) : _seconds(seconds) {}

/*!
    @brief  Construct a TimeSpan from its components.
    @param days Number of days.
    @param hours Number of hours.
    @param minutes Number of minutes.
    @param seconds Number of seconds.
*/
inline TimeSpan::TimeSpan(int16_t days, int8_t hours, int8_t minutes,
                          int8_t seconds)
    : _seconds(static_cast<int32_t>(days) * 86400L + static_cast<int32_t>(hours) * 3600 +
               static_cast<int32_t>(minutes) * 60 + seconds) {}

/*!
    @brief  Copy constructor.
    @param copy TimeSpan to copy.
*/
inline TimeSpan::TimeSpan(const TimeSpan &copy) : _seconds(copy._seconds) {}

/*!
    @brief  Add two TimeSpans.
    @param right The span to add.
    @return The resulting TimeSpan.
*/
inline TimeSpan TimeSpan::operator+(const TimeSpan &right) const {
  return TimeSpan(_seconds + right._seconds);
}

/*!
    @brief  Subtract two TimeSpans.
    @param right The span to subtract.
    @return The resulting TimeSpan.
*/
inline TimeSpan TimeSpan::operator-(const TimeSpan &right) const {
  return TimeSpan(_seconds - right._seconds);
}

#endif // PCF8563_RTC_EXTERNAL_DATETIME_USED

/*!
    @brief  Read a block of consecutive registers.
    @param reg Address of the first register.
    @param buf Buffer receiving the data.
    @param len Number of bytes to read.
    @return true if the expected number of bytes was received.
*/
inline bool RTC_PCF8563::readRegisters(uint8_t reg, uint8_t *buf,
                                       uint8_t len) {
  _wire->beginTransmission(_addr);
  _wire->write(reg);
  // The HYM8563 needs a repeated start here rather than a stop.
  if (_wire->endTransmission(false) != 0)
    return false;
  if (_wire->requestFrom(_addr, len, (uint8_t) true) != len)
    return false;
  for (uint8_t i = 0; i < len; i++)
    buf[i] = _wire->read();
  return true;
}

/*!
    @brief  Read a single register.
    @param reg Register address.
    @return The register value, or 0 if the read failed.
*/
inline uint8_t RTC_PCF8563::readRegister(uint8_t reg) {
  uint8_t val = 0;
  readRegisters(reg, &val, 1);
  return val;
}

/*!
    @brief  Write a block of consecutive registers.
    @param reg Address of the first register.
    @param buf Buffer holding the data.
    @param len Number of bytes to write.
    @return true on success.
*/
inline bool RTC_PCF8563::writeRegisters(uint8_t reg, const uint8_t *buf,
                                        uint8_t len) {
  _wire->beginTransmission(_addr);
  _wire->write(reg);
  for (uint8_t i = 0; i < len; i++)
    _wire->write(buf[i]);
  return _wire->endTransmission() == 0;
}

/*!
    @brief  Start the driver and check that the chip answers.
    @param wireInstance The I2C bus to use, Wire by default.  Wire.begin()
           must already have been called.
    @param addr The I2C address of the chip.
    @return true if the chip acknowledged its address.
*/
inline bool RTC_PCF8563::begin(TwoWire *wireInstance, uint8_t addr) {
  _wire = wireInstance;
  _addr = addr;
  _wire->beginTransmission(_addr);
  return _wire->endTransmission() == 0;
}

/*!
    @brief  Start the driver on an I2C bus given by reference, optionally
            bringing that bus up on the given pins first.  Unlike the
            pointer form this calls `begin()` on the bus for you.
    @param wireInstance The I2C bus to use.
    @param sda SDA pin, or -1 to leave the bus pins alone.
    @param scl SCL pin, or -1 to leave the bus pins alone.
    @param addr The I2C address of the chip.
    @return true if the chip acknowledged its address.
*/
inline bool RTC_PCF8563::begin(TwoWire &wireInstance, int sda, int scl,
                               uint8_t addr) {
  _wire = &wireInstance;
  _addr = addr;
  if (sda >= 0 && scl >= 0) {
#if defined(ARDUINO_ARCH_ESP32)
    _wire->setPins(sda, scl);
    _wire->begin();
#else
    _wire->begin(sda, scl);
#endif
  } else {
    _wire->begin();
  }
  _wire->beginTransmission(_addr);
  return _wire->endTransmission() == 0;
}

/*!
    @brief  Check whether the stored time can be trusted.

    The PCF8563 sets the VL (voltage low) flag when Vdd has dropped far
    enough that the clock may have lost counts.  The flag is cleared by
    adjust().

    @return true if the time is valid, false if it should be set again.
*/
inline bool RTC_PCF8563::initialized() {
  return !(readRegister(PCF8563_SEC_REG) & PCF8563_VOL_LOW_MASK);
}

/*!
    @brief  Check whether the clock is counting.
    @return true unless the STOP bit in Control_1 is set.
*/
inline bool RTC_PCF8563::isrunning() {
  return !(readRegister(PCF8563_CONTROL1_REG) & PCF8563_STOP_BIT);
}

/*!
    @brief  Resume counting by clearing the STOP bit.
*/
inline void RTC_PCF8563::start() {
  uint8_t ctrl1 = readRegister(PCF8563_CONTROL1_REG);
  if (ctrl1 & PCF8563_STOP_BIT)
    writeRegister(PCF8563_CONTROL1_REG, ctrl1 & ~PCF8563_STOP_BIT);
}

/*!
    @brief  Halt counting by setting the STOP bit.
*/
inline void RTC_PCF8563::stop() {
  uint8_t ctrl1 = readRegister(PCF8563_CONTROL1_REG);
  if (!(ctrl1 & PCF8563_STOP_BIT))
    writeRegister(PCF8563_CONTROL1_REG, ctrl1 | PCF8563_STOP_BIT);
}

/*!
    @brief  Set the date and time, clearing the VL flag so that
            initialized() reports the time as valid.
    @param dt The date and time to store.
*/
inline void RTC_PCF8563::adjust(const DateTime &dt) {
  uint8_t ctrl1 = readRegister(PCF8563_CONTROL1_REG);

  // Halt the clock so the seven registers update as one consistent set.
  writeRegister(PCF8563_CONTROL1_REG, ctrl1 | PCF8563_STOP_BIT);

  uint16_t year = dt.year();
  uint8_t buf[7];
  buf[0] = bin2bcd(dt.second()) & PCF8563_SECONDS_MASK; // clears VL
  buf[1] = bin2bcd(dt.minute());
  buf[2] = bin2bcd(dt.hour());
  buf[3] = bin2bcd(dt.day());
  buf[4] = dt.dayOfTheWeek(); // 0-6, not BCD encoded
  buf[5] = bin2bcd(dt.month());
  // The century bit means 19xx when set, 20xx when clear.
  if (year < 2000U)
    buf[5] |= PCF8563_CENTURY_MASK;
  buf[6] = bin2bcd(year % 100U);
  writeRegisters(PCF8563_SEC_REG, buf, sizeof(buf));

  // Restore the previous run state, leaving the clock running by default.
  writeRegister(PCF8563_CONTROL1_REG, ctrl1 & ~PCF8563_STOP_BIT);
}

/*!
    @brief  Read the current date and time.
    @return The DateTime held by the chip.  If the read fails the returned
            DateTime is 2000-01-01T00:00:00, for which isValid() is true but
            which no working clock will report in practice.
*/
inline DateTime RTC_PCF8563::now() {
  uint8_t buf[7];
  if (!readRegisters(PCF8563_SEC_REG, buf, sizeof(buf)))
    return DateTime(2000U, 1, 1, 0, 0, 0);

  uint8_t ss = bcd2bin(buf[0] & PCF8563_SECONDS_MASK);
  uint8_t mm = bcd2bin(buf[1] & PCF8563_MINUTES_MASK);
  uint8_t hh = bcd2bin(buf[2] & PCF8563_HOUR_MASK);
  uint8_t d = bcd2bin(buf[3] & PCF8563_DAY_MASK);
  uint8_t m = bcd2bin(buf[5] & PCF8563_MONTH_MASK);
  // Century bit set means 19xx, clear means 20xx.
  uint16_t y = bcd2bin(buf[6]) + ((buf[5] & PCF8563_CENTURY_MASK) ? 1900U
                                                                  : 2000U);
  return DateTime(y, m, d, hh, mm, ss);
}

/*!
    @brief  Enable the CLKOUT pin at the given frequency.
    @param freq One of the Pcf8563ClkOut values.
    @return true if the frequency was valid and was written.
*/
inline bool RTC_PCF8563::enableCLK(uint8_t freq) {
  if (freq >= PCF8563_CLK_MAX)
    return false;
  return writeRegister(PCF8563_SQW_REG, freq | PCF8563_CLK_ENABLE);
}

/*!
    @brief  Disable the CLKOUT pin, which drops the quiescent current from
            about 550nA to about 250nA.
*/
inline void RTC_PCF8563::disableCLK() {
  writeRegister(PCF8563_SQW_REG, 0x00);
}

#endif    // _PCF8563_RTC_HPP_
