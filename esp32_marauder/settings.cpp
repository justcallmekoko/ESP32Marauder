#include "settings.h"

namespace {
bool isSensitiveSetting(const char* name) {
  return name != nullptr &&
         (strcmp(name, "ClientPW") == 0 ||
          strcmp(name, "wt") == 0 ||
          strcmp(name, WDG_KEY_NAME) == 0);
}

constexpr const char* SETTINGS_PATH = "/settings.json";
constexpr const char* SETTINGS_TEMP_PATH = "/settings.wifi.tmp";
constexpr const char* SETTINGS_BACKUP_PATH = "/settings.wifi.bak";

bool validSettingsFile(const char* path) {
  File file = SPIFFS.open(path, FILE_READ);
  if (!file || file.isDirectory()) {
    if (file)
      file.close();
    return false;
  }

  DynamicJsonDocument json(JSON_SETTING_SIZE);
  const DeserializationError error = deserializeJson(json, file);
  file.close();
  return !error && json["Settings"].is<JsonArray>();
}

bool recoverSettingsFile() {
  if (validSettingsFile(SETTINGS_PATH)) {
    if (SPIFFS.exists(SETTINGS_TEMP_PATH))
      SPIFFS.remove(SETTINGS_TEMP_PATH);
    if (SPIFFS.exists(SETTINGS_BACKUP_PATH))
      SPIFFS.remove(SETTINGS_BACKUP_PATH);
    return true;
  }

  const char* candidates[] = {SETTINGS_BACKUP_PATH, SETTINGS_TEMP_PATH};
  for (const char* candidate : candidates) {
    if (!SPIFFS.exists(candidate) || !validSettingsFile(candidate))
      continue;
    if (SPIFFS.exists(SETTINGS_PATH) && !SPIFFS.remove(SETTINGS_PATH))
      return false;
    if (!SPIFFS.rename(candidate, SETTINGS_PATH))
      return false;
    if (SPIFFS.exists(SETTINGS_TEMP_PATH))
      SPIFFS.remove(SETTINGS_TEMP_PATH);
    if (SPIFFS.exists(SETTINGS_BACKUP_PATH))
      SPIFFS.remove(SETTINGS_BACKUP_PATH);
    return true;
  }
  return false;
}
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

  const bool settings_generation_exists =
    SPIFFS.exists(SETTINGS_PATH) ||
    SPIFFS.exists(SETTINGS_TEMP_PATH) ||
    SPIFFS.exists(SETTINGS_BACKUP_PATH);
  if (settings_generation_exists && !recoverSettingsFile())
    return false;

  File settingsFile;

  // SPIFFS.remove("/settings.json"); // NEED TO REMOVE THIS LINE

  if (SPIFFS.exists(SETTINGS_PATH)) {
    settingsFile = SPIFFS.open(SETTINGS_PATH, FILE_READ);

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

bool Settings::saveWiFiCredentials(const String& ssid, const String& password) {
  DynamicJsonDocument json(JSON_SETTING_SIZE);
  if (deserializeJson(json, this->json_settings_string))
    return false;

  bool found_ssid = false;
  bool found_password = false;
  for (int i = 0; i < (int)json["Settings"].size(); ++i) {
    const char* setting_name = json["Settings"][i]["name"] | "";
    if (strcmp(setting_name, "ClientSSID") == 0) {
      json["Settings"][i]["value"] = ssid;
      found_ssid = true;
    }
    else if (strcmp(setting_name, "ClientPW") == 0) {
      json["Settings"][i]["value"] = password;
      found_password = true;
    }
  }

  if (!found_ssid || !found_password)
    return false;

  String settings_string;
  serializeJson(json, settings_string);
  if (!settings_string.length())
    return false;

  if (SPIFFS.exists(SETTINGS_TEMP_PATH) && !SPIFFS.remove(SETTINGS_TEMP_PATH))
    return false;

  File settings_file = SPIFFS.open(SETTINGS_TEMP_PATH, FILE_WRITE);
  if (!settings_file)
    return false;
  const size_t written = settings_file.write(
    reinterpret_cast<const uint8_t*>(settings_string.c_str()),
    settings_string.length());
  settings_file.flush();
  settings_file.close();
  if (written != settings_string.length() || !validSettingsFile(SETTINGS_TEMP_PATH)) {
    SPIFFS.remove(SETTINGS_TEMP_PATH);
    return false;
  }

  if (SPIFFS.exists(SETTINGS_BACKUP_PATH) && !SPIFFS.remove(SETTINGS_BACKUP_PATH))
    return false;
  if (SPIFFS.exists(SETTINGS_PATH) && !SPIFFS.rename(SETTINGS_PATH, SETTINGS_BACKUP_PATH))
    return false;
  if (!SPIFFS.rename(SETTINGS_TEMP_PATH, SETTINGS_PATH)) {
    if (SPIFFS.exists(SETTINGS_BACKUP_PATH))
      SPIFFS.rename(SETTINGS_BACKUP_PATH, SETTINGS_PATH);
    return false;
  }
  if (!validSettingsFile(SETTINGS_PATH)) {
    SPIFFS.remove(SETTINGS_PATH);
    if (SPIFFS.exists(SETTINGS_BACKUP_PATH))
      SPIFFS.rename(SETTINGS_BACKUP_PATH, SETTINGS_PATH);
    return false;
  }
  if (SPIFFS.exists(SETTINGS_BACKUP_PATH))
    SPIFFS.remove(SETTINGS_BACKUP_PATH);

  this->json_settings_string = settings_string;
  _cache.ClientSSID = ssid;
  _cache.ClientPW = password;
  this->printJsonSettings(settings_string);
  return true;
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
    const char* name = json["Settings"][i]["name"] | "";
    Serial.println("Name: " + String(name));
    Serial.println("Type: " + json["Settings"][i]["type"].as<String>());
    if (isSensitiveSetting(name))
      Serial.println(F("Value: <redacted>\n"));
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
