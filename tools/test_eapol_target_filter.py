import os
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

STUBS = r"""
#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

struct String : std::string {
  using std::string::string;
  void concat(char value) { push_back(value); }
  void concat(const char* value) { append(value); }
};
template<class T> struct LinkedList {
  std::vector<T> items;
  int size() const { return items.size(); }
  T get(int index) const { return items.at(index); }
  void set(int index, T value) { items.at(index) = value; }
};
struct AccessPoint {
  String essid;
  uint8_t channel = 11, bssid[6] = {}, beacon[2] = {}, sec = 0;
  bool selected = false, has_msg_1 = false, has_msg_2 = false;
  bool has_msg_3 = false, has_msg_4 = false, wps = false;
  unsigned packets = 0;
  LinkedList<uint16_t>* stations = nullptr;
};
LinkedList<AccessPoint> points;
LinkedList<AccessPoint>* access_points = &points;
namespace marauder {
template<class T> bool memoryGuardedAdd(LinkedList<T>* list, T value) {
  list->items.push_back(value);
  return true;
}
}
enum wifi_promiscuous_pkt_type_t { WIFI_PKT_MGMT, WIFI_PKT_DATA };
enum { WIFI_IF_AP, WIFI_SCAN_EAPOL, WIFI_SCAN_ACTIVE_EAPOL,
       WIFI_SCAN_ACTIVE_LIST_EAPOL };
struct wifi_promiscuous_pkt_t {
  struct { int sig_len = 128, channel = 11, rssi = -40; } rx_ctrl;
  uint8_t payload[128] = {};
};
struct WifiMgmtHdr {};
struct wifi_ieee80211_packet_t { WifiMgmtHdr hdr; };
void getMAC(char* output, const uint8_t* payload, int offset) {
  const auto* a = payload + offset;
  std::snprintf(output, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
               a[0], a[1], a[2], a[3], a[4], a[5]);
}
struct SerialStub {
  template<class T> void print(T) {}
  void println() {}
} Serial;
#define F(value) value
struct BufferStub {
  unsigned writes = 0;
  void append(wifi_promiscuous_pkt_t*, int) { ++writes; }
} buffer_obj;
unsigned transmissions = 0;
uint8_t transmitted_bssid[6] = {};
uint8_t transmitted_destination[6] = {};
int esp_wifi_80211_tx(int, const void* frame, unsigned, bool) {
  ++transmissions;
  std::memcpy(transmitted_bssid, static_cast<const uint8_t*>(frame) + 16, 6);
  std::memcpy(transmitted_destination, static_cast<const uint8_t*>(frame) + 4, 6);
  return 0;
}
void delay(int) {}
struct WiFiScan {
  unsigned mgmt_frames = 0, req_frames = 0, resp_frames = 0;
  unsigned beacon_frames = 0, deauth_frames = 0, data_frames = 0;
  unsigned eapol_frames = 0;
  int min_rssi = 0, max_rssi = -128;
  bool send_deauth = false;
  int currentScanMode = WIFI_SCAN_EAPOL;
  uint8_t deauth_frame_default[26] = {};
  bool checkMem(unsigned) { return true; }
  uint8_t getSecurityType(const uint8_t*, int) { return 0; }
  int checkMatchAP(const char* address) {
    char candidate[18];
    for (int i = 0; i < access_points->size(); ++i) {
      auto ap = access_points->get(i);
      getMAC(candidate, ap.bssid, 0);
      if (std::strcmp(candidate, address) == 0) return i;
    }
    return -1;
  }
  bool filterActive();
  static void eapolSnifferCallback(void*, wifi_promiscuous_pkt_type_t);
} wifi_scan_obj;
"""

DRIVER = r"""
int main(int argc, char** argv) {
  const std::string scenario = argv[1];
  AccessPoint selected;
  selected.bssid[5] = 1;
  selected.selected = scenario != "empty-targets" && scenario != "unfiltered";
  points.items.push_back(selected);
  AccessPoint other;
  other.bssid[5] = 2;
  points.items.push_back(other);
  wifi_scan_obj.currentScanMode = scenario == "unfiltered"
      ? WIFI_SCAN_ACTIVE_EAPOL : WIFI_SCAN_ACTIVE_LIST_EAPOL;
  wifi_scan_obj.send_deauth = scenario != "passive";
  if (scenario == "passive") wifi_scan_obj.currentScanMode = WIFI_SCAN_EAPOL;
  wifi_promiscuous_pkt_t packet;
  packet.payload[0] = 0x80;
  packet.payload[15] = scenario == "selected" || scenario == "passive" ||
      scenario == "stale-destination" ? 1 : 2;
  if (scenario == "stale-destination")
    std::fill_n(wifi_scan_obj.deauth_frame_default + 4, 6, 0xaa);
  if (scenario == "unknown") packet.payload[15] = 3;
  auto type = WIFI_PKT_MGMT;
  if (scenario == "eapol" || scenario == "foreign-eapol") {
    type = WIFI_PKT_DATA;
    packet.payload[0] = 0x08;
    packet.payload[15] = scenario == "eapol" ? 1 : 2;
    packet.payload[30] = 0x88;
    packet.payload[31] = 0x8e;
    packet.payload[37] = 0x01;
  }
  WiFiScan::eapolSnifferCallback(&packet, type);
  unsigned last = points.get(0).has_msg_2;
  if (scenario == "stale-destination") {
    last = std::all_of(transmitted_destination,
        transmitted_destination + 6, [](uint8_t byte) { return byte == 0xff; });
  }
  std::printf("%u %u %u %u %u\n", transmissions, buffer_obj.writes,
      wifi_scan_obj.eapol_frames, transmitted_bssid[5], last);
  for (auto& ap : points.items) delete ap.stations;
}
"""


class EapolTargetFilterTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = (ROOT / "esp32_marauder" / "WiFiScan.cpp").read_text()
        start = source.index("void WiFiScan::eapolSnifferCallback(")
        end = source.index("// GCOVR_EXCL_START", start)
        cls.directory = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.directory.cleanup)
        harness = Path(cls.directory.name) / "callback.cpp"
        harness.write_text(STUBS + source[start:end] + DRIVER)
        cls.binary = Path(cls.directory.name) / "callback"
        subprocess.run(
            [os.environ.get("CXX", "c++"), "-std=c++11", str(harness),
             "-o", str(cls.binary)], check=True,
        )

    def run_scenario(self, name):
        result = subprocess.run([str(self.binary), name], check=True,
                                capture_output=True, text=True)
        return tuple(map(int, result.stdout.split()))

    def test_selected_beacon_is_transmitted_and_captured(self):
        self.assertEqual(self.run_scenario("selected"), (5, 1, 0, 1, 0))

    def test_active_capture_resets_destination_left_by_another_attack(self):
        self.assertEqual(self.run_scenario("stale-destination"), (5, 1, 0, 1, 1))

    def test_unselected_beacon_on_same_channel_is_not_transmitted(self):
        self.assertEqual(self.run_scenario("unselected"), (0, 0, 0, 0, 0))

    def test_newly_discovered_beacon_is_not_transmitted(self):
        self.assertEqual(self.run_scenario("unknown"), (0, 0, 0, 0, 0))

    def test_targeted_mode_with_no_selected_access_points_does_not_transmit(self):
        self.assertEqual(self.run_scenario("empty-targets"), (0, 0, 0, 0, 0))

    def test_explicit_unfiltered_mode_keeps_existing_behavior(self):
        self.assertEqual(self.run_scenario("unfiltered"), (5, 1, 0, 2, 0))

    def test_passive_capture_does_not_transmit(self):
        self.assertEqual(self.run_scenario("passive"), (0, 1, 0, 0, 0))

    def test_selected_eapol_is_captured_and_updates_handshake_state(self):
        self.assertEqual(self.run_scenario("eapol"), (0, 1, 1, 0, 1))

    def test_unselected_eapol_is_not_captured(self):
        self.assertEqual(self.run_scenario("foreign-eapol"), (0, 0, 0, 0, 0))


if __name__ == "__main__":
    unittest.main()
