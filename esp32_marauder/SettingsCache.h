#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// Flat, hardware-decoupled mirror of the settings that loadSetting<T>() reads
// on the hot path. Extracted from Settings so the parse/sync logic can be unit
// tested natively (see test/test_settings_cache) without SPIFFS, Serial or a
// live filesystem. Field defaults mirror the settings JSON defaults.
struct SettingsCache {
  bool   ForcePMKID = false;
  bool   ForceProbe = false;
  bool   SavePCAP   = true;
  bool   EnableLED  = true;
  bool   EPDeauth   = false;
  bool   ChanHop    = false;
  String ClientSSID = "";
  String ClientPW   = "";
  String wu         = "";
  String wt         = "";
  String wdg_key    = "";
};

// Parse a settings JSON string ({"Settings":[{"name":..,"value":..}, ...]})
// into out. jsonCapacity is the DynamicJsonDocument budget (pass
// JSON_SETTING_SIZE from the caller). On a parse error out is left untouched
// and false is returned; on success recognised keys are copied and unknown
// keys ignored. Keys absent from the JSON keep their current value in out, so
// struct defaults survive a document that omits them.
bool buildSettingsCache(const char* json, SettingsCache& out, size_t jsonCapacity);

// Keep the cache in sync after a single saveSetting() without re-parsing JSON.
// Return true when key matched a cached field, false otherwise (the caller then
// keeps its existing JSON/create fallback untouched).
bool applyCachedBool(SettingsCache& cache, const char* key, bool value);
bool applyCachedString(SettingsCache& cache, const char* key, const String& value);
