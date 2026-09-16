from __future__ import annotations

import json
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CONFIG = (ROOT / "esp32_marauder" / "configs.h").read_text(encoding="utf-8")


class PoomTargetTests(unittest.TestCase):
    def test_official_poom_gpio_map_is_preserved(self) -> None:
        expected = {
            "L_BTN": 3,
            "C_BTN": 28,
            "U_BTN": 7,
            "R_BTN": 23,
            "D_BTN": 24,
            "B_BTN": 9,
            "SD_MISO": 8,
            "SD_MOSI": 4,
            "SD_SCK": 6,
            "SD_CS": 5,
            "PIN": 27,
        }
        for macro, value in expected.items():
            self.assertRegex(CONFIG, rf"#define\s+{macro}\s+{value}\b")

    def test_poom_is_a_public_esp32_c5_target(self) -> None:
        registry = json.loads((ROOT / "installer" / "targets.json").read_text())
        poom = next(target for target in registry["targets"] if target["id"] == "poom")
        self.assertEqual(poom["buildFlag"], "MARAUDER_POOM")
        self.assertEqual(poom["assetSuffix"], "poom")
        self.assertEqual(poom["chipFamily"], "ESP32-C5")
        self.assertNotIn("MARAUDER_POOM", registry["privateBuildFlags"])

    def test_poom_uses_oled_adapter_without_changing_tft_targets(self) -> None:
        display_header = (ROOT / "esp32_marauder" / "Display.h").read_text()
        self.assertIn('#ifdef MARAUDER_POOM\n  #include "PoomDisplay.h"', display_header)
        self.assertIn("#else\n  #include <TFT_eSPI.h>", display_header)

    def test_poom_uses_the_c5_rmt_led_backend(self) -> None:
        led_header = (ROOT / "esp32_marauder" / "LedInterface.h").read_text()
        poom_led = (ROOT / "esp32_marauder" / "PoomWs2812.cpp").read_text()
        self.assertIn('#ifdef MARAUDER_POOM\n    #include "PoomWs2812.h"', led_header)
        self.assertIn("rmtInit(pin_, RMT_TX_MODE", poom_led)
        self.assertIn("rmtWrite(pin_, symbols_", poom_led)
        self.assertNotIn("driver/rmt.h", poom_led)

    def test_poom_compact_layout_reserves_the_status_row(self) -> None:
        display = (ROOT / "esp32_marauder" / "Display.cpp").read_text()
        menus = (ROOT / "esp32_marauder" / "MenuFunctions.cpp").read_text()
        recon = (ROOT / "esp32_marauder" / "ReconMission.cpp").read_text()
        self.assertIn("STATUS_BAR_WIDTH + (i * TEXT_HEIGHT)", display)
        self.assertIn('drawString("C" + (String)wifi_scan_obj.old_channel, 0, 0, 1)', menus)
        self.assertIn('drawString("SD", 116, 0, 1)', menus)
        self.assertIn("#ifdef MARAUDER_POOM\n    return;", display)
        self.assertIn('scan_mode == BT_SCAN_ANALYZER ? "BLE Beacons/50ms" : "Frames/50ms"', menus)
        self.assertIn("fillRect(button_x, button_y, KEY_W, KEY_H, background)", menus)
        self.assertIn("RECON_SCREEN_HEIGHT - STATUS_BAR_WIDTH", recon)
        self.assertIn('drawString("B: stop", 0, 54, 1)', recon)

    def test_poom_has_a_physical_button_text_editor(self) -> None:
        menus = (ROOT / "esp32_marauder" / "MenuFunctions.cpp").read_text()
        self.assertRegex(CONFIG, r"#ifdef MARAUDER_POOM[\s\S]*?#define HAS_MINI_KB")
        self.assertIn('drawString("B:cancel", 0, 54, 1)', menus)
        self.assertIn("display_obj.tft.display(true);", menus)
        self.assertIn("if (b_btn.justPressed())", menus)
        self.assertIn("return wifi_scan_obj.current_mini_kb_ssid;", menus)

    def test_poom_flushes_blocking_wifi_progress_and_shows_setting_state(self) -> None:
        menus = (ROOT / "esp32_marauder" / "MenuFunctions.cpp").read_text()
        wifi_scan = (ROOT / "esp32_marauder" / "WiFiScan.cpp").read_text()
        self.assertIn('String(setting_enabled ? "ON: " : "OFF: ") + settingName', menus)
        self.assertIn("specSettingMenu.list->clear();", menus)
        self.assertGreaterEqual(wifi_scan.count("display_obj.tft.display(true);"), 4)
        self.assertIn('miniKbMenu.parentMenu = &wifiAPMenu;', menus)
        self.assertIn('connected ? "Connected" : "Connection failed"', menus)
        self.assertIn('drawCentreString("No saved WiFi"', wifi_scan)
        self.assertIn('drawCentreString("Saved WiFi failed"', wifi_scan)
        self.assertIn('println(F("Connected:"));', wifi_scan)


if __name__ == "__main__":
    unittest.main()
