from __future__ import annotations

import json
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class LilygoTHmiS3HardwareTests(unittest.TestCase):
    """Contract checks for the LilyGo T-HMI ESP32-S3 (MARAUDER_CYD_HMI)."""

    def test_target_is_registered_in_stable_and_nightly_builds(self) -> None:
        target = next(
            target
            for target in json.loads(
                (ROOT / "installer" / "targets.json").read_text(encoding="utf-8")
            )["targets"]
            if target["buildFlag"] == "MARAUDER_CYD_HMI"
        )
        self.assertEqual(target["assetSuffix"], "lilygo-t-hmi")
        self.assertEqual(target["chipFamily"], "ESP32-S3")

        for workflow_name in ("build_parallel.yml", "nightly_build.yml"):
            workflow = (ROOT / ".github" / "workflows" / workflow_name).read_text(
                encoding="utf-8"
            )
            self.assertIn('flag: "MARAUDER_CYD_HMI"', workflow)
            self.assertIn("FlashSize=16M", workflow)
            self.assertIn('tft_file: "User_Setup_LilyGo_T_HMI.h"', workflow)

    def test_board_contract_keeps_display_touch_and_backlight_isolated(self) -> None:
        config = (ROOT / "esp32_marauder" / "configs.h").read_text(encoding="utf-8")
        tft = (ROOT / "User_Setup_LilyGo_T_HMI.h").read_text(encoding="utf-8")
        display = (ROOT / "esp32_marauder" / "Display.h").read_text(encoding="utf-8")
        backlight = (ROOT / "esp32_marauder" / "BackLight.hpp").read_text(
            encoding="utf-8"
        )

        # LilyGo T-HMI S3 (MARAUDER_CYD_HMI): XPT2046 cap-touch, AW9364 backlight
        # dimmer, SD/MMC, and an on-board battery ADC.
        for expected in (
            "#define HAS_XPT2046",
            "#define HAS_CYD_TOUCH",
            "#define HAS_AW9364",
            "#define HAS_SDMMC",
            "#define BATTERY_ADC_PIN 5",
            "#define BK_LIGHT_PIN 38",
        ):
            self.assertIn(expected, config)

        for expected in (
            "#define TFT_WIDTH 240",
            "#define TFT_HEIGHT 320",
            "#define TFT_CS 6",
            "#define TFT_DC 7",
            "#define TFT_BL 38",
        ):
            self.assertIn(expected, tft)

        # Touch handling is isolated in the XPT2046 driver, pulled in for this
        # board via Display.h.
        self.assertIn("#include <XPT2046_Touchscreen.h>", display)
        self.assertIn("defined(MARAUDER_CYD_HMI)", display)
        # Backlight goes through the AW9364 dimmer path in BackLight.hpp.
        self.assertIn("defined(HAS_AW9364)", backlight)


if __name__ == "__main__":
    unittest.main()
