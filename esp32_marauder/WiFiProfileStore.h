#pragma once

#ifndef WiFiProfileStore_h
#define WiFiProfileStore_h

#include "configs.h"

#include <Arduino.h>
#include <LinkedList.h>

enum class WiFiProfileStoreState : uint8_t {
  Uninitialized,
  Ready,
  NoSD,
  MetadataCorrupt,
  SecretsCorrupt,
  UnsupportedVersion,
  IOError,
  Full
};

struct WiFiProfileInfo {
  uint32_t id = 0;
  String ssid = "";
  uint8_t band = 0;
  uint8_t channel = 0;
};

class WiFiProfileStore {
  public:
    static const uint8_t MAX_PROFILES = 16;

    bool begin(bool sd_mounted);
    WiFiProfileStoreState refresh();

    size_t count();
    bool profileAt(size_t index, WiFiProfileInfo& profile);
    bool loadCredentials(uint32_t id, WiFiProfileInfo& profile, String& password);
    bool remember(
      const String& ssid,
      const String& password,
      int32_t channel = 0);
    bool forget(uint32_t id);
    bool reset();

    WiFiProfileStoreState state() const;
    const String& lastError() const;

  private:
    struct WiFiProfileSecret {
      uint32_t id = 0;
      String ssid = "";
      uint8_t band = 0;
      uint8_t channel = 0;
      String password = "";
    };

    bool initialized = false;
    bool sd_mounted = false;
    bool metadata_recovery_io_error = false;
    bool secrets_recovery_io_error = false;
    bool metadata_version_unsupported = false;
    bool secrets_version_unsupported = false;
    uint8_t metadata_loaded_version = 0;
    uint8_t secrets_loaded_version = 0;
    WiFiProfileStoreState store_state = WiFiProfileStoreState::Uninitialized;
    String last_error = "";

    LinkedList<WiFiProfileInfo> metadata_profiles;
    LinkedList<WiFiProfileSecret> secret_profiles;
    LinkedList<WiFiProfileInfo> visible_profiles;

    bool loadMetadata(LinkedList<WiFiProfileInfo>& profiles, bool& any_generation);
    bool loadSecrets(LinkedList<WiFiProfileSecret>& profiles, bool& any_generation);
    bool readMetadataFile(
      const char* path,
      LinkedList<WiFiProfileInfo>& profiles,
      int* version = nullptr) const;
    bool readSecretsFile(
      const char* path,
      LinkedList<WiFiProfileSecret>& profiles,
      int* version = nullptr) const;
    bool writeMetadata();
    bool writeSecrets();
    bool migrateLegacy(bool secrets_exist);
    bool rebuildMetadataFromSecrets();
    void rebuildVisibleProfiles();

    int metadataIndexById(uint32_t id);
    int metadataIndexByIdentity(const String& ssid, uint8_t band);
    int secretIndexById(uint32_t id);
    int secretIndexByIdentity(const String& ssid, uint8_t band);
    uint32_t generateId();

    static bool validSSID(const String& ssid);
    static bool validPassword(const String& password);
    static bool validChannel(uint8_t channel);
    static bool validBand(uint8_t band);
    static bool validBandChannel(uint8_t band, uint8_t channel);
    static uint8_t bandForChannel(uint8_t channel);
    static bool parseBSSID(const String& bssid, uint8_t output[6]);
};

extern WiFiProfileStore wifi_profile_store;

#endif
