#include <Arduino.h>
#include <time.h>

// should this be a .hpp ??

#include "WiFiScan.h"
extern WiFiScan wifi_scan_obj;

#ifdef HAS_GPS
  #ifdef HAS_GPSI2C
    #include "GpsI2c.h"
    extern GpsI2c gps_obj;
  #else
    #include "GpsInterface.h"
    extern GpsInterface gps_obj;
  #endif
#endif


#ifndef NTPSERVER
  #define NTPSERVER "pool.ntp.org"
#endif

#ifdef HAS_RTC
  #include "RTC.h"
  extern RTC rtc_obj;
#endif


// has system time been set ?
bool system_time_set = false;

// set host time
// Set RTC if install and not in sync
bool set_system_time(struct tm timeInfo, bool setrtc = false) {
  // struct tm tmp = timeInfo;
  time_t t = mktime(&timeInfo);
  if (t == (time_t)-1) {
      log_d("set_system_time: mktime failed");
      return false;
  }
  struct timeval now = { .tv_sec = t, .tv_usec = 0 };
  log_d("struct timeval now"); delay(5);
    if (settimeofday(&now, NULL) != 0) {
        log_d("settimeofday failed");
        return false;
    }
  log_d("settimeofday success"); delay(5);
    system_time_set = true;
    log_d("system time updated");

    #ifdef HAS_RTC
    if (rtc_obj.supported) {
      if (setrtc || !rtc_obj.rtc_synced) {
        log_d("set_system_time: calling rtc_obj.adjust_rtc");
        rtc_obj.adjust_rtc(timeInfo);
      }
    }
    #endif

    return true;
}

// convers date String "YYYY-MM-DD hh:mm:ss" to struct tm
bool set_system_time(const String& time_str, bool setrtc = false) {
  log_d("set_system_time time_str"); delay(5);
    struct tm tm_info = {0};
    // log_d("set_system_time: '%s'", time_str.c_str());
    if (strptime(time_str.c_str(), "%F %T", &tm_info) != NULL) {
        return set_system_time(tm_info, setrtc);
    }
    log_d("set_system_time: invalid time_str '%s'", time_str.c_str());
    return false;
}

// -----------------------------------------------------------------------------
// NTP sync
// -----------------------------------------------------------------------------
bool sync_ntp(const char *ntpServer = nullptr) {
  // if (!supported) { log_d("RTC not supported"); return false; }
  if (!wifi_scan_obj.wifi_connected) {
      log_d("WiFi not connected");
      return false;
  }

  if (ntpServer == nullptr)
    configTime(GMTOFFSET_SEC, DAYLIGHTOFFSET_SEC, "pool.ntp.org", "time.nist.gov", "1.pool.ntp.org");
  else
    configTime(GMTOFFSET_SEC, DAYLIGHTOFFSET_SEC, ntpServer);

  delay(1000);

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    log_d("Failed to obtain time from NTP");
    return false;
  }
  system_time_set = true;
  Serial.println(&timeinfo, "%F %T");  //  "%A, %B %d %Y %H:%M:%S");
  Serial.println(F("System time set via NTP"));

  #ifdef HAS_RTC
  if (rtc_obj.supported && !rtc_obj.rtc_synced) {
      log_d("set_system_time: updating RTC");
      rtc_obj.adjust_rtc(timeinfo);
  }
  #endif

  return true;
}
