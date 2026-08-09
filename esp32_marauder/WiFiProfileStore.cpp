#include "WiFiProfileStore.h"

#include "settings.h"

#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <esp_system.h>

extern Settings settings_obj;

#ifdef HAS_SD
  #include "SDInterface.h"
  #include <SD.h>

  extern SDInterface sd_obj;
#endif

namespace {
constexpr uint8_t LEGACY_STORE_VERSION = 1;
constexpr uint8_t BSSID_STORE_VERSION = 2;
constexpr uint8_t STORE_VERSION = 3;
constexpr size_t STORE_JSON_CAPACITY = 8192;
constexpr size_t STORE_FILE_LIMIT = 8192;

constexpr const char* SECRET_PATH = "/wifi-profile-secrets.json";
constexpr const char* SECRET_TEMP_PATH = "/wifi-profile-secrets.tmp";
constexpr const char* SECRET_BACKUP_PATH = "/wifi-profile-secrets.bak";

#ifdef HAS_SD
constexpr const char* LEGACY_METADATA_DIRECTORY = "/.marauder-system";
constexpr const char* LEGACY_METADATA_PATH = "/.marauder-system/wifi-profiles.json";
constexpr const char* LEGACY_METADATA_TEMP_PATH = "/.marauder-system/wifi-profiles.tmp";
constexpr const char* LEGACY_METADATA_BACKUP_PATH = "/.marauder-system/wifi-profiles.bak";
constexpr const char* METADATA_DIRECTORY = "/MRDSYS01";
constexpr const char* METADATA_PATH = "/MRDSYS01/wifi-profiles.json";
constexpr const char* METADATA_TEMP_PATH = "/MRDSYS01/wifi-profiles.tmp";
constexpr const char* METADATA_BACKUP_PATH = "/MRDSYS01/wifi-profiles.bak";

bool metadataDirectory(const char* path) {
  File directory = SD.open(path, FILE_READ);
  const bool valid = directory && directory.isDirectory();
  if (directory)
    directory.close();
  return valid;
}

bool legacyDirectoryContainsOnlyMetadata() {
  File directory = SD.open(LEGACY_METADATA_DIRECTORY, FILE_READ);
  if (!directory || !directory.isDirectory()) {
    if (directory)
      directory.close();
    return false;
  }

  bool valid = true;
  while (valid) {
    File entry = directory.openNextFile();
    if (!entry)
      break;

    String name = entry.name();
    const int separator = name.lastIndexOf('/');
    if (separator >= 0)
      name = name.substring(separator + 1);
    name.toLowerCase();
    valid = !entry.isDirectory() &&
            (name == "wifi-profiles.json" ||
             name == "wifi-profiles.tmp" ||
             name == "wifi-profiles.bak");
    entry.close();
  }
  directory.close();
  return valid;
}

bool removeLegacyMetadataDirectory() {
  if (!legacyDirectoryContainsOnlyMetadata())
    return false;

  const char* files[] = {
    LEGACY_METADATA_PATH,
    LEGACY_METADATA_TEMP_PATH,
    LEGACY_METADATA_BACKUP_PATH};
  for (const char* file : files) {
    if (SD.exists(file) && !SD.remove(file))
      return false;
  }
  return SD.rmdir(LEGACY_METADATA_DIRECTORY) &&
         !SD.exists(LEGACY_METADATA_DIRECTORY);
}

bool prepareMetadataDirectory(bool& cleanup_legacy_required) {
  cleanup_legacy_required = false;
  const bool legacy_exists = SD.exists(LEGACY_METADATA_DIRECTORY);
  const bool current_exists = SD.exists(METADATA_DIRECTORY);

  if (current_exists && !metadataDirectory(METADATA_DIRECTORY))
    return false;
  if (!legacy_exists)
    return true;
  if (!metadataDirectory(LEGACY_METADATA_DIRECTORY) ||
      !legacyDirectoryContainsOnlyMetadata()) {
    return false;
  }

  if (current_exists) {
    cleanup_legacy_required = true;
    return true;
  }

  if (!SD.rename(LEGACY_METADATA_DIRECTORY, METADATA_DIRECTORY))
    return false;
  return !SD.exists(LEGACY_METADATA_DIRECTORY) &&
         metadataDirectory(METADATA_DIRECTORY);
}
#endif

bool anyFileExists(fs::FS& fs, const char* primary, const char* backup, const char* temporary) {
  return fs.exists(primary) || fs.exists(backup) || fs.exists(temporary);
}

bool moveIntoPlace(
  fs::FS& fs,
  const char* primary,
  const char* backup,
  const char* temporary) {
  const bool had_primary = fs.exists(primary);

  if (fs.exists(backup) && !fs.remove(backup))
    return false;

  if (had_primary && !fs.rename(primary, backup))
    return false;

  if (!fs.rename(temporary, primary))
    return false;

  return true;
}

bool restoreBackup(fs::FS& fs, const char* primary, const char* backup) {
  if (fs.exists(primary) && !fs.remove(primary))
    return false;
  return fs.rename(backup, primary);
}

bool cleanupOldGenerations(fs::FS& fs, const char* backup, const char* temporary) {
  if (fs.exists(backup) && !fs.remove(backup))
    return false;
  if (fs.exists(temporary) && !fs.remove(temporary))
    return false;
  return true;
}

int8_t hexNibble(char value) {
  if (value >= '0' && value <= '9')
    return value - '0';
  if (value >= 'a' && value <= 'f')
    return value - 'a' + 10;
  if (value >= 'A' && value <= 'F')
    return value - 'A' + 10;
  return -1;
}
}

WiFiProfileStore wifi_profile_store;

bool WiFiProfileStore::validSSID(const String& ssid) {
  if (ssid.length() == 0 || ssid.length() > 32)
    return false;

  for (size_t i = 0; i < ssid.length(); ++i) {
    const uint8_t value = static_cast<uint8_t>(ssid.charAt(i));
    if (value < 0x20 || value == 0x7f)
      return false;
  }
  return true;
}

bool WiFiProfileStore::validPassword(const String& password) {
  return password.length() <= 64;
}

bool WiFiProfileStore::validChannel(uint8_t channel) {
  return channel == 0 || channel <= 177;
}

bool WiFiProfileStore::validBand(uint8_t band) {
  return band == 0 || band == 2 || band == 5;
}

uint8_t WiFiProfileStore::bandForChannel(uint8_t channel) {
  if (channel == 0)
    return 0;
  return channel <= 14 ? 2 : 5;
}

bool WiFiProfileStore::validBandChannel(uint8_t band, uint8_t channel) {
  if (!validBand(band) || !validChannel(channel))
    return false;
  if (band == 0)
    return channel == 0;
  if (channel == 0)
    return true;
  return bandForChannel(channel) == band;
}

bool WiFiProfileStore::parseBSSID(const String& bssid, uint8_t output[6]) {
  if (output == nullptr || bssid.length() != 17)
    return false;

  bool all_zero = true;
  bool all_ff = true;
  for (uint8_t i = 0; i < 6; ++i) {
    const size_t offset = static_cast<size_t>(i) * 3;
    if (i > 0 && bssid.charAt(offset - 1) != ':')
      return false;
    const int8_t high = hexNibble(bssid.charAt(offset));
    const int8_t low = hexNibble(bssid.charAt(offset + 1));
    if (high < 0 || low < 0)
      return false;
    output[i] = static_cast<uint8_t>((high << 4) | low);
    all_zero = all_zero && output[i] == 0x00;
    all_ff = all_ff && output[i] == 0xff;
  }

  return !all_zero && !all_ff && (output[0] & 0x01) == 0;
}

bool WiFiProfileStore::begin(bool sd_mounted) {
  this->initialized = true;
  this->sd_mounted = sd_mounted;
  return this->refresh() == WiFiProfileStoreState::Ready;
}

WiFiProfileStoreState WiFiProfileStore::refresh() {
  this->metadata_profiles.clear();
  this->secret_profiles.clear();
  this->visible_profiles.clear();
  this->last_error = "";

  if (!this->initialized) {
    this->store_state = WiFiProfileStoreState::Uninitialized;
    this->last_error = F("WiFi profile store is not initialized");
    return this->store_state;
  }

  #ifndef HAS_SD
    this->store_state = WiFiProfileStoreState::NoSD;
    this->last_error = F("SD card support is unavailable");
    return this->store_state;
  #else
    this->sd_mounted = sd_obj.supported;
    if (!this->sd_mounted || !sd_obj.supported) {
      this->store_state = WiFiProfileStoreState::NoSD;
      this->last_error = F("SD card is unavailable");
      return this->store_state;
    }

    bool metadata_exists = false;
    bool secrets_exist = false;
    const bool secrets_valid = this->loadSecrets(this->secret_profiles, secrets_exist);

    if (this->secrets_version_unsupported) {
      this->secret_profiles.clear();
      this->store_state = WiFiProfileStoreState::UnsupportedVersion;
      this->last_error = F("Saved WiFi profiles require newer firmware");
      return this->store_state;
    }

    if (secrets_exist && !secrets_valid) {
      this->metadata_profiles.clear();
      this->secret_profiles.clear();
      this->store_state = this->secrets_recovery_io_error
        ? WiFiProfileStoreState::IOError
        : WiFiProfileStoreState::SecretsCorrupt;
      this->last_error = this->secrets_recovery_io_error
        ? String(F("Could not recover saved WiFi credentials"))
        : String(F("Saved WiFi credentials are damaged"));
      return this->store_state;
    }

    bool cleanup_legacy_required = false;
    if (!prepareMetadataDirectory(cleanup_legacy_required)) {
      this->metadata_profiles.clear();
      this->secret_profiles.clear();
      this->store_state = WiFiProfileStoreState::IOError;
      this->last_error = F("Could not migrate WiFi profile metadata");
      return this->store_state;
    }

    bool metadata_valid = this->loadMetadata(this->metadata_profiles, metadata_exists);
    if (this->metadata_version_unsupported) {
      this->metadata_profiles.clear();
      this->secret_profiles.clear();
      this->store_state = WiFiProfileStoreState::UnsupportedVersion;
      this->last_error = F("Saved WiFi profiles require newer firmware");
      return this->store_state;
    }

    if (cleanup_legacy_required) {
      if (!secrets_exist) {
        this->metadata_profiles.clear();
        this->secret_profiles.clear();
        this->store_state = WiFiProfileStoreState::SecretsCorrupt;
        this->last_error = F("Saved WiFi credentials are missing");
        return this->store_state;
      }
      if (!removeLegacyMetadataDirectory()) {
        this->metadata_profiles.clear();
        this->secret_profiles.clear();
        this->store_state = WiFiProfileStoreState::IOError;
        this->last_error = F("Could not remove legacy WiFi metadata");
        return this->store_state;
      }

    }

    if (secrets_exist && secrets_valid &&
        this->secrets_loaded_version < STORE_VERSION) {
      // Version 1 did not know bands. A valid v2/v3 metadata entry can safely
      // enrich it because the stable id and SSID must both match.
      if (this->secrets_loaded_version == LEGACY_STORE_VERSION &&
          metadata_exists && metadata_valid &&
          this->metadata_loaded_version >= BSSID_STORE_VERSION) {
        for (int i = 0; i < this->secret_profiles.size(); ++i) {
          WiFiProfileSecret secret = this->secret_profiles.get(i);
          const int metadata_index = this->metadataIndexById(secret.id);
          if (metadata_index >= 0) {
            const WiFiProfileInfo metadata = this->metadata_profiles.get(metadata_index);
            if (metadata.ssid == secret.ssid) {
              secret.band = metadata.band;
              secret.channel = metadata.channel;
              this->secret_profiles.set(i, secret);
            }
          }
        }
      }

      if (!this->writeSecrets()) {
        this->metadata_profiles.clear();
        this->secret_profiles.clear();
        this->store_state = WiFiProfileStoreState::IOError;
        this->last_error = F("Could not upgrade saved WiFi credentials");
        return this->store_state;
      }
      this->secrets_loaded_version = STORE_VERSION;
    }

    if (cleanup_legacy_required && secrets_exist && secrets_valid) {
      if (!this->rebuildMetadataFromSecrets()) {
        this->metadata_profiles.clear();
        this->secret_profiles.clear();
        this->store_state = WiFiProfileStoreState::IOError;
        this->last_error = F("Could not reconcile migrated WiFi metadata");
        return this->store_state;
      }
      metadata_exists = true;
      metadata_valid = true;
      this->metadata_loaded_version = STORE_VERSION;
    }

    if (secrets_exist && secrets_valid && metadata_exists && metadata_valid &&
        this->metadata_loaded_version < STORE_VERSION) {
      if (!this->rebuildMetadataFromSecrets()) {
        this->metadata_profiles.clear();
        this->secret_profiles.clear();
        this->store_state = WiFiProfileStoreState::IOError;
        this->last_error = F("Could not upgrade WiFi profile metadata");
        return this->store_state;
      }
      this->metadata_loaded_version = STORE_VERSION;
    }

    if (metadata_exists && !metadata_valid) {
      if (this->metadata_recovery_io_error || !secrets_exist) {
        this->metadata_profiles.clear();
        this->secret_profiles.clear();
        this->store_state = this->metadata_recovery_io_error
          ? WiFiProfileStoreState::IOError
          : WiFiProfileStoreState::MetadataCorrupt;
        this->last_error = this->metadata_recovery_io_error
          ? String(F("Could not recover WiFi profile metadata"))
          : String(F("Saved WiFi profile metadata is damaged"));
        return this->store_state;
      }

      if (!this->rebuildMetadataFromSecrets()) {
        this->store_state = WiFiProfileStoreState::IOError;
        if (!this->last_error.length())
          this->last_error = F("Could not rebuild WiFi profile metadata");
        return this->store_state;
      }
    }

    if (!metadata_exists) {
      if (!this->migrateLegacy(secrets_exist)) {
        this->metadata_profiles.clear();
        this->secret_profiles.clear();
        this->store_state = WiFiProfileStoreState::IOError;
        if (!this->last_error.length())
          this->last_error = F("Could not initialize WiFi profile storage");
        return this->store_state;
      }
    }
    else if (!secrets_exist) {
      if (this->metadata_profiles.size() != 0 || !this->writeSecrets()) {
        this->metadata_profiles.clear();
        this->secret_profiles.clear();
        this->store_state = WiFiProfileStoreState::SecretsCorrupt;
        this->last_error = F("Saved WiFi credentials are missing");
        return this->store_state;
      }
      if (this->metadata_loaded_version < STORE_VERSION &&
          !this->rebuildMetadataFromSecrets()) {
        this->metadata_profiles.clear();
        this->secret_profiles.clear();
        this->store_state = WiFiProfileStoreState::IOError;
        this->last_error = F("Could not upgrade empty WiFi profile metadata");
        return this->store_state;
      }
    }

    this->rebuildVisibleProfiles();
    if (this->visible_profiles.size() != this->metadata_profiles.size() ||
        this->visible_profiles.size() != this->secret_profiles.size()) {
      if (!this->rebuildMetadataFromSecrets()) {
        this->metadata_profiles.clear();
        this->secret_profiles.clear();
        this->visible_profiles.clear();
        this->store_state = WiFiProfileStoreState::IOError;
        if (!this->last_error.length())
          this->last_error = F("Could not reconcile WiFi profile metadata");
        return this->store_state;
      }
      this->rebuildVisibleProfiles();
    }
    this->store_state = WiFiProfileStoreState::Ready;
    return this->store_state;
  #endif
}

size_t WiFiProfileStore::count() {
  return static_cast<size_t>(this->visible_profiles.size());
}

bool WiFiProfileStore::profileAt(size_t index, WiFiProfileInfo& profile) {
  if (index >= this->count())
    return false;
  profile = this->visible_profiles.get(index);
  return true;
}

WiFiProfileStoreState WiFiProfileStore::state() const {
  return this->store_state;
}

const String& WiFiProfileStore::lastError() const {
  return this->last_error;
}

int WiFiProfileStore::metadataIndexById(uint32_t id) {
  for (int i = 0; i < this->metadata_profiles.size(); ++i) {
    if (this->metadata_profiles.get(i).id == id)
      return i;
  }
  return -1;
}

int WiFiProfileStore::metadataIndexByIdentity(const String& ssid, uint8_t band) {
  for (int i = 0; i < this->metadata_profiles.size(); ++i) {
    const WiFiProfileInfo profile = this->metadata_profiles.get(i);
    if (profile.ssid == ssid && profile.band == band)
      return i;
  }
  return -1;
}

int WiFiProfileStore::secretIndexById(uint32_t id) {
  for (int i = 0; i < this->secret_profiles.size(); ++i) {
    if (this->secret_profiles.get(i).id == id)
      return i;
  }
  return -1;
}

int WiFiProfileStore::secretIndexByIdentity(const String& ssid, uint8_t band) {
  for (int i = 0; i < this->secret_profiles.size(); ++i) {
    const WiFiProfileSecret profile = this->secret_profiles.get(i);
    if (profile.ssid == ssid && profile.band == band)
      return i;
  }
  return -1;
}

uint32_t WiFiProfileStore::generateId() {
  for (uint8_t attempt = 0; attempt < 32; ++attempt) {
    const uint32_t candidate = esp_random();
    if (candidate != 0 && this->metadataIndexById(candidate) < 0 && this->secretIndexById(candidate) < 0)
      return candidate;
  }
  return 0;
}

void WiFiProfileStore::rebuildVisibleProfiles() {
  this->visible_profiles.clear();
  for (int i = 0; i < this->metadata_profiles.size(); ++i) {
    const WiFiProfileInfo metadata = this->metadata_profiles.get(i);
    const int secret_index = this->secretIndexById(metadata.id);
    if (secret_index < 0)
      continue;

    const WiFiProfileSecret secret = this->secret_profiles.get(secret_index);
    if (secret.ssid == metadata.ssid &&
        secret.band == metadata.band &&
        secret.channel == metadata.channel)
      this->visible_profiles.add(metadata);
  }
}

bool WiFiProfileStore::loadCredentials(
  uint32_t id,
  WiFiProfileInfo& profile,
  String& password) {
  profile = WiFiProfileInfo();
  password = "";

  if (this->refresh() != WiFiProfileStoreState::Ready)
    return false;

  const int metadata_index = this->metadataIndexById(id);
  const int secret_index = this->secretIndexById(id);
  if (metadata_index < 0 || secret_index < 0)
    return false;

  const WiFiProfileInfo metadata = this->metadata_profiles.get(metadata_index);
  const WiFiProfileSecret secret = this->secret_profiles.get(secret_index);
  if (metadata.ssid != secret.ssid ||
      metadata.band != secret.band ||
      metadata.channel != secret.channel)
    return false;

  profile = metadata;
  password = secret.password;
  return true;
}

bool WiFiProfileStore::remember(
  const String& ssid,
  const String& password,
  int32_t channel) {
  if (!validSSID(ssid) || !validPassword(password) || channel < 0 || channel > 177) {
    this->store_state = WiFiProfileStoreState::IOError;
    this->last_error = F("WiFi profile values are invalid");
    return false;
  }
  const uint8_t profile_channel = static_cast<uint8_t>(channel);
  const uint8_t profile_band = bandForChannel(profile_channel);

  if (this->refresh() != WiFiProfileStoreState::Ready)
    return false;

  const int metadata_index = this->metadataIndexByIdentity(ssid, profile_band);
  int secret_index = this->secretIndexByIdentity(ssid, profile_band);

  if (metadata_index >= 0) {
    const WiFiProfileInfo previous_metadata = this->metadata_profiles.get(metadata_index);
    secret_index = this->secretIndexById(previous_metadata.id);
    if (secret_index < 0) {
      this->store_state = WiFiProfileStoreState::SecretsCorrupt;
      this->last_error = F("Saved WiFi credential does not match its profile");
      return false;
    }

    const WiFiProfileSecret previous_secret = this->secret_profiles.get(secret_index);
    if (previous_secret.ssid != previous_metadata.ssid ||
        previous_secret.band != previous_metadata.band ||
        previous_secret.channel != previous_metadata.channel) {
      this->store_state = WiFiProfileStoreState::SecretsCorrupt;
      this->last_error = F("Saved WiFi credential does not match its profile");
      return false;
    }

    WiFiProfileSecret updated_secret = previous_secret;
    updated_secret.password = password;
    updated_secret.channel = profile_channel;
    this->secret_profiles.set(secret_index, updated_secret);
    if (!this->writeSecrets()) {
      this->refresh();
      this->store_state = WiFiProfileStoreState::IOError;
      this->last_error = F("Could not update saved WiFi credentials");
      return false;
    }

    WiFiProfileInfo updated_metadata = previous_metadata;
    updated_metadata.channel = profile_channel;
    this->metadata_profiles.set(metadata_index, updated_metadata);
    if (!this->writeMetadata()) {
      this->secret_profiles.set(secret_index, previous_secret);
      const bool rollback_saved = this->writeSecrets();
      this->refresh();
      this->store_state = WiFiProfileStoreState::IOError;
      this->last_error = rollback_saved
        ? String(F("Could not update WiFi profile metadata"))
        : String(F("Could not update metadata or roll back credentials"));
      return false;
    }

    this->rebuildVisibleProfiles();
    this->store_state = WiFiProfileStoreState::Ready;
    this->last_error = "";
    return true;
  }

  if (secret_index >= 0) {
    this->store_state = WiFiProfileStoreState::SecretsCorrupt;
    this->last_error = F("Saved WiFi identity has no matching profile");
    return false;
  }

  if (this->metadata_profiles.size() >= MAX_PROFILES) {
    this->store_state = WiFiProfileStoreState::Full;
    this->last_error = F("WiFi profile storage is full");
    return false;
  }

  if (this->secret_profiles.size() >= MAX_PROFILES) {
    this->store_state = WiFiProfileStoreState::Full;
    this->last_error = F("WiFi credential storage is full");
    return false;
  }

  const uint32_t id = this->generateId();
  if (id == 0) {
    this->store_state = WiFiProfileStoreState::IOError;
    this->last_error = F("Could not create a WiFi profile identifier");
    return false;
  }

  this->secret_profiles.add(WiFiProfileSecret{
    id,
    ssid,
    profile_band,
    profile_channel,
    password});

  if (!this->writeSecrets()) {
    this->refresh();
    this->store_state = WiFiProfileStoreState::IOError;
    this->last_error = F("Could not save WiFi credentials");
    return false;
  }

  this->metadata_profiles.add(WiFiProfileInfo{id, ssid, profile_band, profile_channel});
  if (!this->writeMetadata()) {
    const int rollback_index = this->secretIndexById(id);
    if (rollback_index >= 0)
      this->secret_profiles.remove(rollback_index);
    const bool rollback_saved = this->writeSecrets();
    this->refresh();
    this->store_state = WiFiProfileStoreState::IOError;
    this->last_error = rollback_saved
      ? String(F("Could not save WiFi profile metadata"))
      : String(F("Could not save metadata or roll back credentials"));
    return false;
  }

  this->rebuildVisibleProfiles();
  this->store_state = WiFiProfileStoreState::Ready;
  this->last_error = "";
  return true;
}

bool WiFiProfileStore::forget(uint32_t id) {
  if (this->refresh() != WiFiProfileStoreState::Ready)
    return false;

  const int metadata_index = this->metadataIndexById(id);
  const int secret_index = this->secretIndexById(id);
  if (metadata_index < 0) {
    this->last_error = F("Saved WiFi profile no longer exists");
    return false;
  }

  const WiFiProfileInfo metadata = this->metadata_profiles.get(metadata_index);
  if (secret_index < 0) {
    this->last_error = F("Saved WiFi credentials do not match the profile");
    return false;
  }

  const WiFiProfileSecret secret = this->secret_profiles.get(secret_index);
  if (secret.ssid != metadata.ssid ||
      secret.band != metadata.band ||
      secret.channel != metadata.channel) {
    this->last_error = F("Saved WiFi credentials do not match the profile");
    return false;
  }
  this->secret_profiles.remove(secret_index);
  if (!this->writeSecrets()) {
    this->refresh();
    this->store_state = WiFiProfileStoreState::IOError;
    this->last_error = F("Could not remove saved WiFi credentials");
    return false;
  }

  this->metadata_profiles.remove(metadata_index);
  if (!this->writeMetadata()) {
    this->secret_profiles.add(secret);
    const bool rollback_saved = this->writeSecrets();
    this->refresh();
    this->store_state = WiFiProfileStoreState::IOError;
    this->last_error = rollback_saved
      ? String(F("Could not remove WiFi profile metadata"))
      : String(F("Could not remove metadata or restore credentials"));
    return false;
  }

  bool legacy_removed = true;
  const String legacy_ssid = settings_obj.loadSetting<String>("ClientSSID");
  String legacy_password = settings_obj.loadSetting<String>("ClientPW");
  if (legacy_ssid == metadata.ssid) {
    bool matches_remaining_profile = false;
    for (int i = 0; i < this->secret_profiles.size(); ++i) {
      const WiFiProfileSecret remaining = this->secret_profiles.get(i);
      if (remaining.ssid == legacy_ssid && remaining.password == legacy_password) {
        matches_remaining_profile = true;
        break;
      }
    }
    if (!matches_remaining_profile)
      legacy_removed = settings_obj.saveWiFiCredentials("", "");
  }
  legacy_password = "";

  this->refresh();
  if (!legacy_removed) {
    this->store_state = WiFiProfileStoreState::IOError;
    this->last_error = F("WiFi profile removed, but last-used credentials remain");
    return false;
  }
  return true;
}

bool WiFiProfileStore::reset() {
  if (!this->initialized) {
    this->store_state = WiFiProfileStoreState::Uninitialized;
    this->last_error = F("WiFi profile store is not initialized");
    return false;
  }

  #ifndef HAS_SD
    this->store_state = WiFiProfileStoreState::NoSD;
    this->last_error = F("SD card support is unavailable");
    return false;
  #else
    this->sd_mounted = sd_obj.supported;
    if (!this->sd_mounted) {
      this->store_state = WiFiProfileStoreState::NoSD;
      this->last_error = F("SD card is unavailable");
      return false;
    }

    this->metadata_profiles.clear();
    this->visible_profiles.clear();
    if (!this->writeMetadata()) {
      this->store_state = WiFiProfileStoreState::IOError;
      this->last_error = F("Could not reset WiFi profile metadata");
      return false;
    }

    this->secret_profiles.clear();
    if (!this->writeSecrets()) {
      this->store_state = WiFiProfileStoreState::IOError;
      this->last_error = F("WiFi profiles were hidden but secrets remain");
      return false;
    }

    if (!settings_obj.saveWiFiCredentials("", "")) {
      this->store_state = WiFiProfileStoreState::IOError;
      this->last_error = F("WiFi profiles were reset but legacy credentials remain");
      return false;
    }

    this->store_state = WiFiProfileStoreState::Ready;
    this->last_error = "";
    return true;
  #endif
}

bool WiFiProfileStore::migrateLegacy(bool secrets_exist) {
  this->metadata_profiles.clear();
  if (secrets_exist)
    return this->rebuildMetadataFromSecrets();

  const String legacy_ssid = settings_obj.loadSetting<String>("ClientSSID");
  const String legacy_password = settings_obj.loadSetting<String>("ClientPW");

  this->secret_profiles.clear();

  if (legacy_ssid.length()) {
    if (!validSSID(legacy_ssid) || !validPassword(legacy_password)) {
      this->last_error = F("Legacy WiFi credentials are invalid");
      return false;
    }

    const uint32_t id = this->generateId();
    if (id == 0)
      return false;

    this->secret_profiles.add(WiFiProfileSecret{id, legacy_ssid, 0, 0, legacy_password});
    this->metadata_profiles.add(WiFiProfileInfo{id, legacy_ssid, 0, 0});
  }

  if (!this->writeSecrets()) {
    this->last_error = F("Could not migrate legacy WiFi credentials");
    return false;
  }
  if (!this->writeMetadata()) {
    this->last_error = F("Could not create WiFi profile metadata");
    return false;
  }
  return true;
}

bool WiFiProfileStore::rebuildMetadataFromSecrets() {
  this->metadata_profiles.clear();
  for (int i = 0; i < this->secret_profiles.size(); ++i) {
    const WiFiProfileSecret secret = this->secret_profiles.get(i);
    this->metadata_profiles.add(WiFiProfileInfo{
      secret.id,
      secret.ssid,
      secret.band,
      secret.channel});
  }

  if (!this->writeMetadata()) {
    this->last_error = F("Could not rebuild WiFi profile metadata");
    return false;
  }
  return true;
}

bool WiFiProfileStore::readMetadataFile(
  const char* path,
  LinkedList<WiFiProfileInfo>& profiles,
  int* version) const {
  if (version != nullptr)
    *version = 0;
  #ifndef HAS_SD
    (void)path;
    (void)profiles;
    return false;
  #else
    profiles.clear();
    File file = SD.open(path, FILE_READ);
    if (!file || file.isDirectory() || file.size() > STORE_FILE_LIMIT) {
      if (file)
        file.close();
      return false;
    }

    DynamicJsonDocument json(STORE_JSON_CAPACITY);
    const DeserializationError error = deserializeJson(json, file);
    file.close();
    if (error || !json["version"].is<int>())
      return false;

    const int parsed_version = json["version"].as<int>();
    if (version != nullptr)
      *version = parsed_version;
    if (parsed_version != LEGACY_STORE_VERSION &&
        parsed_version != BSSID_STORE_VERSION &&
        parsed_version != STORE_VERSION)
      return false;
    if (json.size() != 2 || !json["profiles"].is<JsonArray>())
      return false;

    JsonArray entries = json["profiles"].as<JsonArray>();
    if (entries.size() > MAX_PROFILES)
      return false;

    LinkedList<uint32_t> seen_ids;
    for (JsonVariant entry_variant : entries) {
      JsonObject entry = entry_variant.as<JsonObject>();
      const size_t expected_fields = parsed_version == LEGACY_STORE_VERSION ? 2 : 4;
      if (entry.isNull() || entry.size() != expected_fields ||
          !entry["id"].is<uint32_t>() || !entry["ssid"].is<const char*>())
        return false;

      WiFiProfileInfo profile;
      profile.id = entry["id"].as<uint32_t>();
      profile.ssid = entry["ssid"].as<String>();
      if (parsed_version == BSSID_STORE_VERSION) {
        if (!entry["bssid"].is<const char*>() || !entry["channel"].is<uint8_t>())
          return false;
        const String legacy_bssid = entry["bssid"].as<String>();
        profile.channel = entry["channel"].as<uint8_t>();
        if (legacy_bssid.length()) {
          uint8_t parsed_bssid[6] = {0};
          if (!parseBSSID(legacy_bssid, parsed_bssid))
            return false;
        }
        else if (profile.channel != 0)
          return false;
        profile.band = bandForChannel(profile.channel);
      }
      else if (parsed_version == STORE_VERSION) {
        if (!entry["band"].is<uint8_t>() || !entry["channel"].is<uint8_t>())
          return false;
        profile.band = entry["band"].as<uint8_t>();
        profile.channel = entry["channel"].as<uint8_t>();
      }

      if (profile.id == 0 || !validSSID(profile.ssid) ||
          !validBandChannel(profile.band, profile.channel))
        return false;

      for (int i = 0; i < seen_ids.size(); ++i) {
        if (seen_ids.get(i) == profile.id)
          return false;
      }
      seen_ids.add(profile.id);

      int duplicate_identity = -1;
      for (int i = 0; i < profiles.size(); ++i) {
        const WiFiProfileInfo existing = profiles.get(i);
        if (existing.id == profile.id)
          return false;
        if (existing.ssid == profile.ssid && existing.band == profile.band)
          duplicate_identity = i;
      }
      if (duplicate_identity >= 0) {
        if (parsed_version != BSSID_STORE_VERSION)
          return false;
        // V2 could contain one entry per BSSID. V3 keeps the last entry for
        // each SSID/band identity; authoritative secrets are reconciled later.
        profiles.remove(duplicate_identity);
      }
      profiles.add(profile);
    }
    return true;
  #endif
}

bool WiFiProfileStore::readSecretsFile(
  const char* path,
  LinkedList<WiFiProfileSecret>& profiles,
  int* version) const {
  if (version != nullptr)
    *version = 0;
  profiles.clear();
  File file = SPIFFS.open(path, FILE_READ);
  if (!file || file.isDirectory() || file.size() > STORE_FILE_LIMIT) {
    if (file)
      file.close();
    return false;
  }

  DynamicJsonDocument json(STORE_JSON_CAPACITY);
  const DeserializationError error = deserializeJson(json, file);
  file.close();
  if (error || !json["version"].is<int>())
    return false;

  const int parsed_version = json["version"].as<int>();
  if (version != nullptr)
    *version = parsed_version;
  if (parsed_version != LEGACY_STORE_VERSION &&
      parsed_version != BSSID_STORE_VERSION &&
      parsed_version != STORE_VERSION)
    return false;
  if (json.size() != 2 || !json["profiles"].is<JsonArray>())
    return false;

  JsonArray entries = json["profiles"].as<JsonArray>();
  if (entries.size() > MAX_PROFILES)
    return false;

  LinkedList<uint32_t> seen_ids;
  for (JsonVariant entry_variant : entries) {
    JsonObject entry = entry_variant.as<JsonObject>();
    const size_t expected_fields = parsed_version == LEGACY_STORE_VERSION ? 3 : 5;
    if (entry.isNull() || entry.size() != expected_fields || !entry["id"].is<uint32_t>() ||
        !entry["ssid"].is<const char*>() || !entry["password"].is<const char*>()) {
      return false;
    }

    WiFiProfileSecret profile;
    profile.id = entry["id"].as<uint32_t>();
    profile.ssid = entry["ssid"].as<String>();
    if (parsed_version == BSSID_STORE_VERSION) {
      if (!entry["bssid"].is<const char*>() || !entry["channel"].is<uint8_t>())
        return false;
      const String legacy_bssid = entry["bssid"].as<String>();
      profile.channel = entry["channel"].as<uint8_t>();
      if (legacy_bssid.length()) {
        uint8_t parsed_bssid[6] = {0};
        if (!parseBSSID(legacy_bssid, parsed_bssid))
          return false;
      }
      else if (profile.channel != 0)
        return false;
      profile.band = bandForChannel(profile.channel);
    }
    else if (parsed_version == STORE_VERSION) {
      if (!entry["band"].is<uint8_t>() || !entry["channel"].is<uint8_t>())
        return false;
      profile.band = entry["band"].as<uint8_t>();
      profile.channel = entry["channel"].as<uint8_t>();
    }
    profile.password = entry["password"].as<String>();
    if (profile.id == 0 || !validSSID(profile.ssid) || !validPassword(profile.password) ||
        !validBandChannel(profile.band, profile.channel))
      return false;

    for (int i = 0; i < seen_ids.size(); ++i) {
      if (seen_ids.get(i) == profile.id)
        return false;
    }
    seen_ids.add(profile.id);

    int duplicate_identity = -1;
    for (int i = 0; i < profiles.size(); ++i) {
      const WiFiProfileSecret existing = profiles.get(i);
      if (existing.id == profile.id)
        return false;
      if (existing.ssid == profile.ssid && existing.band == profile.band)
        duplicate_identity = i;
    }
    if (duplicate_identity >= 0) {
      if (parsed_version != BSSID_STORE_VERSION)
        return false;
      // File order is deterministic: removing the prior entry makes the last
      // v2 secret win for a duplicate SSID/band identity.
      profiles.remove(duplicate_identity);
    }
    profiles.add(profile);
  }
  return true;
}

bool WiFiProfileStore::loadMetadata(LinkedList<WiFiProfileInfo>& profiles, bool& any_generation) {
  this->metadata_recovery_io_error = false;
  this->metadata_version_unsupported = false;
  this->metadata_loaded_version = 0;
  #ifndef HAS_SD
    (void)profiles;
    any_generation = false;
    return false;
  #else
    any_generation = anyFileExists(SD, METADATA_PATH, METADATA_BACKUP_PATH, METADATA_TEMP_PATH);
    if (!any_generation)
      return false;

    const char* candidates[] = {METADATA_PATH, METADATA_BACKUP_PATH, METADATA_TEMP_PATH};
    for (const char* candidate : candidates) {
      if (!SD.exists(candidate))
        continue;
      int loaded_version = 0;
      if (this->readMetadataFile(candidate, profiles, &loaded_version)) {
        if (strcmp(candidate, METADATA_TEMP_PATH) == 0) {
          if (!moveIntoPlace(
                SD,
                METADATA_PATH,
                METADATA_BACKUP_PATH,
                METADATA_TEMP_PATH)) {
            this->metadata_recovery_io_error = true;
            return false;
          }
        }
        else if (strcmp(candidate, METADATA_PATH) != 0) {
          if (!restoreBackup(SD, METADATA_PATH, METADATA_BACKUP_PATH)) {
            this->metadata_recovery_io_error = true;
            return false;
          }
        }

        if (strcmp(candidate, METADATA_PATH) != 0) {
          LinkedList<WiFiProfileInfo> verified;
          int verified_version = 0;
          if (!this->readMetadataFile(METADATA_PATH, verified, &verified_version) ||
              verified_version != loaded_version) {
            this->metadata_recovery_io_error = true;
            return false;
          }
        }
        if (!cleanupOldGenerations(SD, METADATA_BACKUP_PATH, METADATA_TEMP_PATH)) {
          this->metadata_recovery_io_error = true;
          return false;
        }
        this->metadata_loaded_version = static_cast<uint8_t>(loaded_version);
        return true;
      }
      if (loaded_version > STORE_VERSION) {
        this->metadata_version_unsupported = true;
        return false;
      }
    }
    return false;
  #endif
}

bool WiFiProfileStore::loadSecrets(LinkedList<WiFiProfileSecret>& profiles, bool& any_generation) {
  this->secrets_recovery_io_error = false;
  this->secrets_version_unsupported = false;
  this->secrets_loaded_version = 0;
  any_generation = anyFileExists(SPIFFS, SECRET_PATH, SECRET_BACKUP_PATH, SECRET_TEMP_PATH);
  if (!any_generation)
    return false;

  const char* candidates[] = {SECRET_PATH, SECRET_BACKUP_PATH, SECRET_TEMP_PATH};
  for (const char* candidate : candidates) {
    if (!SPIFFS.exists(candidate))
      continue;
    int loaded_version = 0;
    if (this->readSecretsFile(candidate, profiles, &loaded_version)) {
      if (strcmp(candidate, SECRET_TEMP_PATH) == 0) {
        if (!moveIntoPlace(
              SPIFFS,
              SECRET_PATH,
              SECRET_BACKUP_PATH,
              SECRET_TEMP_PATH)) {
          this->secrets_recovery_io_error = true;
          return false;
        }
      }
      else if (strcmp(candidate, SECRET_PATH) != 0) {
        if (!restoreBackup(SPIFFS, SECRET_PATH, SECRET_BACKUP_PATH)) {
          this->secrets_recovery_io_error = true;
          return false;
        }
      }

      if (strcmp(candidate, SECRET_PATH) != 0) {
        LinkedList<WiFiProfileSecret> verified;
        int verified_version = 0;
        if (!this->readSecretsFile(SECRET_PATH, verified, &verified_version) ||
            verified_version != loaded_version) {
          this->secrets_recovery_io_error = true;
          return false;
        }
      }
      if (!cleanupOldGenerations(SPIFFS, SECRET_BACKUP_PATH, SECRET_TEMP_PATH)) {
        this->secrets_recovery_io_error = true;
        return false;
      }
      this->secrets_loaded_version = static_cast<uint8_t>(loaded_version);
      return true;
    }
    if (loaded_version > STORE_VERSION) {
      this->secrets_version_unsupported = true;
      return false;
    }
  }
  return false;
}

bool WiFiProfileStore::writeMetadata() {
  #ifndef HAS_SD
    return false;
  #else
    if (!this->sd_mounted || !sd_obj.supported)
      return false;
    if (!SD.exists(METADATA_DIRECTORY) && !SD.mkdir(METADATA_DIRECTORY))
      return false;

    if (SD.exists(METADATA_TEMP_PATH) && !SD.remove(METADATA_TEMP_PATH))
      return false;

    DynamicJsonDocument json(STORE_JSON_CAPACITY);
    json["version"] = STORE_VERSION;
    JsonArray entries = json.createNestedArray("profiles");
    for (int i = 0; i < this->metadata_profiles.size(); ++i) {
      const WiFiProfileInfo profile = this->metadata_profiles.get(i);
      JsonObject entry = entries.createNestedObject();
      entry["id"] = profile.id;
      entry["ssid"] = profile.ssid;
      entry["band"] = profile.band;
      entry["channel"] = profile.channel;
    }
    if (json.overflowed())
      return false;

    File file = SD.open(METADATA_TEMP_PATH, FILE_WRITE);
    if (!file)
      return false;
    const size_t written = serializeJson(json, file);
    file.flush();
    file.close();
    if (written == 0)
      return false;

    LinkedList<WiFiProfileInfo> verified;
    if (!this->readMetadataFile(METADATA_TEMP_PATH, verified)) {
      SD.remove(METADATA_TEMP_PATH);
      return false;
    }
    if (verified.size() != this->metadata_profiles.size()) {
      SD.remove(METADATA_TEMP_PATH);
      return false;
    }
    for (int i = 0; i < verified.size(); ++i) {
      const WiFiProfileInfo expected = this->metadata_profiles.get(i);
      const WiFiProfileInfo actual = verified.get(i);
      if (actual.id != expected.id || actual.ssid != expected.ssid ||
          actual.band != expected.band || actual.channel != expected.channel) {
        SD.remove(METADATA_TEMP_PATH);
        return false;
      }
    }

    if (!moveIntoPlace(SD, METADATA_PATH, METADATA_BACKUP_PATH, METADATA_TEMP_PATH))
      return false;

    if (!this->readMetadataFile(METADATA_PATH, verified) ||
        verified.size() != this->metadata_profiles.size()) {
      SD.remove(METADATA_PATH);
      if (SD.exists(METADATA_BACKUP_PATH))
        SD.rename(METADATA_BACKUP_PATH, METADATA_PATH);
      return false;
    }
    for (int i = 0; i < verified.size(); ++i) {
      const WiFiProfileInfo expected = this->metadata_profiles.get(i);
      const WiFiProfileInfo actual = verified.get(i);
      if (actual.id != expected.id || actual.ssid != expected.ssid ||
          actual.band != expected.band || actual.channel != expected.channel) {
        SD.remove(METADATA_PATH);
        if (SD.exists(METADATA_BACKUP_PATH))
          SD.rename(METADATA_BACKUP_PATH, METADATA_PATH);
        return false;
      }
    }

    return cleanupOldGenerations(SD, METADATA_BACKUP_PATH, METADATA_TEMP_PATH);
  #endif
}

bool WiFiProfileStore::writeSecrets() {
  if (SPIFFS.exists(SECRET_TEMP_PATH) && !SPIFFS.remove(SECRET_TEMP_PATH))
    return false;

  DynamicJsonDocument json(STORE_JSON_CAPACITY);
  json["version"] = STORE_VERSION;
  JsonArray entries = json.createNestedArray("profiles");
  for (int i = 0; i < this->secret_profiles.size(); ++i) {
    const WiFiProfileSecret profile = this->secret_profiles.get(i);
    JsonObject entry = entries.createNestedObject();
    entry["id"] = profile.id;
    entry["ssid"] = profile.ssid;
    entry["band"] = profile.band;
    entry["channel"] = profile.channel;
    entry["password"] = profile.password;
  }
  if (json.overflowed())
    return false;

  File file = SPIFFS.open(SECRET_TEMP_PATH, FILE_WRITE);
  if (!file)
    return false;
  const size_t written = serializeJson(json, file);
  file.flush();
  file.close();
  if (written == 0)
    return false;

  LinkedList<WiFiProfileSecret> verified;
  if (!this->readSecretsFile(SECRET_TEMP_PATH, verified)) {
    SPIFFS.remove(SECRET_TEMP_PATH);
    return false;
  }
  if (verified.size() != this->secret_profiles.size()) {
    SPIFFS.remove(SECRET_TEMP_PATH);
    return false;
  }
  for (int i = 0; i < verified.size(); ++i) {
    const WiFiProfileSecret expected = this->secret_profiles.get(i);
    const WiFiProfileSecret actual = verified.get(i);
    if (actual.id != expected.id || actual.ssid != expected.ssid ||
        actual.band != expected.band || actual.channel != expected.channel ||
        actual.password != expected.password) {
      SPIFFS.remove(SECRET_TEMP_PATH);
      return false;
    }
  }

  if (!moveIntoPlace(SPIFFS, SECRET_PATH, SECRET_BACKUP_PATH, SECRET_TEMP_PATH))
    return false;

  if (!this->readSecretsFile(SECRET_PATH, verified) ||
      verified.size() != this->secret_profiles.size()) {
    SPIFFS.remove(SECRET_PATH);
    if (SPIFFS.exists(SECRET_BACKUP_PATH))
      SPIFFS.rename(SECRET_BACKUP_PATH, SECRET_PATH);
    return false;
  }
  for (int i = 0; i < verified.size(); ++i) {
    const WiFiProfileSecret expected = this->secret_profiles.get(i);
    const WiFiProfileSecret actual = verified.get(i);
    if (actual.id != expected.id || actual.ssid != expected.ssid ||
        actual.band != expected.band || actual.channel != expected.channel ||
        actual.password != expected.password) {
      SPIFFS.remove(SECRET_PATH);
      if (SPIFFS.exists(SECRET_BACKUP_PATH))
        SPIFFS.rename(SECRET_BACKUP_PATH, SECRET_PATH);
      return false;
    }
  }

  return cleanupOldGenerations(SPIFFS, SECRET_BACKUP_PATH, SECRET_TEMP_PATH);
}
