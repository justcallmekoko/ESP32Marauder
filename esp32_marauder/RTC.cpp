#include "configs.h"

#if defined(HAS_RTC) && (defined(HAS_DS1307) || defined(HAS_PCF8523) || defined(HAS_PCF85063))

#include "RTC.h"

// -- I2C pin resolution --------------------------------------------------------
#if !defined(RTC_SDA) && defined(I2C_SDA)
  #define RTC_SDA I2C_SDA
  #define RTC_SCL I2C_SCL
#endif

#ifndef RTC_FREQ
  #define RTC_FREQ -1
#endif

// -----------------------------------------------------------------------------
// RunSetup - wire selection, pin config, begin()
// -----------------------------------------------------------------------------
void RTC::RunSetup() {
  log_d("RTC::RunSetup SDA=%d SCL=%d Freq=%d", RTC_SDA, RTC_SCL, RTC_FREQ);

  #if defined(I2C_SDA) && defined(RTC_SDA) && (RTC_SDA != I2C_SDA)
    log_i("RTC::RunSetup Using Wire1");
    _wire = &Wire1;
  #else
    log_i("RTC::RunSetup Using Wire");
    _wire = &Wire;
  #endif

  #ifdef RTC_SDA
    _wire->setPins(RTC_SDA, RTC_SCL);
    _wire->begin();
  #endif

  #if defined(RTC_FREQ) && (RTC_FREQ > 0)
    _wire->setClock(RTC_FREQ);
  #endif

  // -- begin() - all drivers take TwoWire* --------------------------------------
  // begin() returns false ONLY if chip not on bus
  // time validity (OS bit) is checked separately in setup() via initialized()
  #if defined(HAS_PCF85063)
    log_d("Looking for PCF85063");
    supported = rtclock.begin(_wire);
  #elif defined(HAS_PCF8523) || defined(HAS_DS1307)
    log_d("Looking for RTClib device");
    supported = rtclock.begin(_wire);
  #endif

  if (!supported) {
    log_e("RTC: chip not found on I2C bus");
    return;
  }

  setup();
}

// -----------------------------------------------------------------------------
// setup() - per-chip post-init
// -----------------------------------------------------------------------------

#if defined(HAS_PCF85063)

bool RTC::setup() {
  log_i("RTC::PCF85063_setup");

  // supported = chip is present on bus (already confirmed by begin())
  // rtc_synced = time is valid (OS bit clear = battery held time through power loss)
  // These are independent - chip can be present but time unset
  if (!rtclock.initialized() || rtclock.lostPower()) {
    log_w("RTC: chip found but time not set (OS bit set)");
    Serial.println(F("RTC found - time not set, needs adjust()"));
    system_time_set = false;
    rtc_synced = false;
    // supported stays true - caller can call adjust_rtc() or sync_rtc_ntp() later
  } else {
    rtc_synced = true;
    syncFromRTC();
    system_time_set = true;
    log_i("RTC: time valid, system clock synced");
  }

  Serial.println(dt_string());
  return true;  // chip found - always true here
}

#elif defined(HAS_PCF8523)

bool RTC::setup() {
  log_i("RTC::PCF8523_setup");

  if (!rtclock.initialized() || rtclock.lostPower()) {
    log_d("RTC is NOT initialized");
    Serial.println(F("RTC is NOT initialized"));
    system_time_set = false;
  } else {
    system_time_set = true;
    syncFromRTC();
    log_i("SystemTime set from RTC");
  }

  rtclock.start();
  Serial.println(dt_string());
  return supported;
}

#elif defined(HAS_DS1307)

bool RTC::setup() {
  log_i("RTC::DS1307_setup");

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.print("RTC::setup: getLocalTime=");
    Serial.println(&timeinfo, "%F %T");
  } else {
    log_w("getLocalTime Fail");
  }

  if (!rtclock.isrunning()) {
    Serial.println(F("RTC NOT initialized"));
    log_w("RTC is NOT initialized");
  } else {
    system_time_set = true;
    syncFromRTC();
    Serial.println(F("SystemTime set from RTC"));
  }

  log_d("dt_string(): %s", dt_string().c_str());
  log_i("RTC::DS1307_setup Done");
  return supported;
}

#endif

// -----------------------------------------------------------------------------
// syncFromRTC - read chip time → set ESP32 system clock
// All chips use rtclock.now() → DateTime (RTClib-compatible interface)
// -----------------------------------------------------------------------------
void RTC::syncFromRTC() {
  DateTime now = rtclock.now();
  // struct tm tm_time = now.toTm();

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

  time_t t = mktime(&tm_time);
  struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
  settimeofday(&tv, NULL);
  system_time_set = true;
  Serial.println(F("System time synced with RTC"));
}

// -----------------------------------------------------------------------------
// adjust_rtc overloads - all chips use rtclock.adjust(DateTime)
// -----------------------------------------------------------------------------

void RTC::adjust_rtc(const char *time_str) {
  rtclock.adjust(DateTime(time_str));
}

void RTC::adjust_rtc(struct tm *ti) {
  rtclock.adjust(DateTime(
    (uint16_t)(ti->tm_year + 1900),
    (uint8_t) (ti->tm_mon  + 1),
    (uint8_t)  ti->tm_mday,
    (uint8_t)  ti->tm_hour,
    (uint8_t)  ti->tm_min,
    (uint8_t)  ti->tm_sec
    ));
  }

void RTC::adjust_rtc(const DateTime &dt) {
  rtclock.adjust(dt);
}

void RTC::adjust_rtc(uint32_t t) {
  rtclock.adjust(DateTime(t));
}

// -----------------------------------------------------------------------------
// NTP sync
// -----------------------------------------------------------------------------
bool RTC::sync_rtc_ntp(const char *ntpServer) {
  if (!supported) { log_w("RTC not supported"); return false; }
  if (!wifi_scan_obj.wifi_connected) { log_w("WiFi not connected"); return false; }

  configTime(GMTOFFSET_SEC, DAYLIGHTOFFSET_SEC, ntpServer);
  delay(1000);

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    log_w("Failed to obtain time from NTP");
    return false;
  }
  system_time_set = true;
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.println(F("RTC successfully set via NTP"));

  adjust_rtc(&timeinfo);
  Serial.println(F("RTC updated with NTP time"));
  return true;
}

// -----------------------------------------------------------------------------
// Set system time from ISO 8601 string  "2026-06-11T13:26:00"
// -----------------------------------------------------------------------------
bool RTC::getSystemTimeFromString(const char *timeStr) {
  struct tm timeinfo = {};
  if (strptime(timeStr, "%Y-%m-%dT%H:%M:%S", &timeinfo) == NULL) {
    Serial.println(F("Failed to parse time string"));
    return false;
  }
  time_t epochTime = mktime(&timeinfo);
  if (epochTime == -1) {
    Serial.println(F("Failed to convert to epoch"));
    return false;
  }
  struct timeval tv = { .tv_sec = epochTime, .tv_usec = 0 };
  if (settimeofday(&tv, NULL) != 0) {
    Serial.println(F("Failed to update system time"));
    return false;
  }
  system_time_set = true;
  Serial.println(F("System time updated from string"));
  return true;
}

// -----------------------------------------------------------------------------
// dt_string / millis_dt_string
// -----------------------------------------------------------------------------
String RTC::dt_string() {
  if (!supported) { log_w("RTC not supported"); return ""; }
  DateTime now = rtclock.now();
  char buf[20];
  snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  return String(buf);
}

String RTC::millis_dt_string() {
  uint32_t ms = millis();
  uint32_t s  = (ms / 1000)     % 60;
  uint32_t m  = (ms / 60000)    % 60;
  uint32_t h  = (ms / 3600000)  % 24;
  uint32_t d  =  ms / 86400000UL;
  char buf[20];
  snprintf(buf, sizeof(buf), "%ud %02u:%02u:%02u", d, h, m, s);
  return String(buf);
}

#endif // HAS_RTC
