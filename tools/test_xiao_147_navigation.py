"""Exercise the actual two-button graph handler without an ESP32 or LCD."""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from test_brightness_restore import function_body


ROOT = Path(__file__).resolve().parents[1]


class Xiao147NavigationTests(unittest.TestCase):
    def test_graph_buttons_cycle_channels_and_pages_and_always_exit(self):
        source = (ROOT / "esp32_marauder" / "WiFiScan.cpp").read_text(
            encoding="utf-8"
        )
        body = function_body(source, "int8_t WiFiScan::checkAnalyzerButtons(")
        harness = r"""
#include <cassert>
#include <cstdint>
#define XIAO_ESP32_S3_TOUCH
#define MAX_CHANNEL 14
#define CHAN_PER_PAGE 7
#define WIFI_SCAN_CHAN_ACT 1
#define WIFI_PACKET_MONITOR 2
#define EXIT_BUTTON_INDEX 3

struct Button {
  bool pending = false;
  bool justPressed() { bool result = pending; pending = false; return result; }
} c_btn, d_btn;

struct Display {
  int channel = 0;
  int scale_draws = 0;
  int exit_draws = 0;
  void tftDrawChannelScaleButtons(int value, bool) {
    channel = value;
    ++scale_draws;
  }
  void tftDrawExitScaleButtons(bool) { ++exit_draws; }
} display_obj;

struct WiFiScan {
  int currentScanMode = 0;
  int set_channel = 1;
  int activity_page = 1;
  int channel_changes = 0;
  int line_draws = 0;
  void changeChannel(int channel) {
    assert(channel >= 1 && channel <= MAX_CHANNEL);
    set_channel = channel;
    ++channel_changes;
  }
  void drawChannelLine() { ++line_draws; }
  int8_t checkAnalyzerButtons(uint32_t currentTime);
};

int8_t WiFiScan::checkAnalyzerButtons(uint32_t currentTime) {
HANDLER_BODY
}

int main() {
  WiFiScan scan;
  assert(scan.checkAnalyzerButtons(0) == -1);
  assert(scan.channel_changes == 0);
  // All graph channels remain reachable with a single NEXT key.
  for (int press = 1; press <= MAX_CHANNEL * 2; ++press) {
    d_btn.pending = true;
    assert(scan.checkAnalyzerButtons(press) == -1);
    assert(scan.set_channel == press % MAX_CHANNEL + 1);
    assert(scan.channel_changes == press);
    assert(display_obj.channel == scan.set_channel);
    assert(display_obj.scale_draws == press);
    assert(display_obj.exit_draws == press);
    assert(scan.checkAnalyzerButtons(press) == -1);
    assert(scan.channel_changes == press);
  }
  // Activity pages cycle without changing the radio channel here.
  scan.currentScanMode = WIFI_SCAN_CHAN_ACT;
  for (int press = 1; press <= 4; ++press) {
    d_btn.pending = true;
    assert(scan.checkAnalyzerButtons(press) == -1);
    assert(scan.activity_page == press % 2 + 1);
    assert(scan.line_draws == press);
    assert(scan.channel_changes == MAX_CHANNEL * 2);
  }
  // Packet Monitor uses changeChannel's own control redraw.
  scan.currentScanMode = WIFI_PACKET_MONITOR;
  scan.set_channel = MAX_CHANNEL;
  int scale_before = display_obj.scale_draws;
  d_btn.pending = true;
  assert(scan.checkAnalyzerButtons(0) == -1);
  assert(scan.set_channel == 1);
  assert(display_obj.scale_draws == scale_before);
  // EXIT wins over a simultaneous NEXT and never tunes the radio.
  int changes_before = scan.channel_changes;
  c_btn.pending = true;
  d_btn.pending = true;
  assert(scan.checkAnalyzerButtons(0) == EXIT_BUTTON_INDEX);
  assert(scan.channel_changes == changes_before);
}
""".replace("HANDLER_BODY", body)
        compiler = shutil.which("g++")
        self.assertIsNotNone(compiler, "g++ is required for the native handler test")
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            cpp = path / "xiao_graph_navigation.cpp"
            binary = path / "xiao_graph_navigation.exe"
            cpp.write_text(harness, encoding="utf-8")
            subprocess.run(
                [compiler, "-std=c++17", "-Wall", "-Wextra", str(cpp), "-o", str(binary)],
                check=True,
                capture_output=True,
                text=True,
            )
            subprocess.run([str(binary)], check=True, capture_output=True, text=True)


if __name__ == "__main__":
    unittest.main()
