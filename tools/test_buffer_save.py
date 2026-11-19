import os
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

STUBS = r"""
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>
#include <cstdio>

bool fail_allocations = false;
void* test_malloc(size_t size) {
  return fail_allocations ? nullptr : std::malloc(size);
}
#define malloc test_malloc

#define BUF_SIZE 64
#define SNAP_LEN 128
#define portMUX_INITIALIZER_UNLOCKED 0
using portMUX_TYPE = int;
int lock_depth = 0;
#define portENTER_CRITICAL(mux) (++lock_depth)
#define portEXIT_CRITICAL(mux) (--lock_depth)
using String = std::string;
String text02;
uint32_t micros() { return 1000; }
struct wifi_promiscuous_pkt_t {};
std::vector<uint8_t> disk;
std::vector<uint8_t> uart;
std::function<void()> during_write;
unsigned file_opens = 0, file_closes = 0;
struct File {
  bool opened;
  explicit File(bool valid = false) : opened(valid) {}
  explicit operator bool() const { return opened; }
  void write(const uint8_t* data, size_t length) {
    assert(lock_depth == 0);
    assert(opened);
    if (during_write) {
      auto callback = during_write;
      during_write = nullptr;
      callback();
    }
    disk.insert(disk.end(), data, data + length);
  }
  void close() {
    assert(lock_depth == 0);
    if (opened) ++file_closes;
    opened = false;
  }
};
#define FILE_APPEND 1
namespace fs { struct FS {
  File open(String, int) {
    assert(lock_depth == 0);
    ++file_opens;
    return File(true);
  }
}; }
struct SerialStub {
  template<class T> void println(T) {}
  void write(const uint8_t* data, size_t length) {
    assert(lock_depth == 0);
    if (during_write) {
      auto callback = during_write;
      during_write = nullptr;
      callback();
    }
    uart.insert(uart.end(), data, data + length);
  }
} Serial;
"""

DRIVER = r"""
int main(int argc, char** argv) {
  const std::string scenario = argv[1];
  fs::FS filesystem;
  Buffer buffer;
  buffer.fs = scenario == "serial" ? nullptr : &filesystem;
  buffer.serial = scenario == "serial";
  buffer.writing = true;
  const uint8_t a[60] = {1};
  const uint8_t b[10] = {2};
  const uint8_t c[3] = {3, 4, 5};
  const uint8_t full[BUF_SIZE] = {6};
  if (scenario == "second-allocation-failure") {
    buffer.serial = true;
    buffer.add(a, sizeof(a), false);
    buffer.add(b, sizeof(b), false);
    during_write = [&]() { fail_allocations = true; };
    buffer.save();
    assert(file_opens == 1 && file_closes == 1);
    assert(buffer.bufSizeA + buffer.bufSizeB == sizeof(b));
    assert(!buffer.saving);
    assert(disk == std::vector<uint8_t>(a, a + sizeof(a)));
    fail_allocations = false;
    buffer.save();
    std::vector<uint8_t> expected(a, a + sizeof(a));
    expected.insert(expected.end(), b, b + sizeof(b));
    const std::string begin = "[BUF/BEGIN]", end = "[BUF/CLOSE]";
    std::vector<uint8_t> expected_uart;
    for (const auto& payload : {std::vector<uint8_t>(a, a + sizeof(a)),
                                std::vector<uint8_t>(b, b + sizeof(b))}) {
      expected_uart.insert(expected_uart.end(), begin.begin(), begin.end());
      expected_uart.insert(expected_uart.end(), payload.begin(), payload.end());
      expected_uart.insert(expected_uart.end(), end.begin(), end.end());
    }
    const bool correct = disk == expected && uart == expected_uart &&
        file_opens == 2 && file_closes == 2;
    const bool drained = buffer.bufSizeA == 0 && buffer.bufSizeB == 0;
    std::printf("%d %d\n", correct, drained);
    std::free(buffer.bufA);
    std::free(buffer.bufB);
    return 0;
  }
  if (scenario == "allocation-failure" || scenario == "allocation-failure-both") {
    const bool filesystem_enabled = scenario == "allocation-failure-both";
    buffer.fs = filesystem_enabled ? &filesystem : nullptr;
    buffer.serial = true;
    buffer.add(a, sizeof(a), false);
    buffer.add(b, sizeof(b), false);
    fail_allocations = true;
    buffer.save();
    assert(uart.empty());
    assert(buffer.bufSizeA + buffer.bufSizeB == sizeof(a) + sizeof(b));
    assert(!buffer.saving);
    buffer.add(c, sizeof(c), false);
    buffer.save();
    assert(uart.empty());
    assert(buffer.bufSizeA + buffer.bufSizeB == sizeof(a) + sizeof(b) + sizeof(c));
    assert(!buffer.saving);
    fail_allocations = false;
    buffer.save();
    buffer.save();
    std::vector<uint8_t> expected(a, a + sizeof(a));
    expected.insert(expected.end(), b, b + sizeof(b));
    expected.insert(expected.end(), c, c + sizeof(c));
    const std::string begin = "[BUF/BEGIN]", end = "[BUF/CLOSE]";
    std::vector<uint8_t> expected_uart(begin.begin(), begin.end());
    expected_uart.insert(expected_uart.end(), a, a + sizeof(a));
    expected_uart.insert(expected_uart.end(), end.begin(), end.end());
    expected_uart.insert(expected_uart.end(), begin.begin(), begin.end());
    expected_uart.insert(expected_uart.end(), b, b + sizeof(b));
    expected_uart.insert(expected_uart.end(), c, c + sizeof(c));
    expected_uart.insert(expected_uart.end(), end.begin(), end.end());
    const bool correct = uart == expected_uart &&
        (filesystem_enabled ? disk == expected : disk.empty());
    const bool drained = buffer.bufSizeA == 0 && buffer.bufSizeB == 0;
    std::printf("%d %d\n", correct, drained);
    std::free(buffer.bufA);
    std::free(buffer.bufB);
    return 0;
  }
  std::vector<uint8_t> expected;
  if (scenario != "empty") {
    const size_t initial = scenario == "both" || scenario == "full" ? sizeof(a) : 4;
    buffer.add(a, initial, false);
    expected.insert(expected.end(), a, a + initial);
    if (scenario == "both") {
      buffer.add(b, sizeof(b), false);
      expected.insert(expected.end(), b, b + sizeof(b));
    }
    if (scenario == "full") {
      during_write = [&]() {
        buffer.add(full, sizeof(full), false);
        buffer.add(c, sizeof(c), false);
      };
      expected.insert(expected.end(), full, full + sizeof(full));
    } else if (scenario != "ordinary") {
      during_write = [&]() { buffer.add(c, sizeof(c), false); };
      expected.insert(expected.end(), c, c + sizeof(c));
    }
  }
  buffer.save();
  const unsigned expected_opens = scenario == "empty" || scenario == "serial" ? 0 : 1;
  assert(file_opens == expected_opens && file_closes == expected_opens);
  if (scenario == "serial") {
    const std::string begin = "[BUF/BEGIN]", end = "[BUF/CLOSE]";
    size_t offset = 0;
    while (offset < uart.size()) {
      assert(std::memcmp(uart.data() + offset, begin.data(), begin.size()) == 0);
      offset += begin.size();
      const size_t length = disk.empty() ? 4 : sizeof(c);
      disk.insert(disk.end(), uart.begin() + offset, uart.begin() + offset + length);
      offset += length;
      assert(std::memcmp(uart.data() + offset, end.data(), end.size()) == 0);
      offset += end.size();
    }
  }
  const bool correct = disk == expected;
  const bool drained = buffer.bufSizeA == 0 && buffer.bufSizeB == 0;
  std::printf("%d %d\n", correct, drained);
  std::free(buffer.bufA);
  std::free(buffer.bufB);
}
"""


class BufferSaveTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = (ROOT / "esp32_marauder" / "Buffer.cpp").read_text()
        header = (ROOT / "esp32_marauder" / "Buffer.h").read_text()
        declaration = header[header.index("class Buffer {"):header.rindex("#endif")]
        declaration = declaration.replace("private:", "public:")

        def between(start, end):
            return source[source.index(start):source.index(end)]

        methods = between("Buffer::Buffer()", "void Buffer::createFile")
        methods += between("void Buffer::add(", "void Buffer::append(")
        methods += source[source.index("bool Buffer::writeUnlocked("):]
        cls.directory = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.directory.cleanup)
        harness = Path(cls.directory.name) / "buffer.cpp"
        harness.write_text(STUBS + declaration + methods + DRIVER)
        cls.binary = Path(cls.directory.name) / "buffer"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++11",
                        str(harness), "-o", str(cls.binary)], check=True)

    def check_scenario(self, name):
        result = subprocess.run([str(self.binary), name], check=True,
                                capture_output=True, text=True)
        self.assertEqual(result.stdout.strip(), "1 1")

    def test_packet_received_during_sd_write_is_saved(self):
        self.check_scenario("during-write")

    def test_pending_buffers_and_new_packet_keep_arrival_order(self):
        self.check_scenario("both")

    def test_ordinary_save_drains_the_buffer(self):
        self.check_scenario("ordinary")

    def test_empty_save_has_no_output(self):
        self.check_scenario("empty")

    def test_full_receive_buffer_does_not_overwrite_in_flight_data(self):
        self.check_scenario("full")

    def test_serial_capture_preserves_frames_received_during_output(self):
        self.check_scenario("serial")

    def test_serial_allocation_failure_retains_packets_until_retry(self):
        self.check_scenario("allocation-failure")

    def test_serial_retry_does_not_duplicate_packets_on_disk(self):
        self.check_scenario("allocation-failure-both")

    def test_file_closes_when_second_serial_allocation_fails(self):
        self.check_scenario("second-allocation-failure")


if __name__ == "__main__":
    unittest.main()
