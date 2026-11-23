#pragma once

#ifndef time_fmt_h
#define time_fmt_h

// Date/time formatting without strftime()/strptime().
// On ESP32 (classic) newlib places those in IRAM; pulling them in costs
// ~5KB of IRAM and overflows iram0_0_seg on the ESP32 boards.
// Print::println(struct tm*, fmt) calls strftime() too, so use these
// helpers instead.

#include <Arduino.h>
#include <time.h>
#include <stdio.h>

// "YYYY-MM-DD HH:MM:SS"
inline void fmt_datetime(char *buf, size_t len, const struct tm &t) {
  snprintf(buf, len, "%04d-%02d-%02d %02d:%02d:%02d",
           t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
           t.tm_hour, t.tm_min, t.tm_sec);
}

inline void print_datetime(const struct tm &t) {
  char buf[24];
  fmt_datetime(buf, sizeof(buf), t);
  Serial.println(buf);
}

// Parses "YYYY-MM-DD" and "HH:MM:SS" into tm. Returns false on bad input.
inline bool parse_datetime(const char *date, const char *time_str, struct tm *tm) {
  int Y, M, D, h, m, s;
  if (sscanf(date, "%4d-%2d-%2d", &Y, &M, &D) != 3)
    return false;
  if (sscanf(time_str, "%2d:%2d:%2d", &h, &m, &s) != 3)
    return false;
  if (M < 1 || M > 12 || D < 1 || D > 31 || h > 23 || m > 59 || s > 60)
    return false;
  *tm = {};
  tm->tm_year = Y - 1900;
  tm->tm_mon  = M - 1;
  tm->tm_mday = D;
  tm->tm_hour = h;
  tm->tm_min  = m;
  tm->tm_sec  = s;
  return true;
}

// Parses "YYYY-MM-DD HH:MM:SS"
inline bool parse_datetime(const char *str, struct tm *tm) {
  char date[16] = {0};
  char time_str[16] = {0};
  if (sscanf(str, "%15s %15s", date, time_str) != 2)
    return false;
  return parse_datetime(date, time_str, tm);
}

#endif
