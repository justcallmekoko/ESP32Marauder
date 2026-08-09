#pragma once

#include "configs.h"

#ifdef HAS_OUI_LABELS

#include "MarauderOui.h"
#include "SD.h"

namespace marauder {

constexpr char kOuiDatabasePath[] = "/marauder_oui.bin";

enum class OuiStorageStatus : uint8_t {
  kReady,
  kUnavailable,
  kInvalid,
};

struct StoredMacIdentity {
  MacIdentity identity;
  OuiStorageStatus storage_status;
};

class SdOuiDatabase : private OuiByteReader {
 public:
  SdOuiDatabase();
  ~SdOuiDatabase();

  SdOuiDatabase(const SdOuiDatabase&) = delete;
  SdOuiDatabase& operator=(const SdOuiDatabase&) = delete;

  OuiStorageStatus open();
  void close();
  OuiStorageStatus status() const;
  StoredMacIdentity identify(
      const uint8_t mac[kOuiMacAddressSize]);

 private:
  void invalidate();
  size_t size() const override;
  bool read(size_t offset, uint8_t* destination,
            size_t length) const override;

  mutable File database_file_;
  OuiDatabase database_;
  OuiStorageStatus status_;
};

StoredMacIdentity identifyMacAddressFromSd(
    const uint8_t mac[kOuiMacAddressSize]);

const char* ouiIdentityLabel(const StoredMacIdentity& result);

}  // namespace marauder

#endif  // HAS_OUI_LABELS
