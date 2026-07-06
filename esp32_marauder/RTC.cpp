#include "configs.h"

#if defined(HAS_RTC) && (defined(HAS_DS1307) || defined(HAS_PCF8523))

#include "RTC.h"

// https://github.com/adafruit/RTClib


/*
Unfortunately, we can't probe to figure out which ones installed
There are ways to probe and figure it out, but I don't feel it's worth the extra  code
  DS1307_ADDRESS 0x68  with AT24C32 0x50
  DS3231_ADDRESS 0x68
  PCF8523_ADDRESS 0x68
  BM8563/PCF8563_ADDRESS 0x51 (7-bit 0x51 or 8bit 0xA2 0xA3)
*/

#if !defined(RTC_SDA) && defined(I2C_SDA)
#define RTC_SDA I2C_SDA
#define RTC_SCL I2C_SCL
#endif

// default 100000UL
#ifndef RTC_FREQ
#define RTC_FREQ -1
#endif

void RTC::RunSetup() {
  log_d("RTC::RunSetup SDA=%d SCL=%d Freq=%d", RTC_SDA, RTC_SCL, RTC_FREQ);

  #if defined(I2C_SDA) && (RTC_SDA != I2C_SDA)
    log_i("RTC::RunSetup Using Wire1");
    _wire = &Wire1;
  #else
    log_i("RTC::RunSetup Using Wire0");
    _wire = &Wire;
  #endif

  #ifdef RTC_SDA
    _wire->setPins(RTC_SDA, RTC_SCL);
    _wire->begin();
  #endif

  #if defined(RTC_FREQ) && (RTC_FREQ > 0)
    _wire->setClock(RTC_FREQ);
  #endif

  #ifdef DEVELOPER
    #if defined(HAS_PCF8523)
      log_d("Looking for PCF8523");
    #elif defined(HAS_DS1307)
      log_d("Looking for DS1307");
    #else
      log_d("Looking for ?????");
    #endif
  #endif

  supported = rtclock.begin(_wire);

  if (!supported) {
    log_w("RTC Was not detected");
    return;
  }

  setup();
}

#if defined(HAS_PCF8523)

bool RTC::setup() {
  log_i("RTC::PCF8523_setup");

  if (!rtclock.initialized() || rtclock.lostPower()) {
      log_d("RTC is NOT initialized");
      Serial.println(F("RTC is NOT initialized"));
      if (rtclock.lostPower())
        log_d("RTC lostPower");

    // rtclock.adjust(DateTime(F(__DATE__), F(__TIME__)));
    synced = false;
    // setSystemTimeFromCompile();
    // log_i("SystemTime set from Build time");
  } else {
    synced = true;
    system_time_set = true;
    syncFromRTC();
    log_i("SystemTime set from RTC");
  }

  // do this to ensure the RTC is running.
  rtclock.start();

  // log_d(dt_string());
  Serial.println(dt_string());

  return supported;
}

// float RTC::getTemperature() { return 0.0;  }

#elif defined(HAS_DS1307)
// M5StickC_Plus

bool RTC::setup() {
int error;

log_i("RTC::DS1307_setup");

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.print("RTC::setup: getLocalTime=");
    Serial.println(&timeinfo, "%F %T");
  } else {
    log_w("getLocalTime Fail");
    perror("getLocalTime");
  }

  if (!rtclock.isrunning()) {
    Serial.println(F("RTC NOT initialized"));
    Serial.flush();
    log_w("RTC is NOT initialized");
  }  else {
    synced = true;
    system_time_set = true;
    syncFromRTC();
    Serial.println(F("SystemTime set from RTC"));
  }

  // #ifdef DEVELOPER
  //   log_d(dt_string());
  // #else
    Serial.print("dt_string(): ");
    log_d("dt_string(): %s", dt_string());
  // #endif
  log_i("RTC::DS1307_setup Done");

  return supported;
}


// float RTC:getTemperature() { rtclock.getTemperature(); }

// #elif defined(HAS_BM8563)
// bool RTC::setup() {
//   log_i("RTC::BM8563_setup");
// }

#endif

void RTC::syncFromRTC() {
// 1. Read time from the PCF8523

#if defined(HAS_PCF8523) || defined(HAS_DS1307)
  DateTime now = rtclock.now();

  // 2. Populate the standard C tm structure
  struct tm tm_time;
  tm_time.tm_year = now.year() - 1900;  // Years since 1900
  tm_time.tm_mon  = now.month() - 1;    // Months (0-11)
  tm_time.tm_mday = now.day();          // Day of the month (1-31)
  tm_time.tm_hour = now.hour();         // Hours (0-23)
  tm_time.tm_min  = now.minute();       // Minutes (0-59)
  tm_time.tm_sec  = now.second();       // Seconds (0-59)

  // Set isdst to -1 to let the system decide based on your timezone settings
  tm_time.tm_isdst = -1;

  // 3. Convert tm to time_t and configure settimeofday
  time_t t = mktime(&tm_time);
  struct timeval tv = { .tv_sec = t, .tv_usec = 0 };

  settimeofday(&tv, NULL);
  synced = true;
  system_time_set = true;
  Serial.println(F("system time synced with RTC"));
// #elif defined(HAS_BM8563)
//   log_w("syncFromRTC: BM8563 not yet implemented");
#endif
}


  // Function to set system time from an ISO 8601 string (e.g., "2026-06-11T13:26:00")
  bool RTC::getSystemTimeFromString(const char* timeStr) {
      struct tm timeinfo = {0};

      // Parse the string into the tm struct using format specifiers
      // %Y = Year, %m = Month, %d = Day, %H = Hour, %M = Minute, %S = Second
      if (strptime(timeStr, "%Y-%m-%dT%H:%M:%S", &timeinfo) == NULL) {
          Serial.println(F("Failed to parse time string."));
          return false;
      }

      // Convert tm struct to Unix timestamp (seconds since Jan 1 1970)
      time_t epochTime = mktime(&timeinfo);
      if (epochTime == -1) {
          Serial.println(F("Failed to convert tm to epoch time."));
          return false;
      }

      // Structure required by settimeofday
      struct timeval tv;
      tv.tv_sec = epochTime;
      tv.tv_usec = 0;        // Microseconds

      // Apply time directly to ESP32 internal clock
      if (settimeofday(&tv, NULL) != 0) {
          Serial.println(F("Failed to update system time."));
          return false;
      }

      synced = true;
      system_time_set = true;
      Serial.println(F("System time updated successfully!"));
      return true;
  }


  bool RTC::sync_rtc_ntp(const char* ntpServer) {
    struct tm timeinfo;

    if (!supported) {
      log_w("RTC not supported");
      return false;
    }

  if (!wifi_scan_obj.wifi_connected) {
    log_w("Wifi Not Connected");
    return false;
  }

  configTime(GMTOFFSET_SEC, DAYLIGHTOFFSET_SEC, ntpServer);

  delay(1000);

  if (!getLocalTime(&timeinfo)) {
    log_w("Failed to obtain time from NTP");
    return false;
  }
  synced = true;
  system_time_set = true;

  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.println(F("RTC successfully set via NTP!"));

  time_t epoch_now;
  time(&epoch_now);
  struct tm* timeStruct = localtime(&epoch_now);

  DateTime ntpTime(
    timeStruct->tm_year + 1900,  // Year starts from 1900
    timeStruct->tm_mon + 1,      // Month (1-12)
    timeStruct->tm_mday,         // Day of the month
    timeStruct->tm_hour,         // Hour
    timeStruct->tm_min,          // Minute
    timeStruct->tm_sec           // Second
  );

    rtclock.adjust(ntpTime);
    Serial.println(F("RTC updated with NTP time."));
    // log_d("PCF8523 RTC updated with NTP time.");

  return true;
}


// template <typename T>
// void RTC::adjust_rtc(T& tm) {
//    rtclock.adjust(DateTime(tm));
// }

void RTC::adjust_rtc(const char *time_str) {
  rtclock.adjust(DateTime(time_str));
}

void RTC::adjust_rtc(struct tm *timeInfo) {
    // struct tm tmp = timeInfo;
    time_t t = mktime(timeInfo);
    rtclock.adjust(DateTime(t));
}

void RTC::adjust_rtc(const DateTime &dt) {
  rtclock.adjust(dt);
}

void RTC::adjust_rtc(uint32_t t) {
  rtclock.adjust(DateTime(t));
}

/*
void RTC::setSystemTimeFromCompile() {
    struct tm tm_build = {0};

    // Parse built-in compile time macros
    strptime(__DATE__, "%b %d %Y", &tm_build);
    strptime(__TIME__, "%T", &tm_build);

    // Set as UTC
    tm_build.tm_isdst = -1;
    time_t t_of_day = mktime(&tm_build);

    if (t_of_day == (time_t)-1) {
      log_w("setSystemTimeFromCompile: mktime failed");
      return;
    }

    // Update the system time
    struct timeval tv = { .tv_sec = t_of_day, .tv_usec = 0 };
    settimeofday(&tv, NULL);
}
*/

String RTC::dt_string() {
  if (!supported)  log_w("RTC not supported"); return "";

  DateTime now = rtclock.now();

  char format[] = "%Y-%m-%d %H:%M:%S";  // %F %T";

  Serial.println(now.toString("%F %T"));
  Serial.println(now.toString(format));


  return now.toString(format);   // one I2C read
}

String RTC::millis_dt_string() {   // punt
  uint32_t ms = millis();
  uint32_t s  = (ms / 1000) % 60;
  uint32_t m  = (ms / 60000) % 60;
  uint32_t h  = (ms / 3600000) % 24;
  uint32_t d  =  ms / 86400000UL;

  char buf[20];

  snprintf(buf, sizeof(buf), "%ud %02u:%02u:%02u", d, h, m, s);

  return String(buf);
}


#endif  // HAS_RTC

