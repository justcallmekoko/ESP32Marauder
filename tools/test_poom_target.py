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


if __name__ == "__main__":
    unittest.main()
