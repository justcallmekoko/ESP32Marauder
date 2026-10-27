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
        # Pancake's NeoPixel pin (GPIO27) is this board's I2C SDA; it must stay off.
        override = config[config.index("#ifdef MARAUDER_WS_C5_35_OVERRIDE"):]
        self.assertIn("#undef HAS_NEOPIXEL_LED", override)
        self.assertIn("#define I2C_SDA 27", override)
        sketch = (ROOT / "esp32_marauder" / "esp32_marauder.ino").read_text(encoding="utf-8")
        # Brightness drives the expander PWM register, not the (absent) TFT_BL pin.
        self.assertIn("#define BL_SET(duty)     ws_exio_write(WS_EXIO_PWM_REG, (duty))", sketch)
        self.assertIn("#define WS_EXIO_PWM_REG     0x05", expander)
        menus = (ROOT / "esp32_marauder" / "MenuFunctions.cpp").read_text(encoding="utf-8")
        self.assertIn("#define BL_PREVIEW(duty) ws_exio_write(WS_EXIO_PWM_REG, (duty))", menus)
        self.assertIn("#ifdef MARAUDER_WS_C5_35", touch)
        self.assertIn("Wire.end(); Wire.begin(CTP_SDA, CTP_SCL, 100000U);", touch)
        self.assertIn("return -1;", touch)
        # INT gate: TP_INT on GPIO3, polling mode, and no I2C read without INT LOW.
        self.assertIn("#define CTP_INT 3", config)
        self.assertIn("#define FT6336_G_MODE    0xA4", touch)
        self.assertIn("pinMode(CTP_INT, INPUT);", touch)
        self.assertIn("_ft6336_int_gate_active = modeReadOk && gMode == 0x00;", touch)
        gate = "if (_ft6336_int_gate_active && digitalRead(CTP_INT) != LOW) return 0;"
        self.assertIn(gate, touch)
        self.assertLess(
            touch.index(gate),
            touch.index("Wire.end(); Wire.begin(CTP_SDA, CTP_SCL, 100000U);"),
        )
        self.assertIn("#define WS_EXIO_ADDR        0x24", expander)
        self.assertIn("ws_c5_exio_begin", expander)


if __name__ == "__main__":
    unittest.main()
