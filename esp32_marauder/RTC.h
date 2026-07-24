#include "configs.h"

#if defined(HAS_RTC) && (defined(HAS_DS1307) || defined(HAS_PCF8523) || defined(HAS_PCF85063))

#ifndef rtc_h
#define rtc_h

#include <Arduino.h>
#include <Wire.h>
#include <time.h>
#include <sys/time.h>

// -- RTClib path (PCF8523 / DS1307) -------------------------------------------
#if defined(HAS_PCF8523) || defined(HAS_DS1307)
  #include "RTClib.h"
#endif

// -- PCF85063 driver - RTClib-compatible interface -----------------------------
#if defined(HAS_PCF85063)
  #include "PCF85063.h"
#endif

#ifndef NTPSERVER
  #define NTPSERVER "pool.ntp.org"
#endif

#include "WiFiScan.h"
extern WiFiScan wifi_scan_obj;
extern bool system_time_set;

// -- RTC class -----------------------------------------------------------------
class RTC {
public:

  // -- Hardware object - one per build target ----------------------------------
  #if defined(HAS_PCF8523)
    RTC_PCF8523  rtclock;
  #elif defined(HAS_DS1307)
    RTC_DS1307   rtclock;
  #elif defined(HAS_PCF85063)
    PCF85063     rtclock;
  #endif

  bool   supported = false;
  bool   rtc_synced = false;

  void   RunSetup();
  bool   setup();

  // -- Time string helpers -----------------------------------------------------
  String dt_string();
  String millis_dt_string();

  // -- Sync -------------------------------------------------------------------
  void syncFromRTC();
  bool sync_rtc_ntp(const char *ntpServer = NTPSERVER);
  bool getSystemTimeFromString(const char *timeStr);
  void setSystemTimeFromCompile();

  // -- Adjust overloads - all chips use DateTime now --------------------------
  void adjust_rtc(const char *time_str);   // ISO8601 string
  void adjust_rtc(struct tm *timeInfo);
  void adjust_rtc(const DateTime &dt);
  void adjust_rtc(uint32_t t);             // Unix epoch
  void adjust(const DateTime &dt) { rtclock.adjust(dt); }

private:
  TwoWire *_wire = nullptr;
};

#endif // rtc_h
#endif // HAS_RTC
