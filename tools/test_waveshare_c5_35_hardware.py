from __future__ import annotations

import json
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class WaveshareC535HardwareTests(unittest.TestCase):
    def test_target_is_registered_in_stable_and_nightly_builds(self) -> None:
        target = next(
            target
            for target in json.loads(
                (ROOT / "installer" / "targets.json").read_text(encoding="utf-8")
            )["targets"]
            if target["buildFlag"] == "MARAUDER_WS_C5_35"
        )
        self.assertEqual(target["assetSuffix"], "ws_c5_35")
        self.assertEqual(target["chipFamily"], "ESP32-C5")

        for workflow_name in ("build_parallel.yml", "nightly_build.yml"):
            workflow = (ROOT / ".github" / "workflows" / workflow_name).read_text(
                encoding="utf-8"
            )
            self.assertIn('flag: "MARAUDER_WS_C5_35"', workflow)
            # Core 3.3.4 exposes at most a 16 MB choice. Marauder intentionally
            # uses its established 8 MB app layout within the board's 32 MB chip.
            self.assertIn("FlashSize=8M", workflow)
            self.assertIn('tft_file: "User_Setup_ws_c5_35.h"', workflow)

    def test_board_contract_keeps_expander_and_touch_handling_isolated(self) -> None:
        config = (ROOT / "esp32_marauder" / "configs.h").read_text(encoding="utf-8")
        tft = (ROOT / "User_Setup_ws_c5_35.h").read_text(encoding="utf-8")
        touch = (ROOT / "esp32_marauder" / "ft6336.h").read_text(encoding="utf-8")
        expander = (ROOT / "esp32_marauder" / "ws_c5_exio.h").read_text(
            encoding="utf-8"
        )

        for expected in (
            "#define I2C_SDA 27",
            "#define I2C_SCL 26",
            "#define SD_CS 9",
        ):
            self.assertIn(expected, config)
        for expected in (
            "#define TFT_MISO  2",
            "#define TFT_MOSI  7",
            "#define TFT_SCLK  6",
            "#define TFT_CS    8",
            "#define TFT_DC    5",
        ):
            self.assertIn(expected, tft)
        # GPS: LC76G over I2C; the UART GPS path (GPIO13/14 = native USB) never starts.
        override = config[config.index("#ifdef MARAUDER_WS_C5_35_OVERRIDE"):]
        self.assertIn("#define HAS_GPS_LC76G_I2C", override)
        gps = (ROOT / "esp32_marauder" / "GpsInterface.cpp").read_text(encoding="utf-8")
        begin = gps[gps.index("void GpsInterface::begin()"):]
        self.assertLess(begin.index("marauder::lc76g::begin()"), begin.index("Serial2.begin("))
        self.assertLess(begin.index("return;"), begin.index("Serial2.begin("))
        lc76g = (ROOT / "esp32_marauder" / "GpsLc76gI2c.cpp").read_text(encoding="utf-8")
        for expected in ("0xAA510008UL", "0xAA512000UL", "RD_ADDR   = 0x54", "CR_ADDR   = 0x50"):
            self.assertIn(expected, lc76g)
        self.assertIn("#ifdef MARAUDER_WS_C5_35", touch)
        self.assertIn("Wire.end(); Wire.begin(CTP_SDA, CTP_SCL, 100000U);", touch)
        self.assertIn("return -1;", touch)
        self.assertIn("#define WS_EXIO_ADDR        0x24", expander)
        self.assertIn("ws_c5_exio_begin", expander)


if __name__ == "__main__":
    unittest.main()
