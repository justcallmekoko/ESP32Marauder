#include "MarauderOui.h"

#include <limits.h>
#include <string.h>

namespace marauder {
namespace {

constexpr uint8_t kOuiMagic[8] = {'M', 'R', 'O', 'U',
                                  'I', '0', '0', '1'};
constexpr uint8_t kOuiFormatVersion = 1;
constexpr size_t kOuiCount24Offset = 12;
constexpr size_t kOuiCount28Offset = 16;
constexpr size_t kOuiCount36Offset = 20;

uint32_t readUint32LittleEndian(const uint8_t* input) {
  return static_cast<uint32_t>(input[0]) |
         (static_cast<uint32_t>(input[1]) << 8) |
         (static_cast<uint32_t>(input[2]) << 16) |
         (static_cast<uint32_t>(input[3]) << 24);
}

bool isAll(const uint8_t mac[kOuiMacAddressSize], uint8_t value) {
  for (size_t index = 0; index < kOuiMacAddressSize; ++index) {
    if (mac[index] != value) {
      return false;
    }
  }
  return true;
}

void clearResult(OuiLookupResult& result) {
  result.classification = OuiClassification::kUnknown;
  result.prefix_length = 0;
  memset(result.vendor, 0, sizeof(result.vendor));
}

void makePrefixKey(const uint8_t mac[kOuiMacAddressSize],
                   uint8_t prefix_length,
                   uint8_t key[kOuiPrefixKeySize]) {
  memset(key, 0, kOuiPrefixKeySize);

  const size_t whole_bytes = prefix_length / 8;
  memcpy(key, mac, whole_bytes);
  if ((prefix_length % 8) != 0) {
    const uint8_t partial_mask =
        static_cast<uint8_t>(0xFFU << (8 - (prefix_length % 8)));
    key[whole_bytes] = mac[whole_bytes] & partial_mask;
  }
}

int compareKeys(const uint8_t left[kOuiPrefixKeySize],
                const uint8_t right[kOuiPrefixKeySize]) {
  for (size_t index = 0; index < kOuiPrefixKeySize; ++index) {
    if (left[index] < right[index]) {
      return -1;
    }
    if (left[index] > right[index]) {
      return 1;
    }
  }
  return 0;
}

bool copyValidName(const uint8_t raw_name[kOuiNameSize],
                   char destination[kOuiNameSize]) {
  size_t length = 0;
  while (length < kOuiNameSize && raw_name[length] != 0) {
    if (raw_name[length] < 0x20 || raw_name[length] > 0x7E) {
      return false;
    }
    ++length;
  }

  if (length == 0 || length == kOuiNameSize) {
    return false;
  }

  memset(destination, 0, kOuiNameSize);
  memcpy(destination, raw_name, length);
  return true;
}

}  // namespace

OuiClassification classifyOuiMac(
    const uint8_t mac[kOuiMacAddressSize]) {
  if (mac == nullptr || isAll(mac, 0x00)) {
    return OuiClassification::kInvalid;
  }
  if (isAll(mac, 0xFF)) {
    return OuiClassification::kBroadcast;
  }
  if ((mac[0] & 0x01U) != 0) {
    return OuiClassification::kMulticast;
  }
  if ((mac[0] & 0x02U) != 0) {
    return OuiClassification::kLocal;
  }
  return OuiClassification::kUnknown;
}

MacIdentity identifyMacAddress(
    const uint8_t mac[kOuiMacAddressSize],
    const OuiByteReader* reader) {
  MacIdentity identity = {};
  identity.classification = classifyOuiMac(mac);
  if (identity.classification != OuiClassification::kUnknown) {
    return identity;
  }

  OuiDatabase database;
  if (database.open(reader) != OuiOpenStatus::kReady ||
      database.lookup(mac, identity) != OuiLookupStatus::kSuccess) {
    clearResult(identity);
  }
  return identity;
}

OuiDatabase::OuiDatabase() : reader_(nullptr), sections_{} {}

OuiOpenStatus OuiDatabase::open(const OuiByteReader* reader) {
  close();
  if (reader == nullptr) {
    return OuiOpenStatus::kInvalidArgument;
  }
  if (reader->size() < kOuiHeaderSize) {
    return OuiOpenStatus::kInvalidFormat;
  }

  uint8_t header[kOuiHeaderSize] = {};
  if (!reader->read(0, header, sizeof(header))) {
    return OuiOpenStatus::kReadError;
  }

  if (memcmp(header, kOuiMagic, sizeof(kOuiMagic)) != 0 ||
      header[8] != kOuiFormatVersion || header[9] != kOuiRecordSize ||
      header[10] != kOuiNameSize || header[11] != 0) {
    return OuiOpenStatus::kInvalidFormat;
  }

  const uint32_t counts[3] = {
      readUint32LittleEndian(header + kOuiCount24Offset),
      readUint32LittleEndian(header + kOuiCount28Offset),
      readUint32LittleEndian(header + kOuiCount36Offset),
  };
  const uint8_t prefix_lengths[3] = {24, 28, 36};

  uint64_t next_offset = kOuiHeaderSize;
  for (size_t index = 0; index < 3; ++index) {
    if (next_offset > static_cast<uint64_t>(SIZE_MAX)) {
      return OuiOpenStatus::kInvalidFormat;
    }
    sections_[index].offset = static_cast<size_t>(next_offset);
    sections_[index].count = counts[index];
    sections_[index].prefix_length = prefix_lengths[index];
    next_offset += static_cast<uint64_t>(counts[index]) * kOuiRecordSize;
  }

  if (next_offset != static_cast<uint64_t>(reader->size())) {
    close();
    return OuiOpenStatus::kInvalidFormat;
  }

  reader_ = reader;
  return OuiOpenStatus::kReady;
}

void OuiDatabase::close() {
  reader_ = nullptr;
  memset(sections_, 0, sizeof(sections_));
}

bool OuiDatabase::isOpen() const { return reader_ != nullptr; }

OuiLookupStatus OuiDatabase::lookup(
    const uint8_t mac[kOuiMacAddressSize], OuiLookupResult& result) const {
  clearResult(result);
  result.classification = classifyOuiMac(mac);
  if (result.classification != OuiClassification::kUnknown) {
    return OuiLookupStatus::kSuccess;
  }
  if (reader_ == nullptr) {
    return OuiLookupStatus::kDatabaseNotOpen;
  }

  for (size_t reverse_index = 3; reverse_index > 0; --reverse_index) {
    const Section& section = sections_[reverse_index - 1];
    uint8_t key[kOuiPrefixKeySize] = {};
    makePrefixKey(mac, section.prefix_length, key);

    bool found = false;
    const OuiLookupStatus status =
        findInSection(section, key, result, found);
    if (status != OuiLookupStatus::kSuccess) {
      return status;
    }
    if (found) {
      result.classification = OuiClassification::kVendor;
      result.prefix_length = section.prefix_length;
      return OuiLookupStatus::kSuccess;
    }
  }

  return OuiLookupStatus::kSuccess;
}

OuiLookupStatus OuiDatabase::findInSection(
    const Section& section, const uint8_t key[kOuiPrefixKeySize],
    OuiLookupResult& result, bool& found) const {
  found = false;
  uint32_t lower = 0;
  uint32_t upper = section.count;

  while (lower < upper) {
    const uint32_t middle = lower + ((upper - lower) / 2);
    const size_t record_offset =
        section.offset + static_cast<size_t>(middle) * kOuiRecordSize;
    uint8_t record_key[kOuiPrefixKeySize] = {};
    if (!reader_->read(record_offset, record_key, sizeof(record_key))) {
      return OuiLookupStatus::kReadError;
    }

    const int comparison = compareKeys(record_key, key);
    if (comparison < 0) {
      lower = middle + 1;
    } else if (comparison > 0) {
      upper = middle;
    } else {
      uint8_t raw_name[kOuiNameSize] = {};
      if (!reader_->read(record_offset + kOuiPrefixKeySize, raw_name,
                         sizeof(raw_name))) {
        return OuiLookupStatus::kReadError;
      }
      if (!copyValidName(raw_name, result.vendor)) {
        return OuiLookupStatus::kInvalidRecord;
      }
      found = true;
      return OuiLookupStatus::kSuccess;
    }
  }

  return OuiLookupStatus::kSuccess;
}

}  // namespace marauder
