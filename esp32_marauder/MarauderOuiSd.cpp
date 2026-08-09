#include "MarauderOuiSd.h"

#ifdef HAS_OUI_LABELS

#ifndef HAS_SD
  #error "HAS_OUI_LABELS requires HAS_SD"
#endif

#include <limits.h>

#include "SD.h"
#include "SDInterface.h"

extern SDInterface sd_obj;

namespace marauder {
namespace {

StoredMacIdentity makeUnresolvedIdentity(const uint8_t* mac,
                                         OuiStorageStatus status) {
  StoredMacIdentity result = {};
  result.identity = identifyMacAddress(mac, nullptr);
  result.storage_status = status;
  return result;
}

}  // namespace

SdOuiDatabase::SdOuiDatabase()
    : database_file_(),
      database_(),
      status_(OuiStorageStatus::kUnavailable) {}

SdOuiDatabase::~SdOuiDatabase() { close(); }

OuiStorageStatus SdOuiDatabase::open() {
  close();
  if (!sd_obj.supported) {
    return status_;
  }
  if (!SD.exists(kOuiDatabasePath)) {
    File root = SD.open("/", FILE_READ);
    if (!root) {
      status_ = OuiStorageStatus::kInvalid;
    } else {
      root.close();
    }
    return status_;
  }

  database_file_ = SD.open(kOuiDatabasePath, FILE_READ);
  if (!database_file_) {
    status_ = OuiStorageStatus::kInvalid;
    return status_;
  }

  if (database_.open(this) != OuiOpenStatus::kReady) {
    database_file_.close();
    status_ = OuiStorageStatus::kInvalid;
    return status_;
  }

  status_ = OuiStorageStatus::kReady;
  return status_;
}

void SdOuiDatabase::close() {
  database_.close();
  if (database_file_) {
    database_file_.close();
  }
  status_ = OuiStorageStatus::kUnavailable;
}

OuiStorageStatus SdOuiDatabase::status() const { return status_; }

StoredMacIdentity SdOuiDatabase::identify(
    const uint8_t mac[kOuiMacAddressSize]) {
  StoredMacIdentity result = makeUnresolvedIdentity(mac, status_);
  if (result.identity.classification != OuiClassification::kUnknown ||
      status_ != OuiStorageStatus::kReady) {
    return result;
  }

  if (database_.lookup(mac, result.identity) != OuiLookupStatus::kSuccess) {
    invalidate();
    result = makeUnresolvedIdentity(mac, status_);
  }
  return result;
}

void SdOuiDatabase::invalidate() {
  database_.close();
  if (database_file_) {
    database_file_.close();
  }
  status_ = OuiStorageStatus::kInvalid;
}

size_t SdOuiDatabase::size() const { return database_file_.size(); }

bool SdOuiDatabase::read(size_t offset, uint8_t* destination,
                         size_t length) const {
  if (destination == nullptr || offset > UINT32_MAX ||
      !database_file_.seek(static_cast<uint32_t>(offset))) {
    return false;
  }
  return database_file_.read(destination, length) == length;
}

StoredMacIdentity identifyMacAddressFromSd(
    const uint8_t mac[kOuiMacAddressSize]) {
  StoredMacIdentity result =
      makeUnresolvedIdentity(mac, OuiStorageStatus::kUnavailable);
  if (result.identity.classification != OuiClassification::kUnknown) {
    return result;
  }

  SdOuiDatabase database;
  database.open();
  return database.identify(mac);
}

const char* ouiIdentityLabel(const StoredMacIdentity& result) {
  switch (result.identity.classification) {
    case OuiClassification::kVendor:
      return result.identity.vendor;
    case OuiClassification::kLocal:
      return "local/private";
    case OuiClassification::kMulticast:
      return "multicast";
    case OuiClassification::kBroadcast:
      return "broadcast";
    case OuiClassification::kInvalid:
      return "invalid";
    case OuiClassification::kUnknown:
      if (result.storage_status == OuiStorageStatus::kUnavailable) {
        return "no database";
      }
      if (result.storage_status == OuiStorageStatus::kInvalid) {
        return "database error";
      }
      return "unknown";
  }
  return "unknown";
}

}  // namespace marauder

#endif  // HAS_OUI_LABELS
