#pragma once

#ifndef Settings_h
#define Settings_h

#include "configs.h"

#include "SPIFFS.h"
#include <FS.h>
#include <ArduinoJson.h>

#define FORMAT_SPIFFS_IF_FAILED true

#ifdef HAS_SCREEN
  #include "Display.h"

  extern Display display_obj;
#endif

#define WDG_KEY_NAME       "wdg_key"   // WDG Wars API key (String)

class Settings {

  private:
    String json_settings_string;

    // Flat cache populated once at begin() and kept in sync by saveSetting().
    // All loadSetting<T>() reads hit this struct — zero heap, zero JSON parse.
    struct SettingsCache {
      bool  ForceProbe  = false;
      bool  SavePCAP    = true;
      bool  EnableLED   = true;
      bool  EPDeauth    = false;
      bool  ChanHop     = false;
  #ifdef CYD_SOUND
      bool  EnableSND   = true;
  #endif
      String ClientSSID = "";
      String ClientPW   = "";
      String wu           = "";
      String wt           = "";
      String wdg_key      = "";
    } _cache;

    void _buildCache();  // parse json_settings_string -> _cache

  public:
    bool begin();

    template <typename T>
    T loadSetting(const char* key);

    template <typename T>
    T saveSetting(const char* key, bool value);

    template <typename T>
    T saveSetting(const char* key, String value);

    bool toggleSetting(const char* key);
    const char* getSettingType(const char* key);
    String setting_index_to_name(int i);
    int getNumberSettings();

    String getSettingsString();
    //bool createDefaultSettings(fs::FS &fs, bool spec = false, uint8_t index = 0, String typeStr = "bool", String name = "");
    bool createDefaultSettings(fs::FS &fs, bool spec = false, uint8_t index = 0, const char* typeStr = "bool", const char* name = "");
    void printJsonSettings(String json_string);
};

#endif
