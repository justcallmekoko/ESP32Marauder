#include "settings.h"

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
    String name = json["Settings"][i]["name"].as<String>();

    if (name == "ForcePMKID")
      _cache.ForcePMKID = json["Settings"][i]["value"].as<bool>();
    else if (name == "ForceProbe")
      _cache.ForceProbe = json["Settings"][i]["value"].as<bool>();
    else if (name == "SavePCAP")
      _cache.SavePCAP = json["Settings"][i]["value"].as<bool>();
    else if (name == "EnableLED")
      _cache.EnableLED = json["Settings"][i]["value"].as<bool>();
    else if (name == "EPDeauth")
      _cache.EPDeauth = json["Settings"][i]["value"].as<bool>();
    else if (name == "ChanHop")
      _cache.ChanHop = json["Settings"][i]["value"].as<bool>();
    else if (name == "ClientSSID")
      _cache.ClientSSID = json["Settings"][i]["value"].as<String>();
    else if (name == "ClientPW")
      _cache.ClientPW = json["Settings"][i]["value"].as<String>();
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

  serializeJson(jsonBuffer, json_string);

  this->json_settings_string = json_string;

  // Populate the flat cache from the freshly loaded JSON.
  this->_buildCache();

  return true;
}

// ---------------------------------------------------------------------------
// loadSetting<T> — O(1), zero heap, reads straight from the cache struct.
// ---------------------------------------------------------------------------

template <typename T> T Settings::loadSetting(String key) {}

// Get type int settings
template <> int Settings::loadSetting<int>(String key) {
  // No int settings are defined currently; fall back to parsing if ever added.
  DynamicJsonDocument json(JSON_SETTING_SIZE);
  deserializeJson(json, this->json_settings_string);
  for (int i = 0; i < (int)json["Settings"].size(); i++) {
    if (json["Settings"][i]["name"].as<String>() == key)
      return json["Settings"][i]["value"];
  }
  return 0;
}

// Get type String settings — read from cache, no JSON parse
template <> String Settings::loadSetting<String>(String key) {
  if (key == "ClientSSID")
    return _cache.ClientSSID;
  if (key == "ClientPW")
    return _cache.ClientPW;

  // Unknown String key: fall back to JSON so the setting can be auto-created.
  DynamicJsonDocument json(JSON_SETTING_SIZE);
  deserializeJson(json, this->json_settings_string);
  for (int i = 0; i < (int)json["Settings"].size(); i++) {
    if (json["Settings"][i]["name"].as<String>() == key)
      return json["Settings"][i]["value"].as<String>();
  }

  //Serial.println("Did not find setting named " + (String)key + ". Creating...");
  if (this->createDefaultSettings(SPIFFS, true, json["Settings"].size(),
                                  "String", key))
    return "";

  return "";
}

// Get type bool settings — read from cache, no JSON parse
template <> bool Settings::loadSetting<bool>(String key) {
  if (key == "ForcePMKID")
    return _cache.ForcePMKID;
  if (key == "ForceProbe")
    return _cache.ForceProbe;
  if (key == "SavePCAP")
    return _cache.SavePCAP;
  if (key == "EnableLED")
    return _cache.EnableLED;
  if (key == "EPDeauth")
    return _cache.EPDeauth;
  if (key == "ChanHop")
    return _cache.ChanHop;

  // Unknown bool key: fall back to JSON so the setting can be auto-created.
  DynamicJsonDocument json(JSON_SETTING_SIZE);
  deserializeJson(json, this->json_settings_string);
  for (int i = 0; i < (int)json["Settings"].size(); i++) {
    if (json["Settings"][i]["name"].as<String>() == key)
      return json["Settings"][i]["value"].as<bool>();
  }

  //Serial.println("Did not find setting named " + (String)key + ". Creating...");
  if (this->createDefaultSettings(SPIFFS, true, json["Settings"].size(), "bool",
                                  key))
    return true;

  return false;
}

// Get type uint8_t settings — read from cache, no JSON parse
template <> uint8_t Settings::loadSetting<uint8_t>(String key) {
  // uint8_t settings reuse bool cache fields where applicable.
  if (key == "ForcePMKID")
    return (uint8_t)_cache.ForcePMKID;
  if (key == "ForceProbe")
    return (uint8_t)_cache.ForceProbe;
  if (key == "SavePCAP")
    return (uint8_t)_cache.SavePCAP;
  if (key == "EnableLED")
    return (uint8_t)_cache.EnableLED;
  if (key == "EPDeauth")
    return (uint8_t)_cache.EPDeauth;
  if (key == "ChanHop")
    return (uint8_t)_cache.ChanHop;

  DynamicJsonDocument json(JSON_SETTING_SIZE);
  deserializeJson(json, this->json_settings_string);
  for (int i = 0; i < (int)json["Settings"].size(); i++) {
    if (json["Settings"][i]["name"].as<String>() == key)
      return json["Settings"][i]["value"].as<uint8_t>();
  }
  return 0;
}

// ---------------------------------------------------------------------------
// saveSetting — writes SPIFFS, updates json_settings_string, rebuilds cache.
// ---------------------------------------------------------------------------

template <typename T> T Settings::saveSetting(String key, bool value) {}

template <> bool Settings::saveSetting<bool>(String key, bool value) {
  DynamicJsonDocument json(JSON_SETTING_SIZE);

  deserializeJson(json, this->json_settings_string);

  String settings_string;

  for (int i = 0; i < (int)json["Settings"].size(); i++) {
    if (json["Settings"][i]["name"].as<String>() == key) {
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
      if (key == "ForcePMKID")
        _cache.ForcePMKID = value;
      else if (key == "ForceProbe")
        _cache.ForceProbe = value;
      else if (key == "SavePCAP")
        _cache.SavePCAP = value;
      else if (key == "EnableLED")
        _cache.EnableLED = value;
      else if (key == "EPDeauth")
        _cache.EPDeauth = value;
      else if (key == "ChanHop")
        _cache.ChanHop = value;

      this->printJsonSettings(settings_string);

      return true;
    }
  }
  return false;
}

template <typename T> T Settings::saveSetting(String key, String value) {}

template <> bool Settings::saveSetting<bool>(String key, String value) {
  DynamicJsonDocument json(JSON_SETTING_SIZE);

  deserializeJson(json, this->json_settings_string);

  String settings_string;

  for (int i = 0; i < (int)json["Settings"].size(); i++) {
    if (json["Settings"][i]["name"].as<String>() == key) {
      json["Settings"][i]["value"] = value;

      //Serial.println("Saving setting...");

      File settingsFile = SPIFFS.open("/settings.json", FILE_WRITE);

      if (!settingsFile)
        return false;

      serializeJson(json, settingsFile);
      serializeJson(json, settings_string);

      settingsFile.close();

      this->json_settings_string = settings_string;

      // Keep the cache in sync for String fields.
      if (key == "ClientSSID")
        _cache.ClientSSID = value;
      else if (key == "ClientPW")
        _cache.ClientPW = value;

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
bool Settings::toggleSetting(String key) {
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
    Serial.println("Name: " + json["Settings"][i]["name"].as<String>());
    Serial.println("Type: " + json["Settings"][i]["type"].as<String>());
    Serial.println("Value: " + json["Settings"][i]["value"].as<String>() + "\n");
  }
}

// ---------------------------------------------------------------------------
// createDefaultSettings — sets json_settings_string then rebuilds the cache.
// ---------------------------------------------------------------------------
bool Settings::createDefaultSettings(fs::FS &fs, bool spec, uint8_t index, String typeStr, String name) {  
  File settingsFile = fs.open("/settings.json", FILE_WRITE);

  if (!settingsFile) {
    //Serial.println(F("Failed to create settings file"));
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

    //jsonBuffer.printTo(settingsFile);
    if (serializeJson(jsonBuffer, settingsFile) == 0) {
    }
    if (serializeJson(jsonBuffer, settings_string) == 0) {
    }
  }

  else {
    DynamicJsonDocument json(JSON_SETTING_SIZE); // ArduinoJson v6

    if (deserializeJson(json, this->json_settings_string)) {
      return false;
    }

    if (typeStr == "bool") {
      
      json["Settings"][index]["name"] = name;
      json["Settings"][index]["type"] = typeStr;
      json["Settings"][index]["value"] = true;
      json["Settings"][index]["range"]["min"] = false;
      json["Settings"][index]["range"]["max"] = true;

      if (serializeJson(json, settings_string) == 0) {
      }

      if (serializeJson(json, settingsFile) == 0) {
      }
    }

    else if (typeStr == "String") {
      json["Settings"][index]["name"] = name;
      json["Settings"][index]["type"] = typeStr;
      json["Settings"][index]["value"] = "";
      json["Settings"][index]["range"]["min"] = "";
      json["Settings"][index]["range"]["max"] = "";

      if (serializeJson(json, settings_string) == 0) {
      }

      if (serializeJson(json, settingsFile) == 0) {
      }
    }
  }

  // Close the file
  settingsFile.close();

  this->json_settings_string = settings_string;

  // Rebuild cache from the newly written settings.
  this->_buildCache();

  this->printJsonSettings(settings_string);

  return true;
}
