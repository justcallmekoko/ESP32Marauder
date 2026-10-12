#include "SettingsCache.h"

#include <string.h>

// Must match WDG_KEY_NAME in settings.h. Kept as a literal here so this unit
// stays free of settings.h / configs.h (SPIFFS, Display, board config) and can
// compile in the native test environment.
static const char kWdgKeyName[] = "wdg_key";

bool buildSettingsCache(const char* json, SettingsCache& out, size_t jsonCapacity) {
  DynamicJsonDocument doc(jsonCapacity);

  if (deserializeJson(doc, json))
    return false;

  for (JsonObject entry : doc["Settings"].as<JsonArray>()) {
    const char* name = entry["name"] | "";

    if (strcmp(name, "ForcePMKID") == 0)
      out.ForcePMKID = entry["value"].as<bool>();
    else if (strcmp(name, "ForceProbe") == 0)
      out.ForceProbe = entry["value"].as<bool>();
    else if (strcmp(name, "SavePCAP") == 0)
      out.SavePCAP = entry["value"].as<bool>();
    else if (strcmp(name, "EnableLED") == 0)
      out.EnableLED = entry["value"].as<bool>();
    else if (strcmp(name, "EPDeauth") == 0)
      out.EPDeauth = entry["value"].as<bool>();
    else if (strcmp(name, "ChanHop") == 0)
      out.ChanHop = entry["value"].as<bool>();
    else if (strcmp(name, "ClientSSID") == 0)
      out.ClientSSID = String(entry["value"].as<const char*>());
    else if (strcmp(name, "ClientPW") == 0)
      out.ClientPW = String(entry["value"].as<const char*>());
    else if (strcmp(name, "wu") == 0)
      out.wu = String(entry["value"].as<const char*>());
    else if (strcmp(name, "wt") == 0)
      out.wt = String(entry["value"].as<const char*>());
    else if (strcmp(name, kWdgKeyName) == 0)
      out.wdg_key = String(entry["value"].as<const char*>());
  }

  return true;
}

bool applyCachedBool(SettingsCache& cache, const char* key, bool value) {
  if (strcmp(key, "ForcePMKID") == 0)      cache.ForcePMKID = value;
  else if (strcmp(key, "ForceProbe") == 0) cache.ForceProbe = value;
  else if (strcmp(key, "SavePCAP") == 0)   cache.SavePCAP = value;
  else if (strcmp(key, "EnableLED") == 0)  cache.EnableLED = value;
  else if (strcmp(key, "EPDeauth") == 0)   cache.EPDeauth = value;
  else if (strcmp(key, "ChanHop") == 0)    cache.ChanHop = value;
  else return false;
  return true;
}

bool applyCachedString(SettingsCache& cache, const char* key, const String& value) {
  if (strcmp(key, "ClientSSID") == 0)      cache.ClientSSID = value;
  else if (strcmp(key, "ClientPW") == 0)   cache.ClientPW = value;
  else if (strcmp(key, "wu") == 0)         cache.wu = value;
  else if (strcmp(key, "wt") == 0)         cache.wt = value;
  else if (strcmp(key, kWdgKeyName) == 0)  cache.wdg_key = value;
  else return false;
  return true;
}
