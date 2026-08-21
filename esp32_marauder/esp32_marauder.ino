/*
  ESP32 Marauder - Headless / Serial CLI Edition
  Optimized for standard ESP32 plugged into PC via USB/Serial
*/

#include "configs.h"
#include <stdio.h>
#include "Assets.h"
#include "WiFiScan.h"
#include "Buffer.h"
#include "settings.h"
#include "CommandLine.h"
#include "lang_var.h"

#ifdef HAS_SD
  #include "SDInterface.h"
  SDInterface sd_obj;
#endif

#ifdef HAS_GPS
  #include "GpsInterface.h"
  GpsInterface gps_obj;
#endif

WiFiScan wifi_scan_obj;
EvilPortal evil_portal_obj;
Buffer buffer_obj;
Settings settings_obj;
CommandLine cli_obj;

const String PROGMEM version_number = MARAUDER_VERSION;
uint32_t currentTime = 0;

void setup()
{
  randomSeed(esp_random());

  #ifndef DEVELOPER
    esp_log_level_set("*", ESP_LOG_NONE);
  #endif

  #ifndef HAS_IDF_3
    esp_spiram_init();
  #endif

  Serial.begin(921600);
  Serial.setTimeout(50);

  while (!Serial && millis() < 2000)
    delay(10);

  #ifdef HAS_PSRAM
    if (!psramInit()) {
      Serial.println(F("PSRAM not available"));
    }
  #endif

  settings_obj.begin();

  const char* type = settings_obj.getSettingType("wu");
  if (type == nullptr || type[0] == '\0') {
    Serial.println(F("Installing default settings..."));
    settings_obj.createDefaultSettings(SPIFFS);
  }

  buffer_obj = Buffer();

  wifi_scan_obj.RunSetup();
  evil_portal_obj.setup();

  #ifdef HAS_GPS
    gps_obj.begin();
  #endif

  wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  cli_obj.RunSetup();
}

void loop()
{
  currentTime = millis();

  // Process CLI serial commands from PC
  cli_obj.main(currentTime);

  // Process WiFi and BLE scanning / packet operations
  wifi_scan_obj.main(currentTime);

  #ifdef HAS_GPS
    gps_obj.main();
  #endif

  // Flush capture buffer to Serial / SD
  buffer_obj.save();

  delay(1);
}
