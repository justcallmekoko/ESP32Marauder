#pragma once

#include <stddef.h>
#include <stdint.h>

namespace marauder {

constexpr size_t kOuiMacAddressSize = 6;
constexpr size_t kOuiHeaderSize = 24;
constexpr size_t kOuiPrefixKeySize = 5;
constexpr size_t kOuiNameSize = 24;
constexpr size_t kOuiRecordSize = kOuiPrefixKeySize + kOuiNameSize;
static_assert(kOuiRecordSize == 29, "Unexpected OUI record size");

// Read-only view of one complete MROUI001 database. Implementations retain
// ownership of their storage and must remain valid while OuiDatabase is open.
class OuiByteReader {
 public:
  virtual ~OuiByteReader() = default;

  virtual size_t size() const = 0;
  virtual bool read(size_t offset, uint8_t* destination,
                    size_t length) const = 0;
};

enum class OuiOpenStatus : uint8_t {
  kReady,
  kInvalidArgument,
  kReadError,
  kInvalidFormat,
};

enum class OuiClassification : uint8_t {
  kInvalid,
  kBroadcast,
  kMulticast,
  kLocal,
  kVendor,
  kUnknown,
};

enum class OuiLookupStatus : uint8_t {
  kSuccess,
  kDatabaseNotOpen,
  kReadError,
  kInvalidRecord,
};

struct MacIdentity {
  OuiClassification classification;
  uint8_t prefix_length;
  // An owned, NUL-terminated copy. It never points into reader storage.
  char vendor[kOuiNameSize];
};

using OuiLookupResult = MacIdentity;

OuiClassification classifyOuiMac(
    const uint8_t mac[kOuiMacAddressSize]);

MacIdentity identifyMacAddress(
    const uint8_t mac[kOuiMacAddressSize],
    const OuiByteReader* reader);

// Parses the header once and supports repeated lookups against the same reader.
// Records are binary-searched in longest-prefix order: /36, /28, then /24.
class OuiDatabase {
 public:
  OuiDatabase();

  OuiOpenStatus open(const OuiByteReader* reader);
  void close();
  bool isOpen() const;

  OuiLookupStatus lookup(const uint8_t mac[kOuiMacAddressSize],
                         OuiLookupResult& result) const;

 private:
  struct Section {
    size_t offset;
    uint32_t count;
    uint8_t prefix_length;
  };

  OuiLookupStatus findInSection(const Section& section,
                                const uint8_t key[kOuiPrefixKeySize],
                                OuiLookupResult& result,
                                bool& found) const;

  const OuiByteReader* reader_;
  Section sections_[3];
};

}  // namespace marauder
