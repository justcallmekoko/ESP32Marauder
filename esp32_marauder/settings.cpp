#include "settings.h"

static JsonObject findSavedWifiSetting(DynamicJsonDocument& json) {
  JsonArray settings = json["Settings"].as<JsonArray>();
  for (JsonObject setting : settings) {
    if (strcmp(setting["name"] | "", SAVED_WIFI_KEY_NAME) == 0)
      return setting;
  }
  return JsonObject();
}

static JsonObject ensureSavedWifiSetting(DynamicJsonDocument& json) {
  JsonObject setting = findSavedWifiSetting(json);
  if (!setting.isNull())
    return setting;

  JsonArray settings = json["Settings"].as<JsonArray>();
  setting = settings.createNestedObject();
  setting["name"] = SAVED_WIFI_KEY_NAME;
  setting["type"] = "wifi_list";
  setting.createNestedArray("value");
  JsonObject range = setting.createNestedObject("range");
  range["min"] = 0;
  range["max"] = MAX_SAVED_WIFI_PROFILES;
  return setting;
}

// ---------------------------------------------------------------------------
// _buildCache — called once after json_settings_string is loaded/updated.
// Parses the JSON exactly once and fills every field of _cache.
// All loadSetting<T>() reads hit the cache; no heap is allocated on read.
// ---------------------------------------------------------------------------
void Settings::_buildCache() {
  DynamicJsonDocument json(JSON_SETTING_SIZE);

  if (deserializeJson(json, this->json_settings_string)) {
    Serial.println(F("_buildCache: could not parse json"));
    return;
  }

  for (int i = 0; i < (int)json["Settings"].size(); i++) {
    const char* name = json["Settings"][i]["name"] | "";

    if (strcmp(name, "ForcePMKID") == 0)
      _cache.ForcePMKID = json["Settings"][i]["value"].as<bool>();
    else if (strcmp(name, "ForceProbe") == 0)
      _cache.ForceProbe = json["Settings"][i]["value"].as<bool>();
    else if (strcmp(name, "SavePCAP") == 0)
      _cache.SavePCAP = json["Settings"][i]["value"].as<bool>();
    else if (strcmp(name, "EnableLED") == 0)
      _cache.EnableLED = json["Settings"][i]["value"].as<bool>();
    else if (strcmp(name, "EPDeauth") == 0)
      _cache.EPDeauth = json["Settings"][i]["value"].as<bool>();
    else if (strcmp(name, "ChanHop") == 0)
      _cache.ChanHop = json["Settings"][i]["value"].as<bool>();
    else if (strcmp(name, "ClientSSID") == 0)
      _cache.ClientSSID = json["Settings"][i]["value"].as<String>();
    else if (strcmp(name, "ClientPW") == 0)
      _cache.ClientPW = json["Settings"][i]["value"].as<String>();
    else if (strcmp(name, "wu") == 0)
      _cache.wu = json["Settings"][i]["value"].as<String>();
    else if (strcmp(name, "wt") == 0)
      _cache.wt = json["Settings"][i]["value"].as<String>();
    else if (strcmp(name, WDG_KEY_NAME) == 0)
      _cache.wdg_key = json["Settings"][i]["value"].as<String>();
  }
}

// ---------------------------------------------------------------------------

String Settings::getSettingsString() { return this->json_settings_string; }

bool Settings::begin() {
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    return false;
  }

  File settingsFile;

  // SPIFFS.remove("/settings.json"); // NEED TO REMOVE THIS LINE

  if (SPIFFS.exists("/settings.json")) {
    settingsFile = SPIFFS.open("/settings.json", FILE_READ);

    if (!settingsFile) {
      settingsFile.close();
      if (this->createDefaultSettings(SPIFFS))
        return true;
      else
        return false;
    }
  } else {
    if (this->createDefaultSettings(SPIFFS))
      return true;
    else
      return false;
  }

  String json_string;
  DynamicJsonDocument jsonBuffer(JSON_SETTING_SIZE);
  DeserializationError error = deserializeJson(jsonBuffer, settingsFile);
  if (error)
    Serial.println(error.f_str());

  settingsFile.close();

  bool settings_changed = false;
  JsonObject saved_wifi_setting = findSavedWifiSetting(jsonBuffer);
  if (saved_wifi_setting.isNull()) {
    saved_wifi_setting = ensureSavedWifiSetting(jsonBuffer);
    settings_changed = true;
  }

  // One-time, backward-compatible migration of the previous single profile.
  JsonArray saved_wifi = saved_wifi_setting["value"].as<JsonArray>();
  if (saved_wifi.isNull()) {
    saved_wifi = saved_wifi_setting.createNestedArray("value");
    settings_changed = true;
  }
  if (saved_wifi.size() == 0) {
    const char* legacy_ssid = "";
    const char* legacy_password = "";
    for (JsonObject setting : jsonBuffer["Settings"].as<JsonArray>()) {
      const char* setting_name = setting["name"] | "";
      if (strcmp(setting_name, "ClientSSID") == 0)
        legacy_ssid = setting["value"] | "";
      else if (strcmp(setting_name, "ClientPW") == 0)
        legacy_password = setting["value"] | "";
    }
    if (legacy_ssid[0] != '\0') {
      JsonObject profile = saved_wifi.createNestedObject();
      profile["ssid"] = legacy_ssid;
      profile["password"] = legacy_password;
      settings_changed = true;
    }
  }

  if (settings_changed) {
    File updated_settings = SPIFFS.open("/settings.json", FILE_WRITE);
    if (!updated_settings)
      return false;
    serializeJson(jsonBuffer, updated_settings);
    updated_settings.close();
  }

  serializeJson(jsonBuffer, json_string);

  this->json_settings_string = json_string;

  // Populate the flat cache from the freshly loaded JSON.
  this->_buildCache();

  return true;
}

// ---------------------------------------------------------------------------
// loadSetting<T> — O(1), zero heap, reads straight from the cache struct.
// ---------------------------------------------------------------------------

template <typename T> T Settings::loadSetting(const char* key) {}

// Get type int settings
template <> int Settings::loadSetting<int>(const char* key) {
  // No int settings are defined currently; fall back to parsing if ever added.
  DynamicJsonDocument json(JSON_SETTING_SIZE);
  deserializeJson(json, this->json_settings_string);

  for (int i = 0; i < (int)json["Settings"].size(); i++) {
    const char* setting_name = json["Settings"][i]["name"] | "";

    if (strcmp(setting_name, key) == 0)
      return json["Settings"][i]["value"];
  }

  return 0;
}

// Get type String settings — read from cache, no JSON parse
template <> String Settings::loadSetting<String>(const char* key) {
  if (strcmp(key, "ClientSSID") == 0)
    return _cache.ClientSSID;
  if (strcmp(key, "ClientPW") == 0)
    return _cache.ClientPW;
    if (strcmp(key, "wu") == 0)
    return _cache.wu;
  if (strcmp(key, "wt") == 0)
    return _cache.wt;
  if (strcmp(key, WDG_KEY_NAME) == 0)
    return _cache.wdg_key;

  // Unknown String key: fall back to JSON so the setting can be auto-created.
  DynamicJsonDocument json(JSON_SETTING_SIZE);
  deserializeJson(json, this->json_settings_string);

  for (int i = 0; i < (int)json["Settings"].size(); i++) {
    const char* setting_name = json["Settings"][i]["name"] | "";

    if (strcmp(setting_name, key) == 0)
      return json["Settings"][i]["value"].as<String>();
  }

  // Serial.print("Did not find setting named ");
  // Serial.print(key);
  // Serial.println(". Creating...");

  if (this->createDefaultSettings(
          SPIFFS,
          true,
          json["Settings"].size(),
          "String",
          key))
    return "";

  return "";
}

// Get type bool settings — read from cache, no JSON parse
template <> bool Settings::loadSetting<bool>(const char* key) {
  if (strcmp(key, "ForcePMKID") == 0)
    return _cache.ForcePMKID;
  if (strcmp(key, "ForceProbe") == 0)
    return _cache.ForceProbe;
  if (strcmp(key, "SavePCAP") == 0)
    return _cache.SavePCAP;
  if (strcmp(key, "EnableLED") == 0)
    return _cache.EnableLED;
  if (strcmp(key, "EPDeauth") == 0)
    return _cache.EPDeauth;
  if (strcmp(key, "ChanHop") == 0)
    return _cache.ChanHop;

  // Unknown bool key: fall back to JSON so the setting can be auto-created.
  DynamicJsonDocument json(JSON_SETTING_SIZE);
  deserializeJson(json, this->json_settings_string);

  for (int i = 0; i < (int)json["Settings"].size(); i++) {
    const char* setting_name = json["Settings"][i]["name"] | "";

    if (strcmp(setting_name, key) == 0) {
      return json["Settings"][i]["value"].as<bool>();
    }
  }

  // Serial.print("Did not find setting named ");
  // Serial.print(key);
  // Serial.println(". Creating...");

  if (this->createDefaultSettings(SPIFFS, true, json["Settings"].size(), "bool", key))
    return true;

  return false;
}

// Get type uint8_t settings — read from cache, no JSON parse
template <> uint8_t Settings::loadSetting<uint8_t>(const char* key) {
  // uint8_t settings reuse bool cache fields where applicable.
  if (strcmp(key, "ForcePMKID") == 0)
    return (uint8_t)_cache.ForcePMKID;

  if (strcmp(key, "ForceProbe") == 0)
    return (uint8_t)_cache.ForceProbe;

  if (strcmp(key, "SavePCAP") == 0)
    return (uint8_t)_cache.SavePCAP;

  if (strcmp(key, "EnableLED") == 0)
    return (uint8_t)_cache.EnableLED;

  if (strcmp(key, "EPDeauth") == 0)
    return (uint8_t)_cache.EPDeauth;

  if (strcmp(key, "ChanHop") == 0)
    return (uint8_t)_cache.ChanHop;

  DynamicJsonDocument json(JSON_SETTING_SIZE);
  deserializeJson(json, this->json_settings_string);

  for (int i = 0; i < (int)json["Settings"].size(); i++) {
    const char* setting_name = json["Settings"][i]["name"] | "";

    if (strcmp(setting_name, key) == 0)
      return json["Settings"][i]["value"].as<uint8_t>();
  }

  return 0;
}

// ---------------------------------------------------------------------------
// saveSetting — writes SPIFFS, updates json_settings_string, rebuilds cache.
// ---------------------------------------------------------------------------

template <typename T>
T Settings::saveSetting(const char* key, bool value) {}

template <> bool Settings::saveSetting<bool>(const char* key, bool value) {
  DynamicJsonDocument json(JSON_SETTING_SIZE);

  if (deserializeJson(json, this->json_settings_string)) {
    return false;
  }

  String settings_string;

  for (int i = 0; i < (int)json["Settings"].size(); i++) {
    const char* setting_name = json["Settings"][i]["name"] | "";

    if (strcmp(setting_name, key) == 0) {
      json["Settings"][i]["value"] = value;

      File settingsFile = SPIFFS.open("/settings.json", FILE_WRITE);

      if (!settingsFile) {
        return false;
      }

      serializeJson(json, settingsFile);
      serializeJson(json, settings_string);

      settingsFile.close();

      this->json_settings_string = settings_string;

      // Keep the cache in sync — no re-parse needed, just update the field.
      if (strcmp(key, "ForcePMKID") == 0)
        _cache.ForcePMKID = value;
      else if (strcmp(key, "ForceProbe") == 0)
        _cache.ForceProbe = value;
      else if (strcmp(key, "SavePCAP") == 0)
        _cache.SavePCAP = value;
      else if (strcmp(key, "EnableLED") == 0)
        _cache.EnableLED = value;
      else if (strcmp(key, "EPDeauth") == 0)
        _cache.EPDeauth = value;
      else if (strcmp(key, "ChanHop") == 0)
        _cache.ChanHop = value;

      this->printJsonSettings(settings_string);

      return true;
    }
  }

  return false;
}

template <typename T>
T Settings::saveSetting(const char* key, String value) {}

template <> bool Settings::saveSetting<bool>(const char* key, String value) {
  DynamicJsonDocument json(JSON_SETTING_SIZE);

  if (deserializeJson(json, this->json_settings_string)) {
    return false;
  }

  String settings_string;

  for (int i = 0; i < (int)json["Settings"].size(); i++) {
    const char* setting_name = json["Settings"][i]["name"] | "";

    if (strcmp(setting_name, key) == 0) {
      json["Settings"][i]["value"] = value;

      File settingsFile = SPIFFS.open("/settings.json", FILE_WRITE);

      if (!settingsFile) {
        return false;
      }

      serializeJson(json, settingsFile);
      serializeJson(json, settings_string);

      settingsFile.close();

      this->json_settings_string = settings_string;

      // Keep the cache in sync for String fields.
      if (strcmp(key, "ClientSSID") == 0)
        _cache.ClientSSID = value;
      else if (strcmp(key, "ClientPW") == 0)
        _cache.ClientPW = value;
      else if (strcmp(key, "wu") == 0)
        _cache.wu = value;
      else if (strcmp(key, "wt") == 0)
        _cache.wt = value;
      else if (strcmp(key, WDG_KEY_NAME) == 0)
        _cache.wdg_key = value;

      this->printJsonSettings(settings_string);

      return true;
    }
  }

  return false;
}

// ---------------------------------------------------------------------------
// toggleSetting — reads current bool value from cache (fast), then delegates
// to saveSetting which will update the cache again.
// ---------------------------------------------------------------------------
bool Settings::toggleSetting(const char* key) {
  // Use the cached value to decide direction — avoids an extra JSON parse.
  bool current = this->loadSetting<bool>(key);
  if (current) {
    saveSetting<bool>(key, false);
    //Serial.println("Setting value to false");
    return false;
  } else {
    saveSetting<bool>(key, true);
    //Serial.println("Setting value to true");
    return true;
  }
}

// ---------------------------------------------------------------------------
// Rarely-called utility functions — keep JSON parsing, they are not hot paths.
// ---------------------------------------------------------------------------

String Settings::setting_index_to_name(int i) {
  DynamicJsonDocument json(JSON_SETTING_SIZE);

  deserializeJson(json, this->json_settings_string);

  return json["Settings"][i]["name"];
}

int Settings::getNumberSettings() {
  DynamicJsonDocument json(JSON_SETTING_SIZE);

  deserializeJson(json, this->json_settings_string);

  return json["Settings"].size();
}

const char* Settings::getSettingType(const char* key) {
  static char type_buf[16];  // persistent buffer (adjust size if needed)

  DynamicJsonDocument json(JSON_SETTING_SIZE);
  deserializeJson(json, this->json_settings_string);

  for (int i = 0; i < (int)json["Settings"].size(); i++) {
    const char* name = json["Settings"][i]["name"];
    
    if (name && strcmp(name, key) == 0) {
      const char* type = json["Settings"][i]["type"];
      
      if (type) {
        strncpy(type_buf, type, sizeof(type_buf));
        type_buf[sizeof(type_buf) - 1] = '\0';
        return type_buf;
      }
    }
  }

  return "";
}

void Settings::printJsonSettings(String json_string) {
  DynamicJsonDocument json(JSON_SETTING_SIZE);

  deserializeJson(json, json_string);

  Serial.println("Settings\n----------------------------------------------");
  for (int i = 0; i < (int)json["Settings"].size(); i++) {
    String setting_name = json["Settings"][i]["name"].as<String>();
    Serial.println("Name: " + setting_name);
    Serial.println("Type: " + json["Settings"][i]["type"].as<String>());
    if (setting_name == "ClientPW" || setting_name == SAVED_WIFI_KEY_NAME)
      Serial.println(F("Value: [redacted]\n"));
    else
      Serial.println("Value: " + json["Settings"][i]["value"].as<String>() + "\n");
  }
}

// ---------------------------------------------------------------------------
// createDefaultSettings — sets json_settings_string then rebuilds the cache.
// ---------------------------------------------------------------------------

bool Settings::createDefaultSettings(fs::FS &fs, bool spec, uint8_t index, const char* typeStr, const char* name) {
  File settingsFile = fs.open("/settings.json", FILE_WRITE);

  if (!settingsFile) {
    // Serial.println(F("Failed to create settings file"));
    return false;
  }

  String settings_string;

  if (!spec) {
    DynamicJsonDocument jsonBuffer(JSON_SETTING_SIZE);

    jsonBuffer["Settings"][0]["name"] = "ForcePMKID";
    jsonBuffer["Settings"][0]["type"] = "bool";
    jsonBuffer["Settings"][0]["value"] = false;
    jsonBuffer["Settings"][0]["range"]["min"] = false;
    jsonBuffer["Settings"][0]["range"]["max"] = true;

    jsonBuffer["Settings"][1]["name"] = "ForceProbe";
    jsonBuffer["Settings"][1]["type"] = "bool";
    jsonBuffer["Settings"][1]["value"] = false;
    jsonBuffer["Settings"][1]["range"]["min"] = false;
    jsonBuffer["Settings"][1]["range"]["max"] = true;

    jsonBuffer["Settings"][2]["name"] = "SavePCAP";
    jsonBuffer["Settings"][2]["type"] = "bool";
    jsonBuffer["Settings"][2]["value"] = true;
    jsonBuffer["Settings"][2]["range"]["min"] = false;
    jsonBuffer["Settings"][2]["range"]["max"] = true;

    jsonBuffer["Settings"][3]["name"] = "EnableLED";
    jsonBuffer["Settings"][3]["type"] = "bool";
    jsonBuffer["Settings"][3]["value"] = true;
    jsonBuffer["Settings"][3]["range"]["min"] = false;
    jsonBuffer["Settings"][3]["range"]["max"] = true;

    jsonBuffer["Settings"][4]["name"] = "EPDeauth";
    jsonBuffer["Settings"][4]["type"] = "bool";
    jsonBuffer["Settings"][4]["value"] = false;
    jsonBuffer["Settings"][4]["range"]["min"] = false;
    jsonBuffer["Settings"][4]["range"]["max"] = true;

    jsonBuffer["Settings"][5]["name"] = "ChanHop";
    jsonBuffer["Settings"][5]["type"] = "bool";
    jsonBuffer["Settings"][5]["value"] = false;
    jsonBuffer["Settings"][5]["range"]["min"] = false;
    jsonBuffer["Settings"][5]["range"]["max"] = true;

    jsonBuffer["Settings"][6]["name"] = "ClientSSID";
    jsonBuffer["Settings"][6]["type"] = "String";
    jsonBuffer["Settings"][6]["value"] = "";
    jsonBuffer["Settings"][6]["range"]["min"] = "";
    jsonBuffer["Settings"][6]["range"]["max"] = "";

    jsonBuffer["Settings"][7]["name"] = "ClientPW";
    jsonBuffer["Settings"][7]["type"] = "String";
    jsonBuffer["Settings"][7]["value"] = "";
    jsonBuffer["Settings"][7]["range"]["min"] = "";
    jsonBuffer["Settings"][7]["range"]["max"] = "";

    jsonBuffer["Settings"][8]["name"] = "wu";
    jsonBuffer["Settings"][8]["type"] = "String";
    jsonBuffer["Settings"][8]["value"] = "";
    jsonBuffer["Settings"][8]["range"]["min"] = "";
    jsonBuffer["Settings"][8]["range"]["max"] = "";

    jsonBuffer["Settings"][9]["name"] = "wt";
    jsonBuffer["Settings"][9]["type"] = "String";
    jsonBuffer["Settings"][9]["value"] = "";
    jsonBuffer["Settings"][9]["range"]["min"] = "";
    jsonBuffer["Settings"][9]["range"]["max"] = "";

    jsonBuffer["Settings"][10]["name"] = WDG_KEY_NAME;
    jsonBuffer["Settings"][10]["type"] = "String";
    jsonBuffer["Settings"][10]["value"] = "";
    jsonBuffer["Settings"][10]["range"]["min"] = "";
    jsonBuffer["Settings"][10]["range"]["max"] = "";

    jsonBuffer["Settings"][11]["name"] = SAVED_WIFI_KEY_NAME;
    jsonBuffer["Settings"][11]["type"] = "wifi_list";
    jsonBuffer["Settings"][11].createNestedArray("value");
    jsonBuffer["Settings"][11]["range"]["min"] = 0;
    jsonBuffer["Settings"][11]["range"]["max"] = MAX_SAVED_WIFI_PROFILES;

    serializeJson(jsonBuffer, settingsFile);
    serializeJson(jsonBuffer, settings_string);
  } else {
    DynamicJsonDocument json(JSON_SETTING_SIZE);

    if (deserializeJson(json, this->json_settings_string)) {
      settingsFile.close();
      return false;
    }

    if (strcmp(typeStr, "bool") == 0) {
      json["Settings"][index]["name"] = name;
      json["Settings"][index]["type"] = typeStr;
      json["Settings"][index]["value"] = true;
      json["Settings"][index]["range"]["min"] = false;
      json["Settings"][index]["range"]["max"] = true;

      serializeJson(json, settings_string);
      serializeJson(json, settingsFile);
    } else if (strcmp(typeStr, "String") == 0) {
      json["Settings"][index]["name"] = name;
      json["Settings"][index]["type"] = typeStr;
      json["Settings"][index]["value"] = "";
      json["Settings"][index]["range"]["min"] = "";
      json["Settings"][index]["range"]["max"] = "";

      serializeJson(json, settings_string);
      serializeJson(json, settingsFile);
    }
  }

  settingsFile.close();

  this->json_settings_string = settings_string;

  // Rebuild cache from the newly written settings.
  this->_buildCache();

  this->printJsonSettings(settings_string);

  return true;
}

uint8_t Settings::getSavedWifiCount() {
  DynamicJsonDocument json(JSON_SETTING_SIZE);
  if (deserializeJson(json, this->json_settings_string))
    return 0;
  JsonObject setting = findSavedWifiSetting(json);
  if (setting.isNull())
    return 0;
  const size_t count = setting["value"].as<JsonArray>().size();
  return count > MAX_SAVED_WIFI_PROFILES ? MAX_SAVED_WIFI_PROFILES : count;
}

bool Settings::loadSavedWifiCredential(uint8_t index, String& ssid, String& password) {
  ssid = "";
  password = "";
  DynamicJsonDocument json(JSON_SETTING_SIZE);
  if (deserializeJson(json, this->json_settings_string))
    return false;
  JsonObject setting = findSavedWifiSetting(json);
  JsonArray profiles = setting["value"].as<JsonArray>();
  if (setting.isNull() || index >= profiles.size())
    return false;
  ssid = profiles[index]["ssid"].as<String>();
  password = profiles[index]["password"].as<String>();
  return ssid.length() > 0;
}

WifiCredentialSaveResult Settings::saveWifiCredential(const String& ssid, const String& password, int8_t replace_index) {
  if (ssid.length() == 0 || ssid.length() > 32 || password.length() > 63)
    return WIFI_CREDENTIAL_ERROR;

  DynamicJsonDocument json(JSON_SETTING_SIZE);
  if (deserializeJson(json, this->json_settings_string))
    return WIFI_CREDENTIAL_ERROR;
  JsonObject setting = ensureSavedWifiSetting(json);
  JsonArray profiles = setting["value"].as<JsonArray>();

  int8_t existing_index = -1;
  for (uint8_t i = 0; i < profiles.size(); i++) {
    if (ssid == profiles[i]["ssid"].as<String>()) {
      existing_index = i;
      break;
    }
  }

  WifiCredentialSaveResult result = WIFI_CREDENTIAL_SAVED;
  int8_t target_index = existing_index;
  if (existing_index >= 0) {
    result = WIFI_CREDENTIAL_UPDATED;
  }
  else if (profiles.size() < MAX_SAVED_WIFI_PROFILES) {
    profiles.createNestedObject();
    target_index = profiles.size() - 1;
  }
  else if (replace_index >= 0 && replace_index < (int8_t)profiles.size()) {
    target_index = replace_index;
  }
  else {
    return WIFI_CREDENTIAL_FULL;
  }

  // Successful/new credentials become the first profile tried. Rotate the
  // small bounded array without retaining a second credential collection.
  for (int8_t i = target_index; i > 0; i--) {
    profiles[i]["ssid"] = profiles[i - 1]["ssid"].as<String>();
    profiles[i]["password"] = profiles[i - 1]["password"].as<String>();
  }
  profiles[0]["ssid"] = ssid;
  profiles[0]["password"] = password;

  File settings_file = SPIFFS.open("/settings.json", FILE_WRITE);
  if (!settings_file)
    return WIFI_CREDENTIAL_ERROR;
  serializeJson(json, settings_file);
  settings_file.close();
  serializeJson(json, this->json_settings_string);
  return result;
}

bool Settings::removeSavedWifiCredential(uint8_t index) {
  DynamicJsonDocument json(JSON_SETTING_SIZE);
  if (deserializeJson(json, this->json_settings_string))
    return false;
  JsonObject setting = findSavedWifiSetting(json);
  JsonArray profiles = setting["value"].as<JsonArray>();
  if (setting.isNull() || index >= profiles.size())
    return false;
  profiles.remove(index);

  File settings_file = SPIFFS.open("/settings.json", FILE_WRITE);
  if (!settings_file)
    return false;
  serializeJson(json, settings_file);
  settings_file.close();
  serializeJson(json, this->json_settings_string);
  return true;
}

bool Settings::markSavedWifiSuccessful(uint8_t index) {
  // The first profile is already the most recently successful. Avoid an
  // unnecessary settings-file rewrite on every normal connection.
  if (index == 0)
    return getSavedWifiCount() > 0;
  String ssid;
  String password;
  if (!loadSavedWifiCredential(index, ssid, password))
    return false;
  return saveWifiCredential(ssid, password) != WIFI_CREDENTIAL_ERROR;
}
