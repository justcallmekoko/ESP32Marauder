from __future__ import annotations

import json
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class WaveshareC528HardwareTests(unittest.TestCase):
    def test_target_is_registered_in_stable_and_nightly_builds(self) -> None:
        target = next(
            target
            for target in json.loads(
                (ROOT / "installer" / "targets.json").read_text(encoding="utf-8")
            )["targets"]
            if target["buildFlag"] == "MARAUDER_WS_C5_28"
        )
        self.assertEqual(target["assetSuffix"], "ws-c5-28")
        self.assertEqual(target["chipFamily"], "ESP32-C5")

        for workflow_name in ("build_parallel.yml", "nightly_build.yml"):
            workflow = (ROOT / ".github" / "workflows" / workflow_name).read_text(
                encoding="utf-8"
            )
            self.assertIn('flag: "MARAUDER_WS_C5_28"', workflow)
            self.assertIn("FlashSize=16M", workflow)
            self.assertIn('tft_file: "User_Setup_marauder_ws_c5_28.h"', workflow)

    def test_board_contract_keeps_expander_and_touch_handling_isolated(self) -> None:
        config = (ROOT / "esp32_marauder" / "configs.h").read_text(encoding="utf-8")
        tft = (ROOT / "User_Setup_marauder_ws_c5_28.h").read_text(encoding="utf-8")
        touch = (ROOT / "esp32_marauder" / "CST3530.hpp").read_text(encoding="utf-8")
        expander = (ROOT / "esp32_marauder" / "CH32V00X_IOExpander.hpp").read_text(
            encoding="utf-8"
        )

        for expected in (
            "#define I2C_SDA           0",
            "#define I2C_SCL           1",
            "#define SD_CS             23",
        ):
            self.assertIn(expected, config)
        # WS_C5_28 selects the CST3530 cap-touch controller and the CH32V00X
        # I/O expander (panel power / LCD + touch reset / backlight).
        self.assertIn("#define HAS_CST3530", config)
        self.assertIn("#define HAS_CH32V00X", config)

        for expected in (
            "#define TFT_SCLK   6",
            "#define TFT_MOSI   7",
            "#define TFT_MISO   8",
            "#define TFT_DC     9",
            "#define TFT_CS    10",
        ):
            self.assertIn(expected, tft)

        # Touch handling is isolated in the CST3530 driver (I2C 0x58).
        self.assertIn("#ifdef HAS_CST3530", touch)
        self.assertIn("#define CST3530_I2C_ADDR        (0x58)", touch)
        self.assertIn("class CST3530", touch)

        # Panel power/resets go through the CH32V00X I/O expander at 0x24.
        self.assertIn("#define CH32V00X_DEFAULT_ADDR   (0x24)", expander)
        self.assertIn("class CH32V00X_IOExpander", expander)


if __name__ == "__main__":
    unittest.main()
