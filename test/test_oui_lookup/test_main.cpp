#include <unity.h>

#include <array>
#include <cstring>
#include <utility>
#include <vector>

#include "MarauderOui.h"

namespace {

using marauder::OuiByteReader;
using marauder::OuiClassification;
using marauder::OuiDatabase;
using marauder::OuiLookupResult;
using marauder::OuiLookupStatus;
using marauder::OuiOpenStatus;

constexpr size_t kNoReadFailure = static_cast<size_t>(-1);

class MemoryReader : public OuiByteReader {
 public:
  explicit MemoryReader(std::vector<uint8_t> bytes)
      : bytes_(std::move(bytes)), fail_at_(kNoReadFailure), reads_(0) {}

  size_t size() const override { return bytes_.size(); }

  bool read(size_t offset, uint8_t* destination,
            size_t length) const override {
    ++reads_;
    if (destination == nullptr || offset > bytes_.size() ||
        length > bytes_.size() - offset) {
      return false;
    }
    if (fail_at_ != kNoReadFailure && offset <= fail_at_ &&
        length > fail_at_ - offset) {
      return false;
    }
    std::memcpy(destination, bytes_.data() + offset, length);
    return true;
  }

  void failAt(size_t offset) { fail_at_ = offset; }
  size_t reads() const { return reads_; }
  void resetReads() const { reads_ = 0; }

 private:
  std::vector<uint8_t> bytes_;
  size_t fail_at_;
  mutable size_t reads_;
};

struct Record {
  std::array<uint8_t, marauder::kOuiPrefixKeySize> key;
  const char* name;
};

void appendLittleEndian(std::vector<uint8_t>& output, uint32_t value) {
  output.push_back(static_cast<uint8_t>(value));
  output.push_back(static_cast<uint8_t>(value >> 8));
  output.push_back(static_cast<uint8_t>(value >> 16));
  output.push_back(static_cast<uint8_t>(value >> 24));
}

void appendRecord(std::vector<uint8_t>& output, const Record& record) {
  output.insert(output.end(), record.key.begin(), record.key.end());
  std::array<uint8_t, marauder::kOuiNameSize> name = {};
  const size_t length = std::strlen(record.name);
  TEST_ASSERT_LESS_THAN(marauder::kOuiNameSize, length);
  std::memcpy(name.data(), record.name, length);
  output.insert(output.end(), name.begin(), name.end());
}

std::vector<uint8_t> makeDatabase(const std::vector<Record>& records24,
                                  const std::vector<Record>& records28,
                                  const std::vector<Record>& records36) {
  std::vector<uint8_t> output = {'M', 'R', 'O', 'U', 'I', '0', '0', '1',
                                 1,   29,  24,  0};
  appendLittleEndian(output, static_cast<uint32_t>(records24.size()));
  appendLittleEndian(output, static_cast<uint32_t>(records28.size()));
  appendLittleEndian(output, static_cast<uint32_t>(records36.size()));
  for (const Record& record : records24) {
    appendRecord(output, record);
  }
  for (const Record& record : records28) {
    appendRecord(output, record);
  }
  for (const Record& record : records36) {
    appendRecord(output, record);
  }
  return output;
}

MemoryReader makeFullDatabase() {
  return MemoryReader(makeDatabase(
      {{{0x00, 0x11, 0x22, 0x00, 0x00}, "Vendor 24"},
       {{0x10, 0x20, 0x30, 0x00, 0x00}, "Second 24"}},
      {{{0x00, 0x11, 0x22, 0x30, 0x00}, "Vendor 28"},
       {{0x10, 0x20, 0x30, 0x40, 0x00}, "Second 28"}},
      {{{0x00, 0x11, 0x22, 0x33, 0x40}, "Vendor 36"},
       {{0x10, 0x20, 0x30, 0x40, 0x50}, "Second 36"}}));
}

void assertClassification(const uint8_t* mac,
                          OuiClassification expected) {
  MemoryReader reader = makeFullDatabase();
  OuiDatabase database;
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OuiOpenStatus::kReady),
                        static_cast<int>(database.open(&reader)));
  reader.resetReads();

  OuiLookupResult result = {};
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(OuiLookupStatus::kSuccess),
      static_cast<int>(database.lookup(mac, result)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(expected),
                        static_cast<int>(result.classification));
  TEST_ASSERT_EQUAL_UINT8(0, result.prefix_length);
  TEST_ASSERT_EQUAL_STRING("", result.vendor);
  TEST_ASSERT_EQUAL_UINT32(0, reader.reads());
}

}  // namespace

void setUp() {}

void tearDown() {}

void test_special_mac_classes_do_not_read_database() {
  const uint8_t invalid[6] = {};
  const uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  const uint8_t multicast[6] = {0x01, 0x00, 0x5E, 0x01, 0x02, 0x03};
  const uint8_t local[6] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x55};

  assertClassification(nullptr, OuiClassification::kInvalid);
  assertClassification(invalid, OuiClassification::kInvalid);
  assertClassification(broadcast, OuiClassification::kBroadcast);
  assertClassification(multicast, OuiClassification::kMulticast);
  assertClassification(local, OuiClassification::kLocal);
}

void test_lookup_prefers_36_then_28_then_24_bit_prefixes() {
  MemoryReader reader = makeFullDatabase();
  OuiDatabase database;
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OuiOpenStatus::kReady),
                        static_cast<int>(database.open(&reader)));

  const uint8_t mac36[6] = {0x00, 0x11, 0x22, 0x33, 0x4F, 0xAA};
  const uint8_t mac28[6] = {0x00, 0x11, 0x22, 0x3F, 0xA0, 0x01};
  const uint8_t mac24[6] = {0x00, 0x11, 0x22, 0xA0, 0x00, 0x01};
  OuiLookupResult result = {};

  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(OuiLookupStatus::kSuccess),
      static_cast<int>(database.lookup(mac36, result)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OuiClassification::kVendor),
                        static_cast<int>(result.classification));
  TEST_ASSERT_EQUAL_UINT8(36, result.prefix_length);
  TEST_ASSERT_EQUAL_STRING("Vendor 36", result.vendor);

  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(OuiLookupStatus::kSuccess),
      static_cast<int>(database.lookup(mac28, result)));
  TEST_ASSERT_EQUAL_UINT8(28, result.prefix_length);
  TEST_ASSERT_EQUAL_STRING("Vendor 28", result.vendor);

  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(OuiLookupStatus::kSuccess),
      static_cast<int>(database.lookup(mac24, result)));
  TEST_ASSERT_EQUAL_UINT8(24, result.prefix_length);
  TEST_ASSERT_EQUAL_STRING("Vendor 24", result.vendor);
}

void test_lookup_uses_big_endian_sorted_keys_and_section_boundaries() {
  MemoryReader reader = makeFullDatabase();
  OuiDatabase database;
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OuiOpenStatus::kReady),
                        static_cast<int>(database.open(&reader)));

  const uint8_t first[6] = {0x00, 0x11, 0x22, 0x33, 0x4A, 0x01};
  const uint8_t last[6] = {0x10, 0x20, 0x30, 0x40, 0x5F, 0x01};
  OuiLookupResult result = {};

  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(OuiLookupStatus::kSuccess),
      static_cast<int>(database.lookup(first, result)));
  TEST_ASSERT_EQUAL_STRING("Vendor 36", result.vendor);

  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(OuiLookupStatus::kSuccess),
      static_cast<int>(database.lookup(last, result)));
  TEST_ASSERT_EQUAL_UINT8(36, result.prefix_length);
  TEST_ASSERT_EQUAL_STRING("Second 36", result.vendor);
}

void test_global_mac_without_match_is_unknown() {
  MemoryReader reader = makeFullDatabase();
  OuiDatabase database;
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OuiOpenStatus::kReady),
                        static_cast<int>(database.open(&reader)));
  const uint8_t mac[6] = {0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
  OuiLookupResult result = {};

  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(OuiLookupStatus::kSuccess),
      static_cast<int>(database.lookup(mac, result)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OuiClassification::kUnknown),
                        static_cast<int>(result.classification));
  TEST_ASSERT_EQUAL_UINT8(0, result.prefix_length);
  TEST_ASSERT_EQUAL_STRING("", result.vendor);
}

void test_identify_mac_address_returns_owned_name_and_tolerates_null_reader() {
  MemoryReader reader = makeFullDatabase();
  const uint8_t mac[6] = {0x00, 0x11, 0x22, 0x33, 0x4F, 0xAA};

  marauder::MacIdentity identity =
      marauder::identifyMacAddress(mac, &reader);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OuiClassification::kVendor),
                        static_cast<int>(identity.classification));
  TEST_ASSERT_EQUAL_UINT8(36, identity.prefix_length);
  TEST_ASSERT_EQUAL_STRING("Vendor 36", identity.vendor);

  reader = MemoryReader(makeDatabase({}, {}, {}));
  TEST_ASSERT_EQUAL_STRING("Vendor 36", identity.vendor);

  identity = marauder::identifyMacAddress(mac, nullptr);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OuiClassification::kUnknown),
                        static_cast<int>(identity.classification));
  TEST_ASSERT_EQUAL_UINT8(0, identity.prefix_length);
  TEST_ASSERT_EQUAL_STRING("", identity.vendor);
}

void test_open_validates_exact_header_and_file_size() {
  const std::vector<uint8_t> valid = makeDatabase({}, {}, {});

  for (size_t index = 0; index < 12; ++index) {
    std::vector<uint8_t> changed = valid;
    ++changed[index];
    MemoryReader reader(std::move(changed));
    OuiDatabase database;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(OuiOpenStatus::kInvalidFormat),
                          static_cast<int>(database.open(&reader)));
    TEST_ASSERT_FALSE(database.isOpen());
  }

  std::vector<uint8_t> trailing = valid;
  trailing.push_back(0);
  MemoryReader trailing_reader(std::move(trailing));
  OuiDatabase database;
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OuiOpenStatus::kInvalidFormat),
                        static_cast<int>(database.open(&trailing_reader)));

  std::vector<uint8_t> truncated(valid.begin(), valid.end() - 1);
  MemoryReader truncated_reader(std::move(truncated));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OuiOpenStatus::kInvalidFormat),
                        static_cast<int>(database.open(&truncated_reader)));
}

void test_open_rejects_null_reader_and_header_read_failure() {
  OuiDatabase database;
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OuiOpenStatus::kInvalidArgument),
                        static_cast<int>(database.open(nullptr)));

  MemoryReader reader(makeDatabase({}, {}, {}));
  reader.failAt(0);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OuiOpenStatus::kReadError),
                        static_cast<int>(database.open(&reader)));
}

void test_lookup_requires_open_database_only_for_global_addresses() {
  OuiDatabase database;
  OuiLookupResult result = {};
  const uint8_t global[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
  const uint8_t local[6] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x55};

  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(OuiLookupStatus::kDatabaseNotOpen),
      static_cast<int>(database.lookup(global, result)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OuiClassification::kUnknown),
                        static_cast<int>(result.classification));

  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(OuiLookupStatus::kSuccess),
      static_cast<int>(database.lookup(local, result)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OuiClassification::kLocal),
                        static_cast<int>(result.classification));
}

void test_lookup_reports_reader_failure_without_stale_vendor() {
  MemoryReader reader = makeFullDatabase();
  OuiDatabase database;
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OuiOpenStatus::kReady),
                        static_cast<int>(database.open(&reader)));
  reader.failAt(marauder::kOuiHeaderSize + 5 * marauder::kOuiRecordSize);
  const uint8_t mac[6] = {0x00, 0x11, 0x22, 0x33, 0x4F, 0xAA};
  OuiLookupResult result = {OuiClassification::kVendor, 36, "stale"};

  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(OuiLookupStatus::kReadError),
      static_cast<int>(database.lookup(mac, result)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OuiClassification::kUnknown),
                        static_cast<int>(result.classification));
  TEST_ASSERT_EQUAL_UINT8(0, result.prefix_length);
  TEST_ASSERT_EQUAL_STRING("", result.vendor);
}

void test_lookup_rejects_nonterminated_or_nonascii_vendor_name() {
  std::vector<uint8_t> nonterminated = makeDatabase(
      {{{0x00, 0x11, 0x22, 0x00, 0x00}, "valid"}}, {}, {});
  std::memset(nonterminated.data() + marauder::kOuiHeaderSize +
                  marauder::kOuiPrefixKeySize,
              'A', marauder::kOuiNameSize);
  MemoryReader nonterminated_reader(std::move(nonterminated));
  OuiDatabase database;
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(OuiOpenStatus::kReady),
      static_cast<int>(database.open(&nonterminated_reader)));
  const uint8_t mac[6] = {0x00, 0x11, 0x22, 0xAA, 0xBB, 0xCC};
  OuiLookupResult result = {};
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(OuiLookupStatus::kInvalidRecord),
      static_cast<int>(database.lookup(mac, result)));

  std::vector<uint8_t> nonascii = makeDatabase(
      {{{0x00, 0x11, 0x22, 0x00, 0x00}, "valid"}}, {}, {});
  nonascii[marauder::kOuiHeaderSize + marauder::kOuiPrefixKeySize] = 0x80;
  MemoryReader nonascii_reader(std::move(nonascii));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(OuiOpenStatus::kReady),
                        static_cast<int>(database.open(&nonascii_reader)));
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(OuiLookupStatus::kInvalidRecord),
      static_cast<int>(database.lookup(mac, result)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_special_mac_classes_do_not_read_database);
  RUN_TEST(test_lookup_prefers_36_then_28_then_24_bit_prefixes);
  RUN_TEST(test_lookup_uses_big_endian_sorted_keys_and_section_boundaries);
  RUN_TEST(test_global_mac_without_match_is_unknown);
  RUN_TEST(
      test_identify_mac_address_returns_owned_name_and_tolerates_null_reader);
  RUN_TEST(test_open_validates_exact_header_and_file_size);
  RUN_TEST(test_open_rejects_null_reader_and_header_read_failure);
  RUN_TEST(test_lookup_requires_open_database_only_for_global_addresses);
  RUN_TEST(test_lookup_reports_reader_failure_without_stale_vendor);
  RUN_TEST(test_lookup_rejects_nonterminated_or_nonascii_vendor_name);
  return UNITY_END();
}
