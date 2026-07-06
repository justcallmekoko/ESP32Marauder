
#include "configs.h"

#if defined(HAS_RTC) && (defined(HAS_DS1307) || defined(HAS_PCF8523))

#ifndef rtc_h
#define rtc_h


#include <Arduino.h>
#if defined(HAS_PCF8523) || defined(HAS_DS1307)
  #include "RTClib.h"
// #elif defined(HAS_BM8563)
//   #include "I2C_BM8563.h"
#endif

#ifndef NTPSERVER
  #define NTPSERVER "pool.ntp.org"
#endif

#include <time.h>
#include <sys/time.h>

#include <Wire.h>
#include "WiFiScan.h"


extern WiFiScan wifi_scan_obj;

// If system time/date has been set
extern bool system_time_set;


/*
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -28800;      // Replace with your offset (e.g., -28800 for PST)
const int daylightOffset_sec = 3600;    // Adjust daylight savings (e.g., 3600 for DST)
*/


class RTC  {    // RTC_PCF8523

  public:

    #if defined(HAS_PCF8523)
      RTC_PCF8523 rtclock;
    #elif defined(HAS_DS1307)
      RTC_DS1307 rtclock;
//     #elif defined(HAS_BM8563)
//       RTC_BM8563 rtclock;
    #endif

    bool setup();

    void RunSetup();
    bool supported = false;
    String dt_string();
    String millis_dt_string();
    bool sync_rtc_ntp(const char* ntpServer = NTPSERVER);
    bool synced = false;


    // float getTemperature();  // PCF8523 only

    bool getSystemTimeFromString(const char* timeStr);
    // static const char* const daysOfTheWeek[] PROGMEM = {
    //  "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    //  };

    // const char* ntpServer = NTPSERVER;
    // const long gmtOffset_sec = 0;   // Always 0 for UTC
    // const int daylightOffset_sec = 0;
    void setSystemTimeFromCompile();
    void syncFromRTC();

    // template <typename T>
    // void adjust_rtc(T& tm);

    void adjust_rtc(const char *time_str);
    void adjust_rtc(struct tm *timeInfo);
    void adjust_rtc(const DateTime &dt);
    void adjust_rtc(uint32_t t);

    // helper for direct calls
    void adjust(const DateTime &dt) {
      rtclock.adjust(dt);
    }

  private:
    TwoWire *_wire;

};

// template <typename T>
// void adjust_rtc(T tm) {
//   rtclock.adjust(DateTime(tm));
// }


#endif // rtc_h

#endif // HAS_RTC
